/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace libebml {
class EbmlElement;
}

class mm_io_c;

namespace mtx {

class doc_type_version_handler_private_c;
class doc_type_version_handler_c {
protected:
  MTX_DECLARE_PRIVATE(doc_type_version_handler_private_c)

  std::unique_ptr<doc_type_version_handler_private_c> const p_ptr;

  explicit doc_type_version_handler_c(doc_type_version_handler_private_c &p);

public:
  enum class update_result_e {
    ok_no_update_needed,
    ok_updated,
    err_no_head_found,
    err_not_enough_space,
    err_read_or_write_failure,
  };

public:

  doc_type_version_handler_c();
  virtual ~doc_type_version_handler_c();

  libebml::EbmlElement &account(libebml::EbmlElement &element, bool with_default = false);
  libebml::EbmlElement &render(libebml::EbmlElement &element, mm_io_c &file, bool with_default = false);

  update_result_e update_ebml_head(mm_io_c &file);

private:
  update_result_e do_update_ebml_head(mm_io_c &file);
};

} // namespace mtx
