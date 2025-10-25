/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <ebml/EbmlMaster.h>
#include <ebml/EbmlStream.h>
#if LIBEBML_VERSION < 0x020000
# include <ebml/EbmlSubHead.h>
#endif
#include <ebml/EbmlVoid.h>
#include <matroska/KaxSemantic.h>

#include "common/at_scope_exit.h"
#include "common/ebml.h"
#include "common/doc_type_version_handler.h"
#include "common/doc_type_version_handler_p.h"
#include "common/list_utils.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/stereo_mode.h"

namespace mtx {

doc_type_version_handler_c::doc_type_version_handler_c()
  : p_ptr{new doc_type_version_handler_private_c}
{
  doc_type_version_handler_private_c::init_tables();
}

doc_type_version_handler_c::~doc_type_version_handler_c() { // NOLINT(modernize-use-equals-default) Need to tell compiler where to put code for this function.
}

libebml::EbmlElement &
doc_type_version_handler_c::render(libebml::EbmlElement &element,
                                   mm_io_c &file,
                                   bool with_default) {
  if (dynamic_cast<libebml::EbmlMaster *>(&element))
    remove_unrenderable_elements(static_cast<libebml::EbmlMaster &>(element), with_default);

  element.Render(file, render_should_write_arg(with_default));
  return account(element, with_default);
}

libebml::EbmlElement &
doc_type_version_handler_c::account(libebml::EbmlElement &element,
                                    bool with_default) {
  if (!with_default && element.IsDefaultValue())
    return element;

  auto p  = p_func();
  auto id = get_ebml_id(element).GetValue();

  if (p->s_version_by_element[id] > p->version) {
    mxdebug_if(p->debug, fmt::format("account: bumping version from {0} to {1} due to ID 0x{2:x}\n", p->version, p->s_version_by_element[id], id));
    p->version = p->s_version_by_element[id];
  }

  if (p->s_read_version_by_element[id] > p->read_version) {
    mxdebug_if(p->debug, fmt::format("account: bumping read_version from {0} to {1} due to ID 0x{2:x}\n", p->read_version, p->s_read_version_by_element[id], id));
    p->read_version = p->s_read_version_by_element[id];
  }

  if (dynamic_cast<libebml::EbmlMaster *>(&element)) {
    auto &master = static_cast<libebml::EbmlMaster &>(element);
    for (auto child : master)
      account(*child);

  } else if (dynamic_cast<libmatroska::KaxVideoStereoMode *>(&element)) {
    auto value = static_cast<libmatroska::KaxVideoStereoMode &>(element).GetValue();
    if (!mtx::included_in(static_cast<stereo_mode_c::mode>(value), stereo_mode_c::mono, stereo_mode_c::unspecified) && (p->version < 3)) {
      mxdebug_if(p->debug, fmt::format("account: bumping version from {0} to 3 due to libmatroska::KaxVideoStereoMode value {1}\n", p->version, value));
      p->version = 3;
    }
  }

  return element;
}

doc_type_version_handler_c::update_result_e
doc_type_version_handler_c::update_ebml_head(mm_io_c &file) {
  auto p      = p_func();
  auto result = do_update_ebml_head(file);

  mxdebug_if(p->debug, fmt::format("update_ebml_head: result {0}\n", static_cast<unsigned int>(result)));

  return result;
}

doc_type_version_handler_c::update_result_e
doc_type_version_handler_c::do_update_ebml_head(mm_io_c &file) {
  auto p = p_func();

  auto previous_pos = file.getFilePointer();
  at_scope_exit_c restore_pos([previous_pos, &file]() { file.setFilePointer(previous_pos); });

  try {
    file.setFilePointer(0);
    auto stream = std::make_shared<libebml::EbmlStream>(file);
    auto head   = std::shared_ptr<libebml::EbmlHead>(static_cast<libebml::EbmlHead *>(stream->FindNextID(EBML_INFO(libebml::EbmlHead), 0xFFFFFFFFL)));

    if (!head)
      return update_result_e::err_no_head_found;

    libebml::EbmlElement *l0{};
    int upper_lvl_el{};
    auto &context = EBML_CONTEXT(head.get());

    head->Read(*stream, context, upper_lvl_el, l0, true, libebml::SCOPE_ALL_DATA);
    head->SkipData(*stream, context);

    auto old_size          = file.getFilePointer() - head->GetElementPosition();
    auto &dt_version       = get_child<libebml::EDocTypeVersion>(*head);
    auto file_version      = dt_version.GetValue();
    auto &dt_read_version  = get_child<libebml::EDocTypeReadVersion>(*head);
    auto file_read_version = dt_read_version.GetValue();
    auto changed           = false;

    if (file_version < p->version) {
      dt_version.SetValue(p->version);
      changed = true;
    }

    if (file_read_version < p->read_version) {
      dt_read_version.SetValue(p->read_version);
      changed = true;
    }

    mxdebug_if(p->debug,
               fmt::format("do_update_ebml_head: account version {0} read_version {1}, file version {2} read_version {3}, changed {4}\n",
                           p->version, p->read_version, file_version, file_read_version, changed));

    if (!changed)
      return update_result_e::ok_no_update_needed;

    if (head->GetSizeLength() > 2) {
      mxdebug_if(p->debug, fmt::format("do_update_ebml_head:   old head size length was {0}, limiting to 2 before updating the element's size\n", head->GetSizeLength()));
      head->SetSizeLength(2);
    }

    head->UpdateSize(render_should_write_arg(true));
    auto new_size = head->ElementSize(render_should_write_arg(true));

    mxdebug_if(p->debug, fmt::format("do_update_ebml_head:   old size {0} new size {1} position {2} size length {3}\n", old_size, new_size, head->GetElementPosition(), head->GetSizeLength()));

    if (new_size > old_size)
      return update_result_e::err_not_enough_space;

    auto diff = old_size - new_size;
    if (diff == 1)
      head->SetSizeLength(head->GetSizeLength() + 1);

    else if (diff > 1) {
      auto v = new libebml::EbmlVoid;
      v->SetSize(diff - 2);
      head->PushElement(*v);
      new_size = head->UpdateSize(render_should_write_arg(true));

      mxdebug_if(p->debug, fmt::format("do_update_ebml_head:   diff > 1 case; new size now {0}\n", new_size));
    }

    file.setFilePointer(head->GetElementPosition());
    head->Render(*stream, render_should_write_arg(true));

  } catch (mtx::mm_io::exception &) {
    return update_result_e::err_read_or_write_failure;
  }

  return update_result_e::ok_updated;
}

} // namespace mtx
