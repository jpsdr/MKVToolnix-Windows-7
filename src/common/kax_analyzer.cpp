/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Matroska file analyzer and updater

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>

#include <ebml/EbmlStream.h>
#if LIBEBML_VERSION < 0x020000
# include <ebml/EbmlSubHead.h>
#endif
#include <ebml/EbmlVoid.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>

#include "common/bitvalue.h"
#include "common/construct.h"
#include "common/doc_type_version_handler.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/list_utils.h"
#include "common/kax_analyzer.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"

namespace {

constexpr auto CONSOLE_PERCENTAGE_WIDTH = 25;

template<typename Tmaster,
         typename Telement>
kax_analyzer_c::update_element_result_e
update_uid_referrals(kax_analyzer_c &analyzer,
                     std::unordered_map<uint64_t, uint64_t> const &changes) {
  auto master = analyzer.read_all(EBML_INFO(Tmaster));
  if (!master)
    return kax_analyzer_c::uer_success;

  auto modified = change_values<Telement>(static_cast<libebml::EbmlMaster &>(*master), changes);

  return modified ? analyzer.update_element(master) : kax_analyzer_c::uer_success;
}

}

bool
operator <(const kax_analyzer_data_cptr &d1,
           const kax_analyzer_data_cptr &d2) {
  return d1->m_pos < d2->m_pos;
}

std::string
kax_analyzer_data_c::to_string() const {
  const libebml::EbmlCallbacks *callbacks = find_ebml_callbacks(EBML_INFO(libmatroska::KaxSegment), m_id);

  if (!callbacks && is_type<libebml::EbmlVoid>(m_id))
    callbacks = &EBML_CLASS_CALLBACK(libebml::EbmlVoid);

  std::string name;
  if (callbacks)
    name = EBML_INFO_NAME(*callbacks);

  else
    name = fmt::format("0x{0:0{1}x}", m_id.GetValue(), m_id.GetLength() * 2);

  return fmt::format("{0} size {1} at {2}", name, m_size_known ? fmt::to_string(m_size) : "unknown"s, m_pos);
}

kax_analyzer_c::kax_analyzer_c(std::string file_name)
{
  m_file_name  = file_name;
  m_close_file = true;
}

kax_analyzer_c::kax_analyzer_c(mm_io_cptr const &file)
{
  m_file_name  = file->get_file_name();
  m_file       = file;
  m_close_file = false;
}

kax_analyzer_c::~kax_analyzer_c() {
  close_file();
}

void
kax_analyzer_c::close_file() {
  if (m_close_file) {
    m_file.reset();
    m_stream.reset();
  }
}

void
kax_analyzer_c::reopen_file() {
  if (m_file)
    return;

  try {
    m_file = std::make_shared<mm_file_io_c>(m_file_name, m_open_mode);
    if (libebml::MODE_READ == m_open_mode)
      m_file = std::make_shared<mm_read_buffer_io_c>(m_file);

  } catch (mtx::mm_io::exception &) {
    m_file.reset();

    throw libebml::MODE_READ == m_open_mode ? uer_error_opening_for_reading : uer_error_opening_for_writing;
  }

  m_stream = std::make_shared<libebml::EbmlStream>(*m_file);
}

void
kax_analyzer_c::reopen_file_for_writing() {
  if (m_file && (libebml::MODE_WRITE == m_open_mode))
    return;

  m_file.reset();
  m_open_mode = libebml::MODE_WRITE;

  reopen_file();
}

void
kax_analyzer_c::_log_debug_message(const std::string &message) {
  mxinfo(message);
}

bool
kax_analyzer_c::analyzer_debugging_requested(const std::string &section) {
  return m_debug || debugging_c::requested("kax_analyzer_"s + section);
}

void
kax_analyzer_c::debug_dump_elements() {
  size_t i;
  for (i = 0; i < m_data.size(); i++)
    log_debug_message(fmt::format("{0}: {1}\n", i, m_data[i]->to_string()));
}

void
kax_analyzer_c::debug_dump_elements_maybe(const std::string &hook_name) {
  if (!m_debug_elements && !debugging_c::requested("kax_analyzer_"s + hook_name))
    return;

  log_debug_message(fmt::format("kax_analyzer_{0} dumping elements:\n", hook_name));
  debug_dump_elements();
}

void
kax_analyzer_c::validate_data_structures(const std::string &hook_name) {
  debug_dump_elements_maybe(hook_name);

  if (m_data.empty())
    return;

  bool gap_debugging = analyzer_debugging_requested("gaps");
  bool ok            = true;
  size_t i;

  for (i = 0; m_data.size() -1 > i; i++) {
    if ((m_data[i]->m_pos + m_data[i]->m_size) > m_data[i + 1]->m_pos) {
      log_debug_message(fmt::format("kax_analyzer_{0}: Interal data structure corruption at pos {1} (size + position > next position); dumping elements\n", hook_name, i));
      ok = false;
    } else if (gap_debugging && ((m_data[i]->m_pos + m_data[i]->m_size) < m_data[i + 1]->m_pos)) {
      log_debug_message(fmt::format("kax_analyzer_{0}: Gap found at pos {1} (size + position < next position); dumping elements\n", hook_name, i));
      ok = false;
    }
  }

  if (ok)
    return;

  debug_dump_elements();

  if (m_throw_on_error)
    throw mtx::kax_analyzer_x{Y("The data in the file is corrupted and cannot be modified safely")};

  debug_abort_process();
}

void
kax_analyzer_c::verify_data_structures_against_file(const std::string &hook_name) {
  kax_analyzer_c actual_content(m_file);
  actual_content.process();

  unsigned int num_items = std::max(m_data.size(), actual_content.m_data.size());
  bool ok                = m_data.size() == actual_content.m_data.size();
  size_t max_info_len    = 0;
  std::vector<std::string> info_this, info_actual, info_markings;
  size_t i;

  for (i = 0; num_items > i; ++i) {
    info_this.push_back(                 m_data.size() > i ?                m_data[i]->to_string() : std::string{});
    info_actual.push_back(actual_content.m_data.size() > i ? actual_content.m_data[i]->to_string() : std::string{});

    max_info_len           = std::max(max_info_len, info_this.back().length());

    bool row_is_identical  = info_this.back() == info_actual.back();
    ok                    &= row_is_identical;

    info_markings.emplace_back(row_is_identical ? " " : "*");
  }

  if (ok)
    return;

  log_debug_message(fmt::format("verify_data_structures_against_file({0}) failed. Dumping this on the left, actual on the right.\n", hook_name));

  for (i = 0; num_items > i; ++i)
    log_debug_message(fmt::format("{0} {1:<{2}s} {3}\n", info_markings[i], info_this[i], max_info_len, info_actual[i]));

  debug_abort_process();
}

bool
kax_analyzer_c::probe(std::string file_name) {
  try {
    uint8_t data[4];
    mm_file_io_c in(file_name.c_str());

    if (in.read(data, 4) != 4)
      return false;

    return ((0x1a == data[0]) && (0x45 == data[1]) && (0xdf == data[2]) && (0xa3 == data[3]));
  } catch (...) {
    return false;
  }
}

kax_analyzer_c &
kax_analyzer_c::set_parse_mode(parse_mode_e parse_mode) {
  m_parse_mode = parse_mode;
  return *this;
}

kax_analyzer_c &
kax_analyzer_c::set_open_mode(libebml::open_mode mode) {
  m_open_mode = mode;
  return *this;
}

kax_analyzer_c &
kax_analyzer_c::set_throw_on_error(bool throw_on_error) {
  m_throw_on_error = throw_on_error;
  return *this;
}

kax_analyzer_c &
kax_analyzer_c::set_parser_start_position(uint64_t position) {
  m_parser_start_position = position;
  return *this;
}

kax_analyzer_c &
kax_analyzer_c::set_doc_type_version_handler(mtx::doc_type_version_handler_c *handler) {
  m_doc_type_version_handler = handler;
  return *this;
}

bool
kax_analyzer_c::process() {
  try {
    auto result = process_internal();
    mxdebug_if(m_debug, fmt::format("kax_analyzer: parsing file '{0}' result {1}\n", m_file->get_file_name(), result));

    return result;

  } catch (std::exception const &ex) {
    mxdebug_if(m_debug, fmt::format("kax_analyzer: parsing file '{0}' failed with an exception of type {1}: {2}\n", m_file->get_file_name(), typeid(ex).name(), ex.what()));

    show_progress_done();

    if (m_throw_on_error)
      throw;
    return false;

  } catch (...) {
    mxdebug_if(m_debug, fmt::format("kax_analyzer: parsing file '{0}' failed with an unknown exception\n", m_file->get_file_name()));

    show_progress_done();

    if (m_throw_on_error)
      throw;
    return false;
  }
}

bool
kax_analyzer_c::process_internal() {
  bool parse_fully = parse_mode_full == m_parse_mode;

  reopen_file();

  int64_t file_size = m_file->get_size();
  show_progress_start(file_size);

  m_segment.reset();
  m_data.clear();

  m_file->setFilePointer(0);
  m_stream = std::make_shared<libebml::EbmlStream>(*m_file);

  // Find the libebml::EbmlHead element. Must be the first one.
  m_ebml_head.reset(static_cast<libebml::EbmlHead *>(m_stream->FindNextID(EBML_INFO(libebml::EbmlHead), 0xFFFFFFFFL)));
  if (!m_ebml_head || !is_type<libebml::EbmlHead>(*m_ebml_head))
    throw mtx::kax_analyzer_x(Y("Not a valid Matroska file (no EBML head found)"));

  libebml::EbmlElement *l0{};
  int upper_lvl_el{};

  m_ebml_head->Read(*m_stream, EBML_CONTEXT(m_ebml_head.get()), upper_lvl_el, l0, true, libebml::SCOPE_ALL_DATA);
  m_ebml_head->SkipData(*m_stream, EBML_CONTEXT(m_ebml_head.get()));

  determine_webm();

  if (l0) {
    delete l0;
    l0 = nullptr;
  }

  while (true) {
    // Next element must be a segment
    l0 = m_stream->FindNextID(EBML_INFO(libmatroska::KaxSegment), 0xFFFFFFFFFFFFFFFFLL);
    if (!l0)
      throw mtx::kax_analyzer_x(Y("Not a valid Matroska file (no segment/level 0 element found)"));

    if (is_type<libmatroska::KaxSegment>(l0))
      break;

    l0->SkipData(*m_stream, EBML_CONTEXT(l0));
    delete l0;
  }

  m_segment            = std::shared_ptr<libmatroska::KaxSegment>(static_cast<libmatroska::KaxSegment *>(l0));
  bool aborted         = false;
  bool cluster_found   = false;
  bool meta_seek_found = false;
  m_segment_end        = m_segment->IsFiniteSize() ? m_segment->GetDataStart() + m_segment->GetSize() : m_file->get_size();
  upper_lvl_el         = 0;
  libebml::EbmlElement *l1{};

  // In certain situations the caller doesn't way to have to pay the
  // price for full analysis. Then it can configure the parser to
  // start parsing at a certain offset. libebml::EbmlStream::FindNextElement()
  // should take care of re-syncing to a known level 1 element. But
  // take care not to start before the segment's data start position.
  if (m_parser_start_position)
    m_file->setFilePointer(std::max<uint64_t>(*m_parser_start_position, m_segment->GetDataStart()));

  // We've got our segment, so let's find all level 1 elements.
  while (m_file->getFilePointer() < m_segment_end) {
    if (!l1)
      l1 = m_stream->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true, 1);

    if (!l1 || (0 < upper_lvl_el))
      break;

    m_data.push_back(kax_analyzer_data_c::create(get_ebml_id(*l1), l1->GetElementPosition(), l1->ElementSize(render_should_write_arg(true)), l1->IsFiniteSize()));

    cluster_found   |= is_type<libmatroska::KaxCluster>(l1);
    meta_seek_found |= is_type<libmatroska::KaxSeekHead>(l1);

    // auto id = l1->GetClassId().GetValue();
    auto pos_before = m_file->getFilePointer();
    auto is_unknown = !l1->IsFiniteSize();
    l1->SkipData(*m_stream, EBML_CONTEXT(l1));
    delete l1;
    l1 = nullptr;

    if (is_unknown) {
      mxinfo(fmt::format("skip unknown before {0} after {1}\n", pos_before, m_file->getFilePointer()));
    }

    aborted = !show_progress_running((int)(m_file->getFilePointer() * 100 / file_size));

    auto in_parent = !m_segment->IsFiniteSize()
                  || (m_file->getFilePointer() < (m_segment->GetDataStart() + m_segment->GetSize()));

    // mxdebug_if(m_debug, fmt::format("dings read at {0} elt 0x{6:x} in_parent {1} aborted {2} cluster_found {3} meta_seek_found {4} parse_fully {5}\n", m_file->getFilePointer(), in_parent, aborted, cluster_found, meta_seek_found, parse_fully, id));

    if (!in_parent || aborted || (cluster_found && meta_seek_found && !parse_fully))
      break;

  } // while (l1)

  if (l1)
    delete l1;

  if (!aborted && !parse_fully)
    read_all_meta_seeks();

  show_progress_done();

  validate_data_structures("process_internal_end");

  if (!aborted) {
    if (parse_mode_full != m_parse_mode)
      fix_element_sizes(file_size);

    return true;
  }

  m_segment.reset();
  m_data.clear();

  return false;
}

ebml_element_cptr
kax_analyzer_c::read_element(kax_analyzer_data_cptr const &element_data) {
  return read_element(*element_data);
}

ebml_element_cptr
kax_analyzer_c::read_element(unsigned int pos) {
  return read_element(*m_data[pos]);
}

ebml_element_cptr
kax_analyzer_c::read_element(kax_analyzer_data_c const &element_data) {
  reopen_file();

  libebml::EbmlStream es(*m_file);
  m_file->setFilePointer(element_data.m_pos);

  int upper_lvl_el_found         = 0;
  ebml_element_cptr e            = ebml_element_cptr(es.FindNextElement(EBML_CONTEXT(m_segment.get()), upper_lvl_el_found, 0xFFFFFFFFL, true, 1));
  const libebml::EbmlCallbacks *callbacks = find_ebml_callbacks(EBML_INFO(libmatroska::KaxSegment), element_data.m_id);

  if (!e || !callbacks || (get_ebml_id(*e) != EBML_INFO_ID(*callbacks))) {
    e.reset();
    return e;
  }

  upper_lvl_el_found        = 0;
  libebml::EbmlElement *upper_lvl_el = nullptr;
  e->Read(*m_stream, EBML_INFO_CONTEXT(*callbacks), upper_lvl_el_found, upper_lvl_el, true);

  return e;
}

bool
kax_analyzer_c::validate_and_break(std::string const &hook_name) {
  mxdebug_if(m_debug, fmt::format("validate_and_break {0}\n", hook_name));
  debug_dump_elements_maybe(hook_name);
  validate_data_structures(hook_name);

  if (analyzer_debugging_requested("verify"))
    verify_data_structures_against_file(hook_name);

  return debugging_c::requested(fmt::format("kax_analyzer_{0}_break", hook_name));
}

kax_analyzer_c::update_element_result_e
kax_analyzer_c::update_element(ebml_element_cptr const &e,
                               bool write_defaults,
                               bool add_mandatory_elements_if_missing) {
  return update_element(e.get(), write_defaults, add_mandatory_elements_if_missing);
}

kax_analyzer_c::update_element_result_e
kax_analyzer_c::update_element(libebml::EbmlElement *e,
                               bool write_defaults,
                               bool add_mandatory_elements_if_missing) {
  try {
    reopen_file_for_writing();

    if (add_mandatory_elements_if_missing)
      fix_mandatory_elements(e);
    remove_voids_from_master(e);

    placement_strategy_e strategy = get_placement_strategy_for(e);

    if (validate_and_break("update_element_0"))
      return uer_success;

    fix_unknown_size_for_last_level1_element();
    if (validate_and_break("update_element_1"))
      return uer_success;

    overwrite_all_instances(get_ebml_id(*e));
    if (validate_and_break("update_element_2"))
      return uer_success;

    merge_void_elements();
    if (validate_and_break("update_element_3"))
      return uer_success;

    write_element(e, write_defaults, strategy);
    if (validate_and_break("update_element_4"))
      return uer_success;

    remove_from_meta_seeks(get_ebml_id(*e));
    if (validate_and_break("update_element_5"))
      return uer_success;

    merge_void_elements();
    if (validate_and_break("update_element_6"))
      return uer_success;

    add_to_meta_seek(e);
    if (validate_and_break("update_element_7"))
      return uer_success;

    merge_void_elements();
    if (validate_and_break("update_element_8"))
      return uer_success;

  } catch (kax_analyzer_c::update_element_result_e result) {
    debug_dump_elements_maybe("update_element_exception");
    return result;

  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug, fmt::format("I/O exception: {0}\n", ex.what()));
    return uer_error_unknown;
  }

  return uer_success;
}

kax_analyzer_c::update_element_result_e
kax_analyzer_c::remove_elements(libebml::EbmlId const &id) {
  try {
    reopen_file_for_writing();

    if (validate_and_break("remove_elements_0"))
      return uer_success;

    fix_unknown_size_for_last_level1_element();
    if (validate_and_break("remove_elements_1"))
      return uer_success;

    overwrite_all_instances(id);
    if (validate_and_break("remove_elements_2"))
      return uer_success;

    merge_void_elements();
    if (validate_and_break("remove_elements_3"))
      return uer_success;

    remove_from_meta_seeks(id);
    if (validate_and_break("remove_elements_4"))
      return uer_success;

    merge_void_elements();
    if (validate_and_break("remove_elements_5"))
      return uer_success;

  } catch (kax_analyzer_c::update_element_result_e result) {
    debug_dump_elements_maybe("update_element_exception");
    return result;
  }

  return uer_success;
}

kax_analyzer_c::update_element_result_e
kax_analyzer_c::update_uid_referrals(std::unordered_map<uint64_t, uint64_t> const &track_uid_changes) {
  mxdebug_if(m_debug, fmt::format("kax_analyzer: update_track_uid_referrals: number of changes: {0}\n", track_uid_changes.size()));

  if (track_uid_changes.empty())
    return uer_success;

  mxdebug_if(m_debug, fmt::format("kax_analyzer: update_track_uid_referrals: number of changes: {0}\n", track_uid_changes.size()));

  auto result = ::update_uid_referrals<libmatroska::KaxChapters, libmatroska::KaxChapterTrackNumber>(*this, track_uid_changes);
  if (result != uer_success)
    return result;

  result = ::update_uid_referrals<libmatroska::KaxTags, libmatroska::KaxTagTrackUID>(*this, track_uid_changes);
  if (result != uer_success)
    return result;

  return uer_success;
}

/** \brief Sets the m_segment size to the length of the file
 */
void
kax_analyzer_c::adjust_segment_size() {
  // If the old segment's size is unknown then don't try to force a
  // finite size as this will fail most of the time: an
  // infinite/unknown size is coded by the value 0 which is often
  // stored as a single byte (e.g. Haali's muxer does this).
  if (!m_segment->IsFiniteSize())
    return;

  auto new_segment = std::make_shared<libmatroska::KaxSegment>();
  m_file->setFilePointer(m_segment->GetElementPosition());
  new_segment->WriteHead(*m_file, get_head_size(*m_segment) - 4);

  m_file->setFilePointer(0, libebml::seek_end);
  if (!new_segment->ForceSize(m_file->getFilePointer() - m_segment->GetDataStart())) {
    m_segment->OverwriteHead(*m_file);
    throw uer_error_segment_size_for_element;
  }

  new_segment->OverwriteHead(*m_file);
  m_segment = new_segment;
}

/** \brief Create an libebml::EbmlVoid element at a specific location

    This function fills a gap in the file with an libebml::EbmlVoid. If an
    libebml::EbmlVoid element is located directly behind the gap then this
    element is overwritten as well.

    The function calculates the size of the new void element by taking
    the next non-EbmlVoid's position and subtracting from it the end
    position of the current element indicated by the \c data_idx
    parameter.

    If the space is not big enough to contain an libebml::EbmlVoid element then
    the EBML head of the following element is moved one byte to the
    front and its size field is extended by one byte. That way the
    file stays compatible with all parsers, and only a small number of
    bytes have to be moved around.

    The \c m_data member structure is also updated to reflect the
    changes made to the file.

    The function relies on \c m_data[data_idx] to be up to date
    regarding its size. If the size of \c m_data[data_idx] is zero
    then it is assumed that the element shall be overwritten with an
    libebml::EbmlVoid element, and \c m_data[data_idx] will be removed from the
    \c m_data structure.

    \param data_idx Index into the \c m_data structure pointing to the
     current element after which the gap is located.

    \return \c true if a new void element was created and \c false if
      there was no need to create one or if there was not enough
      space.
 */
bool
kax_analyzer_c::handle_void_elements(size_t data_idx) {
  static debugging_option_c s_debug_void{"kax_analyzer_handle_void_elements"};

  // Are the following elements libebml::EbmlVoid elements?
  size_t end_idx = data_idx + 1;
  while ((m_data.size() > end_idx) && is_type<libebml::EbmlVoid>(m_data[end_idx]->m_id))
    ++end_idx;

  // Is the element at the end of the file or are there only void
  // elements following? If so truncate the file and remove the
  // elements from the data structure if that was requested. Then
  // we're done.
  if (m_data.size() == end_idx) {
    mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): element is at end or only void elements following; truncating file; m_data.size() {1}\n", data_idx, m_data.size()));
    m_file->truncate(m_data[data_idx]->m_pos + m_data[data_idx]->m_size);
    adjust_segment_size();
    if (0 == m_data[data_idx]->m_size)
      m_data.erase(m_data.begin() + data_idx, m_data.end());

    return false;
  }

  if (end_idx > data_idx + 1) {
    mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): {1} void element(s) following; merging\n", data_idx, end_idx - data_idx - 1));

    // Yes, there is at least one. Remove these elements from the list
    // in order to create a new libebml::EbmlVoid element covering their space
    // as well.
    m_data.erase(m_data.begin() + data_idx + 1, m_data.begin() + end_idx);
  }

  // Calculate how much space we have to cover with a void
  // element. This is the difference between the next element's
  // position and the current element's end.
  int64_t void_pos = m_data[data_idx]->m_pos + m_data[data_idx]->m_size;
  int void_size    = m_data[data_idx + 1]->m_pos - void_pos;

  // If the difference is 0 then we have nothing to do.
  if (0 == void_size) {
    mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): void_size == 0, nothing further to do\n", data_idx));
    return false;
  }

  // See if we have enough space to fit an libebml::EbmlVoid element in. An
  // libebml::EbmlVoid element needs at least two bytes (one for the ID, one
  // for the size).
  if (1 == void_size) {
    mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): void_size == 1; move next element's head down one byte & enlarge its size portion\n", data_idx));

    // No. The most compatible way to deal with this situation is to
    // move the element ID of the following element one byte to the
    // front and extend the following element's size field by one
    // byte.

    // If the following element's coded size length is 8 bytes
    // already, we can make the head smaller instead and move it one
    // byte up. That leaves a gap of two bytes which we can fill with
    // an empty new EBML Void element. The variable `move_up` reflects
    // this; it is true the coded size length is less than 8 bytes and
    // the element's head can be moved up and false if we have to
    // shrink it and fill the space with a new EBML void.

    auto e = read_element(m_data[data_idx + 1]);

    if (!e) {
      mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): void_size == 1: could not read following element\n", data_idx));
      return false;
    }

    auto move_up = e->GetSizeLength() < 8;
    auto new_pos = m_data[data_idx + 1]->m_pos + (move_up ? -1 : 1);

    uint8_t head[4 + 8];         // Class D + 64 bits coded size
    unsigned int head_size = get_ebml_id(*e).GetLength();
    get_ebml_id(*e).Fill(head);

    int coded_size = libebml::CodedSizeLength(e->GetSize(), move_up ? e->GetSizeLength() + 1 : 7, true);
    libebml::CodedValueLength(e->GetSize(), coded_size, &head[head_size]);
    head_size += coded_size;

    mxdebug_if(s_debug_void,
               fmt::format("handle_void_elements({0}): void_size == 1: writing {1} header bytes at position {2} (ID length {3} coded size length {4} previous size length {5} element size {6}); bytes: {7}\n",
                           data_idx, head_size, new_pos, head_size - coded_size, coded_size, e->GetSizeLength(), e->GetSize(), mtx::string::to_hex(head, head_size)));

    m_file->setFilePointer(new_pos);
    m_file->write(head, head_size);

    auto original_position        = m_data[data_idx + 1]->m_pos;
    m_data[data_idx + 1]->m_pos   = new_pos;
    m_data[data_idx + 1]->m_size += move_up ? 1 : -1;

    if (!move_up) {
      // Need to add an empty new EBML void element.
      mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): void_size == 1: adding new two-byte EBML void element at position {1}\n", data_idx, new_pos - 2));

      m_file->setFilePointer(new_pos - 2);

      libebml::EbmlVoid evoid;
      evoid.SetSize(0);
      evoid.Render(*m_file);

      m_data.insert(m_data.begin() + data_idx + 1, kax_analyzer_data_c::create(EBML_ID(libebml::EbmlVoid), new_pos - 2, 2));
      ++data_idx;
    }

    // Update meta seek indices for m_data[data_idx]'s new position.
    e = read_element(m_data[data_idx + 1]);

    mxdebug_if(s_debug_void, fmt::format("handle_void_elements({0}): void_size == 1: element re-read; now removing from meta seeks, merging void elements etc.\n", data_idx));

    remove_from_meta_seeks(get_ebml_id(*e));
    merge_void_elements();
    add_to_meta_seek(e.get());
    merge_void_elements();

    if (is_type<libmatroska::KaxCluster>(*e)) {
      mxdebug_if(s_debug_void,
                 fmt::format("handle_void_elements({0}): shifted element is cluster; adjusting cues; original relative position {1} new relative position {2}\n",
                             data_idx, m_segment->GetRelativePosition(original_position), m_segment->GetRelativePosition(e->GetElementPosition())));
      adjust_cues_for_cluster(static_cast<libmatroska::KaxCluster &>(*e), m_segment->GetRelativePosition(original_position));
    }

    return false;
  }

  m_file->setFilePointer(void_pos);

  // Yes. Write a new libebml::EbmlVoid element and update the internal records.

  // Calculating the void element's content size. This is not straight
  // forward because there are special values for the total size for
  // which the header size must be forced to be one byte more than would
  // be strictly necessary in order to occupy the space. Here's an
  // example:

  // A two-byte header field (one for the ID, one for the content
  // size) can have a content size of at most 127 bytes, meaning a
  // total size of 129 is easy to achieve: set the content size to 127
  // bytes, libEBML will calculate the required length of the content
  // size field to be 1 and the resulting element's total size will be
  // the desired 129 bytes.

  // A content size of 128 would mean that the content size field must be
  // at least two bytes long. Taking the element's ID into account this
  // would mean a total element size of 128 + 2 + 1 = 131 bytes. So
  // getting libEBML to produce an element of a total size of 131 is
  // easy, too: set the content size to 128 bytes, and libEBML will do
  // the rest.

  // The problematic total size is a 130 bytes. There simply is no
  // content size for which adding the minimal length of the content
  // size field and 1 for the ID would result in a total size of 130
  // bytes.

  // The solution is writing the length field with more bytes than
  // necessary. In the case of a total size of 130 bytes we could use
  // the maximum one-byte content size = 127 bytes, add one byte for the
  // ID and force the content size field's length to be two instead of
  // one byte (0x407f instead of 0xff).

  // Similar corner cases exist for the transition between content
  // size field being two/three bytes, three/four bytes long etc. In
  // order to keep the code simple we always use an eight-bytes long
  // content size field if the total size is at least nine bytes and a
  // one-byte long content size field otherwise.

  libebml::EbmlVoid evoid;
  if (void_size < 9)
    evoid.SetSize(void_size - 2);

  else {
    evoid.SetSize(void_size - 9);
    evoid.SetSizeLength(8);
  }

  evoid.Render(*m_file);

  m_data.insert(m_data.begin() + data_idx + 1, kax_analyzer_data_c::create(EBML_ID(libebml::EbmlVoid), void_pos, void_size));

  // Now check if we should overwrite the current element with the
  // libebml::EbmlVoid element. That is the case if the current element's size
  // is 0. In that case simply remove the element from the data
  // vector.
  if (0 == m_data[data_idx]->m_size)
    m_data.erase(m_data.begin() + data_idx);

  return true;
}

void
kax_analyzer_c::adjust_cues_for_cluster(libmatroska::KaxCluster const &cluster,
                                        uint64_t original_relative_position) {
  static debugging_option_c s_debug_cues{"kax_analyzer_adjust_cues_for_cluster"};

  auto cues = read_all(EBML_INFO(libmatroska::KaxCues));
  if (!cues) {
    mxdebug_if(s_debug_cues, fmt::format("adjust_cues_for_cluster: no cues found\n"));
    return;
  }

  mxdebug_if(s_debug_cues, fmt::format("adjust_cues_for_cluster: cues found; looking for relative position {0}, rewriting to new position {1}\n", original_relative_position, m_segment->GetRelativePosition(cluster)));

  auto modified = false;

  for (auto const &potential_cue_point : *cues) {
    if (!dynamic_cast<libmatroska::KaxCuePoint *>(potential_cue_point))
      continue;

    for (auto const &potential_track_positions : static_cast<libmatroska::KaxCuePoint &>(*potential_cue_point)) {
      if (!dynamic_cast<libmatroska::KaxCueTrackPositions *>(potential_track_positions))
        continue;

      auto cluster_position = find_child<libmatroska::KaxCueClusterPosition>(static_cast<libmatroska::KaxCueTrackPositions &>(*potential_track_positions));
      if (!cluster_position || (cluster_position->GetValue() != original_relative_position))
        continue;

      cluster_position->SetValue(m_segment->GetRelativePosition(cluster));
      modified = true;
    }
  }

  mxdebug_if(s_debug_cues, fmt::format("adjust_cues_for_cluster: modifed? {0}\n", modified));

  if (modified)
    update_element(cues);
}

/** \brief Removes all seek entries for a specific element

    Iterates over the level 1 elements in the file and reads each seek
    head it finds. All entries for the given \c id are removed from
    the seek head. If the seek head has been changed then it is
    rewritten to its original position. The space freed up is filled
    with a new libebml::EbmlVoid element.

    \param id The ID of the elements that should be removed.
 */
void
kax_analyzer_c::remove_from_meta_seeks(libebml::EbmlId id) {
  size_t data_idx;

  for (data_idx = 0; m_data.size() > data_idx; ++data_idx) {
    // We only have to do work on SeekHead elements. Skip the others.
    if (!is_type<libmatroska::KaxSeekHead>(m_data[data_idx]->m_id))
      continue;

    // Read the element from the file. Remember its size so that a new
    // libebml::EbmlVoid element can be constructed afterwards.
    auto element   = read_element(data_idx);
    auto seek_head = dynamic_cast<libmatroska::KaxSeekHead *>(element.get());
    if (!seek_head)
      throw uer_error_unknown;

    int64_t old_size = seek_head->ElementSize(render_should_write_arg(true));

    // Iterate over its children and delete the ones we're looking for.
    bool modified = false;
    size_t sh_idx = 0;
    while (seek_head->ListSize() > sh_idx) {
      if (!is_type<libmatroska::KaxSeek>((*seek_head)[sh_idx])) {
        ++sh_idx;
        continue;
      }

      auto seek_entry = dynamic_cast<libmatroska::KaxSeek *>((*seek_head)[sh_idx]);

      if (!seek_entry->IsEbmlId(id)) {
        ++sh_idx;
        continue;
      }

      delete (*seek_head)[sh_idx];
      seek_head->Remove(sh_idx);

      modified = true;
    }

    // Only rewrite the element to the file if it has been modified.
    if (!modified)
      continue;

    // If the seek head is now empty then simply remove and overwrite
    // it with a void element.
    if (0 == seek_head->ListSize()) {
      m_data[data_idx]->m_size = 0;
      handle_void_elements(data_idx);

      continue;
    }

    // First make sure the new element is smaller than the old one.
    // The following code cannot deal with the other case.
    seek_head->UpdateSize(render_should_write_arg(true));
    int64_t new_size = seek_head->ElementSize(render_should_write_arg(true));
    if (new_size > old_size)
      throw uer_error_unknown;

    // Overwrite the element itself and update its internal record.
    m_file->setFilePointer(m_data[data_idx]->m_pos);
    seek_head->Render(*m_file, render_should_write_arg(true));
    if (m_doc_type_version_handler)
      m_doc_type_version_handler->account(*seek_head, true);

    m_data[data_idx]->m_size = new_size;

    // Create a void element to cover the freed space.
    handle_void_elements(data_idx);
  }
}

/** \brief Overwrites all instances of a specific level 1 element

    Iterates over the level 1 elements in the file and overwrites
    each instance of a specific level 1 element given by \c id.
    It is replaced with a new libebml::EbmlVoid element.

    \param id The ID of the elements that should be overwritten.
 */
void
kax_analyzer_c::overwrite_all_instances(libebml::EbmlId id) {
  size_t data_idx;

  for (data_idx = 0; m_data.size() > data_idx; ++data_idx) {
    // We only have to do work on specific elements. Skip the others.
    if (m_data[data_idx]->m_id != id)
      continue;

    // Overwrite with a void element.
    m_data[data_idx]->m_size = 0;
    handle_void_elements(data_idx);
  }
}

/** \brief Merges consecutive libebml::EbmlVoid elements into a single one

    Iterates over the level 1 elements in the file and merges
    consecutive libebml::EbmlVoid elements into a single one which covers
    the same space as the smaller ones combined.

    Void elements at the end of the file are removed as well.
 */
void
kax_analyzer_c::merge_void_elements() {
  size_t start_idx = 0;

  while (m_data.size() > start_idx) {
    // We only have to do work on libebml::EbmlVoid elements. Skip the others.
    if (!is_type<libebml::EbmlVoid>(m_data[start_idx]->m_id)) {
      ++start_idx;
      continue;
    }

    // Found an libebml::EbmlVoid element. See how many consecutive libebml::EbmlVoid elements
    // there are at this position and calculate the combined size.
    size_t end_idx  = start_idx + 1;
    size_t new_size = m_data[start_idx]->m_size;
    while ((m_data.size() > end_idx) && is_type<libebml::EbmlVoid>(m_data[end_idx]->m_id)) {
      new_size += m_data[end_idx]->m_size;
      ++end_idx;
    }

    // Is this only a single libebml::EbmlVoid element? If yes continue.
    if (end_idx == (start_idx + 1)) {
      start_idx += 2;
      continue;
    }

    // Write the new libebml::EbmlVoid element to the file.
    m_file->setFilePointer(m_data[start_idx]->m_pos);

    libebml::EbmlVoid evoid;
    evoid.SetSize(new_size);
    evoid.UpdateSize();
    evoid.SetSize(new_size - get_head_size(evoid));
    evoid.Render(*m_file);

    // Update the internal records to reflect the changes.
    m_data[start_idx]->m_size = new_size;
    m_data.erase(m_data.begin() + start_idx + 1, m_data.begin() + end_idx);

    start_idx += 2;
  }

  // See how many void elements there are at the end of the file.
  start_idx = m_data.size();

  while ((0 < start_idx) && is_type<libebml::EbmlVoid>(m_data[start_idx - 1]->m_id))
    --start_idx;

  // If there are none then we're done.
  if (m_data.size() <= start_idx)
    return;

  mxdebug_if(m_debug, fmt::format("merge_void_elements: removing trailing void elements from start_idx {0} to m_data.size {1}\n", start_idx, m_data.size()));

  // Truncate the file after the last non-void element and update the segment size.
  m_file->truncate(m_data[start_idx]->m_pos);
  adjust_segment_size();

  // Lastly remove the elements from our internal records.
  m_data.erase(m_data.begin() + start_idx, m_data.end());
}

/** \brief Finds a suitable spot for an element and writes it to the file

    First, a suitable spot for the element is determined by looking at
    libebml::EbmlVoid elements. If none is found in the middle of the file then
    the element will be appended at the end.

    Second, the element is written at the location determined in the
    first step. If libebml::EbmlVoid elements are overwritten then a new,
    smaller one is created which covers the remainder of the
    overwritten one.

    Third, the internal records are updated to reflect the changes.

    \param e Pointer to the element to write.
    \param write_defaults Boolean that decides whether or not elements
      which contain their default value are written to the file.
 */
void
kax_analyzer_c::write_element(libebml::EbmlElement *e,
                              bool write_defaults,
                              placement_strategy_e strategy) {
  e->UpdateSize(render_should_write_arg(write_defaults), true);
  int64_t element_size = e->ElementSize(render_should_write_arg(write_defaults));

  size_t data_idx;
  for (data_idx = (ps_anywhere == strategy ? 0 : m_data.size() - 1); m_data.size() > data_idx; ++data_idx) {
    // We're only interested in libebml::EbmlVoid elements. Skip the others.
    if (!is_type<libebml::EbmlVoid>(m_data[data_idx]->m_id))
      continue;

    // Skip the element if it doesn't provide enough space.
    if (m_data[data_idx]->m_size < element_size)
      continue;

    // We've found our element. Overwrite it.
    m_file->setFilePointer(m_data[data_idx]->m_pos);
    e->Render(*m_file, render_should_write_arg(write_defaults), false, true);
    if (m_doc_type_version_handler)
      m_doc_type_version_handler->account(*e, write_defaults);

    // Update the internal records.
    m_data[data_idx]->m_id   = get_ebml_id(*e);
    m_data[data_idx]->m_size = e->ElementSize(render_should_write_arg(write_defaults));

    // Create a new void element after the element we've just written.
    handle_void_elements(data_idx);

    // We're done.
    return;
  }

  // We haven't found a suitable place. So store the element at the end of the file
  // and update the internal records.
  m_file->setFilePointer(0, libebml::seek_end);
  e->Render(*m_file, render_should_write_arg(write_defaults), false, true);
  if (m_doc_type_version_handler)
    m_doc_type_version_handler->account(*e, write_defaults);
  m_data.push_back(kax_analyzer_data_c::create(get_ebml_id(*e), m_file->getFilePointer() - e->ElementSize(render_should_write_arg(write_defaults)), e->ElementSize(render_should_write_arg(write_defaults))));

  // Adjust the segment's size.
  adjust_segment_size();
}

int
kax_analyzer_c::ensure_front_seek_head_links_to(unsigned int seek_head_idx) {
  // It is possible that the seek head at the front has been removed
  // due to moving the seek head at the back one byte to the front
  // which calls remove_from_meta_seeks() with that second seek head.

  // If this seek head is located at the front then we've got nothing
  // to do. At the same time look for a seek head at the start.

  mxdebug_if(m_debug, fmt::format("ensure_front_seek_head_links_to start\n"));

  std::optional<unsigned int> first_seek_head_idx;

  for (int data_idx = 0, end = m_data.size(); end > data_idx; ++data_idx) {
    auto const &data = *m_data[data_idx];

    if (is_type<libmatroska::KaxSeekHead>(data.m_id)) {
      if (static_cast<unsigned int>(data_idx) == seek_head_idx)
        return seek_head_idx;

      first_seek_head_idx = data_idx;

    } else if (is_type<libmatroska::KaxCluster>(data.m_id))
      break;
  }

  // This seek head is not located at the start. If there is another
  // seek head present then the rest of the code has already taken
  // care of everything.
  if (first_seek_head_idx)
    return *first_seek_head_idx;

  // Our seek head is at the end and there's no seek head at the
  // start.
  mxdebug_if(m_debug, fmt::format("  no seek head at start but one at the end\n"));

  auto seek_head_position = m_segment->GetRelativePosition(m_data[seek_head_idx]->m_pos);
  auto seek_head_id       = memory_c::alloc(4);

  put_uint32_be(seek_head_id->get_buffer(), EBML_ID(libmatroska::KaxSeekHead).GetValue());

  auto new_seek_head = ebml_master_cptr{
    mtx::construct::cons<libmatroska::KaxSeekHead>(mtx::construct::cons<libmatroska::KaxSeek>(new libmatroska::KaxSeekID,       seek_head_id,
                                                                                              new libmatroska::KaxSeekPosition, seek_head_position))
  };

  new_seek_head->UpdateSize();
  auto needed_size = static_cast<int64_t>(new_seek_head->ElementSize(render_should_write_arg(true)));
  auto first_time  = true;

  while (first_time) {
    mxdebug_if(m_debug, fmt::format("  looking for place for the new seek head at the start…\n"));
    // Find a place at the front with enough space.
    for (int data_idx = 0, end = m_data.size(); data_idx < end; ++data_idx) {
      auto &data = *m_data[data_idx];

      if (is_type<libmatroska::KaxCluster>(data.m_id))
        break;

      if (!is_type<libebml::EbmlVoid>(data.m_id))
        continue;

      // Avoid the case with the space being only one byte larger than
      // what we need. That would require moving the next element one
      // byte to the front triggering seek head adjustments.
      if ((data.m_size != needed_size) && (data.m_size < (needed_size + 2)))
        continue;

      mxdebug_if(m_debug, fmt::format("  got one! writing at file position {0}\n", data.m_pos));
      // Got a place. Write the seek head, update the internal record &
      // write a new void element.
      m_file->setFilePointer(data.m_pos);
      new_seek_head->Render(*m_file, render_should_write_arg(true));
      if (m_doc_type_version_handler)
        m_doc_type_version_handler->account(*new_seek_head, true);

      data.m_size = needed_size;
      data.m_id   = EBML_ID(libmatroska::KaxSeekHead);

      handle_void_elements(data_idx);

      return data_idx;
    }

    mxdebug_if(m_debug, fmt::format("  no place, moving level 1 elements and trying again\n"));
    // We haven't found a spot. Move an existing level 1 element to the
    // end if we haven't done that yet and try again. Otherwise fail.
    if (first_time && !move_level1_element_before_cluster_to_end_of_file())
      break;

    first_time = false;
  }

  throw uer_error_unknown;

  return -1;
}

std::pair<bool, int>
kax_analyzer_c::try_adding_to_existing_meta_seek(libebml::EbmlElement *e) {
  mxdebug_if(m_debug, fmt::format("try_adding_to_existing_meta_seek start\n"));
  auto first_seek_head_idx = -1;

  for (auto data_idx = 0u; m_data.size() > data_idx; ++data_idx) {
    // We only have to do work on SeekHead elements. Skip the others.
    if (!is_type<libmatroska::KaxSeekHead>(m_data[data_idx]->m_id))
      continue;

    // Calculate how much free space there is behind the seek head.
    // merge_void_elements() guarantees that there is no libebml::EbmlVoid element
    // at the end of the file and that all consecutive libebml::EbmlVoid elements
    // have been merged into a single element.
    size_t available_space = m_data[data_idx]->m_size;
    if (((data_idx + 1) < m_data.size()) && is_type<libebml::EbmlVoid>(m_data[data_idx + 1]->m_id))
      available_space += m_data[data_idx + 1]->m_size;

    // Read the seek head, index the element and see how much space it needs.
    auto element   = read_element(data_idx);
    auto seek_head = dynamic_cast<libmatroska::KaxSeekHead *>(element.get());
    if (!seek_head)
      throw uer_error_unknown;

    if (-1 == first_seek_head_idx)
      first_seek_head_idx = data_idx;

    seek_head->IndexThis(*e, *m_segment.get());
    seek_head->UpdateSize(render_should_write_arg(true));

    // We can use this seek head if it is at the end of the file, or if there
    // is enough space behind it in form of void elements.
    auto is_at_end         = m_data.size() == (data_idx + 1);
    auto have_enough_space = seek_head->ElementSize(render_should_write_arg(true)) <= available_space;
    auto use_this          = is_at_end || have_enough_space;

    mxdebug_if(m_debug, fmt::format("  seek head idx {0} available_space {1} at end? {2} enough space? {3} use? {4}\n", data_idx, available_space, is_at_end, have_enough_space, use_this));

    if (!use_this)
      continue;

    // Write the seek head.
    m_file->setFilePointer(m_data[data_idx]->m_pos);
    seek_head->Render(*m_file, render_should_write_arg(true));
    if (m_doc_type_version_handler)
      m_doc_type_version_handler->account(*seek_head, true);

    // Update the internal record.
    m_data[data_idx]->m_size = seek_head->ElementSize(render_should_write_arg(true));

    // If this seek head is located at the end of the file then we have
    // to adjust the segment size.
    if (m_data.size() == (data_idx + 1))
      adjust_segment_size();

    else
      // Otherwise create an libebml::EbmlVoid to fill the gap (if any).
      handle_void_elements(data_idx);

    // Now ensure that the seek head is either at the front of the
    // file itself or that it is referenced from one at the front. It
    // is possible that the seek head at the front has been removed
    // due to moving the seek head at the back one byte to the front
    // which calls remove_from_meta_seeks() with that second seek head.
    first_seek_head_idx = ensure_front_seek_head_links_to(data_idx);

    // We're done.
    return { true, first_seek_head_idx };
  }

  return { false, first_seek_head_idx };
}

void
kax_analyzer_c::move_seek_head_to_end_and_create_new_one_at_start(libebml::EbmlElement *e,
                                                                  int first_seek_head_idx) {
  mxdebug_if(m_debug, fmt::format("move_seek_head_to_end_and_create_new_one_at_start start first_seek_head_idx {0}\n", first_seek_head_idx));

  // Read the first seek head…
  auto element   = read_element(first_seek_head_idx);
  auto seek_head = dynamic_cast<libmatroska::KaxSeekHead *>(element.get());
  if (!seek_head)
    throw uer_error_unknown;

  // …index our element…
  seek_head->IndexThis(*e, *m_segment.get());
  seek_head->UpdateSize(render_should_write_arg(true));

  // …write the seek head at the end of the file…
  m_file->setFilePointer(0, libebml::seek_end);
  seek_head->Render(*m_file, render_should_write_arg(true));
  if (m_doc_type_version_handler)
    m_doc_type_version_handler->account(*seek_head, true);

  // …and update the internal records.
  m_data.push_back(kax_analyzer_data_c::create(EBML_ID(libmatroska::KaxSeekHead), seek_head->GetElementPosition(), seek_head->ElementSize(render_should_write_arg(true))));

  // Update the segment size.
  adjust_segment_size();

  // Create a new seek head and write it to the file.
  std::shared_ptr<libmatroska::KaxSeekHead> forward_seek_head(new libmatroska::KaxSeekHead);
  forward_seek_head->IndexThis(*seek_head, *m_segment.get());
  forward_seek_head->UpdateSize(render_should_write_arg(true));

  auto size_diff = static_cast<int>(m_data[first_seek_head_idx]->m_size) - static_cast<int>(forward_seek_head->ElementSize(render_should_write_arg(true)));

  mxdebug_if(m_debug, fmt::format("  trailing seek head written; segment size adjusted; forward seek head size {0} available(first seek head's size) {1} diff {2}\n", m_data[first_seek_head_idx]->m_size, forward_seek_head->ElementSize(render_should_write_arg(true)), size_diff));

  if (size_diff < 0) {
    mxdebug_if(m_debug, fmt::format("  not enough space! voiding existing entry & re-trying to create_new_meta_seek_at_start\n"));

    auto &data = *m_data[first_seek_head_idx];

    m_file->setFilePointer(data.m_pos);

    libebml::EbmlVoid evoid;
    evoid.SetSize(data.m_size);
    evoid.UpdateSize();
    evoid.SetSize(data.m_size - get_head_size(evoid));
    evoid.Render(*m_file);

    // Update the internal records to reflect the changes.
    data.m_id = EBML_ID(libebml::EbmlVoid);

    // We don't have a seek head to copy. Create one before the first chapter if possible.
    if (create_new_meta_seek_at_start(seek_head))
      return;

    mxdebug_if(m_debug, fmt::format("  no place found for one; trying move_seek_head_to_end_and_create_new_one_at_start\n"));

    // We haven't found a place for the new seek head before the first
    // cluster. Therefore we must try to move an existing level 1
    // element to the end of the file first.
    if (move_level1_element_before_cluster_to_end_of_file())
      add_to_meta_seek(seek_head);

    return;
  }

  m_file->setFilePointer(m_data[first_seek_head_idx]->m_pos);
  forward_seek_head->Render(*m_file, render_should_write_arg(true));
  if (m_doc_type_version_handler)
    m_doc_type_version_handler->account(*forward_seek_head, true);

  // Update the internal record to reflect that there's a new seek head.
  m_data[first_seek_head_idx]->m_size = forward_seek_head->ElementSize(render_should_write_arg(true));

  mxdebug_if(m_debug, fmt::format("  about to handle void elements\n"));

  // Create a void element behind the small new first seek head.
  handle_void_elements(first_seek_head_idx);

  mxdebug_if(m_debug, fmt::format("  void elements handled\n"));

  // We're done.
}

bool
kax_analyzer_c::create_new_meta_seek_at_start(libebml::EbmlElement *e) {
  mxdebug_if(m_debug, fmt::format("create_new_meta_seek_at_start start\n"));

  auto new_seek_head = std::make_shared<libmatroska::KaxSeekHead>();
  new_seek_head->IndexThis(*e, *m_segment.get());
  new_seek_head->UpdateSize(render_should_write_arg(true));

  for (auto data_idx = 0u; m_data.size() > data_idx; ++data_idx) {
    auto &data = *m_data[data_idx];

    // We can only overwrite void elements. Skip the others.
    if (!is_type<libebml::EbmlVoid>(data.m_id))
      continue;

    // Skip the element if it doesn't offer enough space for the seek head.
    if (data.m_size < static_cast<int64_t>(new_seek_head->ElementSize(render_should_write_arg(true))))
      continue;

    mxdebug_if(m_debug, fmt::format("  spot at idx {0} size {1} file pos {2}\n", data_idx, data.m_size, data.m_pos));

    // We've found a suitable spot. Write the seek head.
    m_file->setFilePointer(data.m_pos);
    new_seek_head->Render(*m_file, render_should_write_arg(true));
    if (m_doc_type_version_handler)
      m_doc_type_version_handler->account(*new_seek_head, true);

    // Adjust the internal records for the new seek head.
    data.m_size = new_seek_head->ElementSize(render_should_write_arg(true));
    data.m_id   = EBML_ID(libmatroska::KaxSeekHead);

    // Write a void element after the newly written seek head in order to
    // cover the space previously occupied by the old void element.
    handle_void_elements(data_idx);

    // We're done.
    return true;
  }

  return false;
}

bool
kax_analyzer_c::move_level1_element_before_cluster_to_end_of_file() {
  auto candidates_for_moving = std::vector<std::pair<int, unsigned int> >{};

  for (auto data_idx = 0u; m_data.size() > data_idx; ++data_idx) {
    auto const &id = m_data[data_idx]->m_id;

    if (is_type<libmatroska::KaxCluster>(id))
      break;

    if (mtx::included_in(id, EBML_ID(libmatroska::KaxAttachments)))
      candidates_for_moving.emplace_back(10, data_idx);

    else if (id == EBML_ID(libmatroska::KaxTracks))
      candidates_for_moving.emplace_back(20, data_idx);

    else if (id == EBML_ID(libmatroska::KaxInfo))
      candidates_for_moving.emplace_back(30, data_idx);
  }

  // Have we found at least one suitable element before the first
  // cluster? If not bail out.
  if (candidates_for_moving.empty())
    return false;

  // Sort by their importance first and position second. Lower
  // importance means the element is more likely to be moved.
  std::sort(candidates_for_moving.begin(), candidates_for_moving.end());

  auto const to_move_idx = candidates_for_moving.front().second;
  auto const &to_move    = *m_data[to_move_idx];

  mxdebug_if(m_debug, fmt::format("Moving level 1 at index {0} to the end ({1})\n", to_move_idx, to_move.to_string()));

  // We read the element and write it again at the end of the file.
  m_file->setFilePointer(to_move.m_pos);
  auto buf = m_file->read(to_move.m_size);

  m_file->setFilePointer(0, libebml::seek_end);
  auto position = m_file->getFilePointer();

  m_file->write(buf);

  // Update the internal records.
  m_data.push_back(kax_analyzer_data_c::create(to_move.m_id, position, to_move.m_size));

  // Overwrite with a void element.
  m_data[to_move_idx]->m_size = 0;
  handle_void_elements(to_move_idx);

  debug_dump_elements_maybe("move_level1_element_before_cluster_to_end_of_file");

  verify_data_structures_against_file("move_level1_element_before_cluster_to_end_of_file");

  // And add it to a meta seek element.
  auto e = read_element(m_data.size() - 1);
  if (!e)
    throw uer_error_unknown;

  add_to_meta_seek(e.get());

  return true;
}

/** \brief Adds an element to one of the meta seek entries

    This function iterates over all meta seek elements and looks
    for one that has enough space (via following libebml::EbmlVoid elements or
    because it is located at the end of the file) for indexing
    the element \c e.

    If no such element is found then a new meta seek element is
    created at an appropriate place, and that element is indexed.

    \param e Pointer to the element to index.
 */
void
kax_analyzer_c::add_to_meta_seek(libebml::EbmlElement *e) {
  auto result = try_adding_to_existing_meta_seek(e);

  mxdebug_if(m_debug, fmt::format("add_to_meta_seek: adding to existing result {0}/{1}\n", result.first, result.second));

  if (result.first)
    return;

  // No suitable meta seek head found -- we have to write a new one.

  // If we have found a prior seek head then we copy that one to the end
  // of the file including the newly indexed element and write a one-element
  // seek head at the first meta seek's position pointing to the one at the
  // end.

  if (-1 != result.second) {
    move_seek_head_to_end_and_create_new_one_at_start(e, result.second);
    return;
  }

  // We don't have a seek head to copy. Create one before the first chapter if possible.
  if (create_new_meta_seek_at_start(e))
    return;

  // We haven't found a place for the new seek head before the first
  // cluster. Therefore we must try to move an existing level 1
  // element to the end of the file first.
  if (move_level1_element_before_cluster_to_end_of_file()) {
    add_to_meta_seek(e);
    return;
  }

  // This is not supported at the moment.
  throw uer_error_not_indexable;
}

ebml_master_cptr
kax_analyzer_c::read_all(const libebml::EbmlCallbacks &callbacks) {
  reopen_file();

  ebml_master_cptr master;
  libebml::EbmlStream es(*m_file);
  size_t i;

  for (i = 0; m_data.size() > i; ++i) {
    kax_analyzer_data_c &data = *m_data[i].get();
    if (EBML_INFO_ID(callbacks) != data.m_id)
      continue;

    m_file->setFilePointer(data.m_pos);
    int upper_lvl_el = 0;
    auto element     = es.FindNextElement(EBML_CLASS_CONTEXT(libmatroska::KaxSegment), upper_lvl_el, 0xFFFFFFFFL, true);
    if (!element)
      continue;

    if (get_ebml_id(*element) != EBML_INFO_ID(callbacks)) {
      delete element;
      continue;
    }

    libebml::EbmlElement *l2 = nullptr;
    element->Read(*m_stream, EBML_INFO_CONTEXT(callbacks), upper_lvl_el, l2, true);

    if (!master)
      master = ebml_master_cptr(static_cast<libebml::EbmlMaster *>(element));
    else {
      auto src = static_cast<libebml::EbmlMaster *>(element);
      while (src->ListSize() > 0) {
        master->PushElement(*(*src)[0]);
        src->Remove(0);
      }
      delete element;
    }
  }

  if (master && (master->ListSize() == 0))
    master.reset();

  return master;
}

void
kax_analyzer_c::read_all_meta_seeks() {
  m_meta_seeks_by_position.clear();

  unsigned int i, num_entries = m_data.size();
  std::map<int64_t, bool> positions_found;

  for (i = 0; i < num_entries; i++)
    positions_found[m_data[i]->m_pos] = true;

  for (i = 0; i < num_entries; i++)
    if (is_type<libmatroska::KaxSeekHead>(m_data[i]->m_id))
      read_meta_seek(m_data[i]->m_pos, positions_found);

  std::sort(m_data.begin(), m_data.end());
}

void
kax_analyzer_c::read_meta_seek(uint64_t pos,
                               std::map<int64_t, bool> &positions_found) {
  if (m_meta_seeks_by_position[pos])
    return;

  m_meta_seeks_by_position[pos] = true;

  m_file->setFilePointer(pos);

  int upper_lvl_el = 0;
  auto l1          = m_stream->FindNextElement(EBML_CONTEXT(m_segment.get()), upper_lvl_el, 0xFFFFFFFFL, true, 1);

  if (!l1)
    return;

  if (!is_type<libmatroska::KaxSeekHead>(l1)) {
    delete l1;
    return;
  }

  libebml::EbmlElement *l2 = nullptr;
  auto master              = static_cast<libebml::EbmlMaster *>(l1);
  master->Read(*m_stream, EBML_CONTEXT(l1), upper_lvl_el, l2, true);

  unsigned int i;
  for (i = 0; master->ListSize() > i; i++) {
    if (!is_type<libmatroska::KaxSeek>((*master)[i]))
      continue;

    auto seek        = static_cast<libmatroska::KaxSeek *>((*master)[i]);
    auto seek_id     = find_child<libmatroska::KaxSeekID>(seek);
    int64_t seek_pos = seek->Location() + m_segment->GetDataStart();

    if ((0 == pos) || !seek_id)
      continue;

    if (positions_found[seek_pos])
      continue;

    auto the_id = create_ebml_id_from(*seek_id);
    m_data.push_back(kax_analyzer_data_c::create(the_id, seek_pos, -1));
    positions_found[seek_pos] = true;

    if (is_type<libmatroska::KaxSeekHead>(the_id))
      read_meta_seek(seek_pos, positions_found);
  }

  delete l1;
}

void
kax_analyzer_c::fix_element_sizes(uint64_t file_size) {
  unsigned int i;
  for (i = 0; m_data.size() > i; ++i)
    if (-1 == m_data[i]->m_size)
      m_data[i]->m_size = ((i + 1) < m_data.size() ? m_data[i + 1]->m_pos : file_size) - m_data[i]->m_pos;
}

void
kax_analyzer_c::fix_unknown_size_for_last_level1_element() {
  if (!m_data.size())
    return;

  auto &data = *m_data.back();
  if (data.m_size_known)
    return;

  mxinfo(fmt::format("chunky bacon! data {0} seg end {1}\n", data.to_string(), m_segment_end));

  auto elt = read_element(m_data.size() - 1);
  if (!elt)
    throw uer_error_fixing_last_element_unknown_size_failed;

  auto head_size       = static_cast<unsigned int>(get_head_size(*elt));
  auto actual_size     = m_segment_end - (elt->GetElementPosition() + head_size);
  auto required_bytes  = libebml::CodedSizeLength(actual_size, 0);
  auto available_bytes = elt->GetSizeLength();

  if ((available_bytes < required_bytes) || !elt->ForceSize(actual_size))
    throw uer_error_fixing_last_element_unknown_size_failed;

  elt->OverwriteHead(*m_stream, true);

  data.m_size       = actual_size + head_size;
  data.m_size_known = true;

  log_debug_message(fmt::format("fix_unknown_size_for_last_level1_element: element fixed to new payload size {0} head size {1} segment end {2}\n", actual_size, head_size, m_segment_end));
}

kax_analyzer_c::placement_strategy_e
kax_analyzer_c::get_placement_strategy_for(libebml::EbmlElement *e) {
  return is_type<libmatroska::KaxTags>(e) ? ps_end : ps_anywhere;
}

libebml::EbmlHead &
kax_analyzer_c::get_ebml_head() {
  return *m_ebml_head;
}

bool
kax_analyzer_c::is_webm()
  const {
  return m_is_webm;
}

uint64_t
kax_analyzer_c::get_segment_pos()
  const {
  if (!m_segment)
    return 0;

  return m_segment->GetElementPosition();
}

uint64_t
kax_analyzer_c::get_segment_data_start_pos()
  const {
  if (!m_segment)
    return 0;

  return m_segment->GetDataStart();
}

mtx::bits::value_cptr
kax_analyzer_c::read_segment_uid_from(std::string const &file_name) {
  try {
    auto analyzer = std::make_shared<kax_analyzer_c>(file_name);
    auto ok       = analyzer
      ->set_parse_mode(kax_analyzer_c::parse_mode_fast)
      .set_open_mode(libebml::MODE_READ)
      .process();

    if (ok) {
      auto element      = analyzer->read_all(EBML_INFO(libmatroska::KaxInfo));
      auto segment_info = dynamic_cast<libmatroska::KaxInfo *>(element.get());
      auto segment_uid  = segment_info ? find_child<libmatroska::KaxSegmentUID>(segment_info) : nullptr;

      if (segment_uid)
        return std::make_shared<mtx::bits::value_c>(*segment_uid);
    }

  } catch (mtx::mm_io::exception &ex) {
    throw mtx::kax_analyzer_x{fmt::format(FY("The file '{0}' could not be opened for reading: {1}."), file_name, ex)};

  } catch (mtx::kax_analyzer_x &ex) {
    throw mtx::kax_analyzer_x{fmt::format(FY("The file '{0}' could not be opened for reading: {1}."), file_name, ex)};

  } catch (...) {
    throw mtx::kax_analyzer_x{fmt::format(FY("The file '{0}' could not be opened or parsed."), file_name)};

  }

  throw mtx::kax_analyzer_x{fmt::format(FY("No segment UID could be found in the file '{0}'."), file_name)};
}

int
kax_analyzer_c::find(libebml::EbmlId const &id) {
  for (int idx = 0, end = m_data.size(); idx < end; idx++)
    if (id == m_data[idx]->m_id)
      return idx;

  return -1;
}

void
kax_analyzer_c::with_elements(const libebml::EbmlId &id,
                              std::function<void(kax_analyzer_data_c const &)> worker)
  const {
  for (auto const &data : m_data)
    if (data->m_id == id)
      worker(*data);
}

void
kax_analyzer_c::determine_webm() {
  auto doc_type = find_child<libebml::EDocType>(*m_ebml_head);
  m_is_webm     = doc_type && (doc_type->GetValue() == "webm");
}


// ------------------------------------------------------------

bool m_show_progress;
int m_previous_percentage;

console_kax_analyzer_c::console_kax_analyzer_c(std::string file_name)
  : kax_analyzer_c(file_name)
  , m_show_progress(false)
  , m_previous_percentage(-1)
{
}

void
console_kax_analyzer_c::set_show_progress(bool show_progress) {
  if (-1 == m_previous_percentage)
    m_show_progress = show_progress;
}

void
console_kax_analyzer_c::show_progress_start(int64_t) {
  if (!m_show_progress)
    return;

  m_previous_percentage = -1;
  show_progress_running(0);
}

bool
console_kax_analyzer_c::show_progress_running(int percentage) {
  if (!m_show_progress || (percentage == m_previous_percentage))
    return true;

  std::string full_bar(        percentage  * CONSOLE_PERCENTAGE_WIDTH / 100, '=');
  std::string empty_bar((100 - percentage) * CONSOLE_PERCENTAGE_WIDTH / 100, ' ');

  mxinfo(fmt::format(FY("Progress: [{0}{1}] {2}%"), full_bar, empty_bar, percentage));
  mxinfo("\r");

  m_previous_percentage = percentage;

  return true;
}

void
console_kax_analyzer_c::show_progress_done() {
  if (!m_show_progress)
    return;

  show_progress_running(100);
  mxinfo("\n");
}

void
console_kax_analyzer_c::debug_abort_process() {
}
