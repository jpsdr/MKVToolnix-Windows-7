/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   high level Matroska file reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <typeinfo>

#include <ebml/EbmlCrc32.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>

#include "common/ebml.h"
#include "common/fs_sys_helpers.h"
#include "common/kax_file.h"
#include "common/mm_io_x.h"
#include "common/strings/formatting.h"

kax_file_c::kax_file_c(mm_io_c &in)
  : m_in(in)
  , m_resynced{}
  , m_resync_start_pos{}
  , m_file_size{static_cast<uint64_t>(m_in.get_size())}
  , m_segment_end{}
  , m_timestamp_scale{TIMESTAMP_SCALE}
  , m_last_timestamp{-1}
  , m_es{new EbmlStream{m_in}}
  , m_debug_read_next{"kax_file|kax_file_read_next"}
  , m_debug_resync{   "kax_file|kax_file_resync"}
{
}

kax_file_c::~kax_file_c() {
}

EbmlElement *
kax_file_c::read_next_level1_element(uint32_t wanted_id,
                                     bool report_cluster_timestamp) {
  try {
    auto element = read_next_level1_element_internal(wanted_id);

    if (report_cluster_timestamp && (-1 != m_timestamp_scale))
      report(boost::format(Y("The first cluster timestamp after the resync is %1%.\n"))
             % format_timestamp(FindChildValue<KaxClusterTimecode>(static_cast<KaxCluster *>(element)) * m_timestamp_scale));

    return element;

  } catch (mtx::mm_io::exception &e) {
    mxwarn(boost::format("%1% %2% %3%\n")
           % (boost::format(Y("%1%: an exception occurred (message: %2%; type: %3%).")) % "kax_file_c::read_next_level1_element()" % (boost::format("%1% / %2%") % e.what() % e.error()) % typeid(e).name())
           % Y("This usually indicates a damaged file structure.") % Y("The file will not be processed further."));

  } catch (std::exception &e) {
    mxwarn(boost::format("%1% %2% %3%\n")
           % (boost::format(Y("%1%: an exception occurred (message: %2%; type: %3%).")) % "kax_file_c::read_next_level1_element()" % e.what() % typeid(e).name())
           % Y("This usually indicates a damaged file structure.") % Y("The file will not be processed further."));

  } catch (...) {
    mxwarn(boost::format("%1% %2% %3%\n")
           % (boost::format(Y("%1%: an unknown exception occurred.")) % "kax_file_c::read_next_level1_element()")
           % Y("This usually indicates a damaged file structure.") % Y("The file will not be processed further."));
  }
  return nullptr;
}

EbmlElement *
kax_file_c::read_next_level1_element_internal(uint32_t wanted_id) {
  if (m_segment_end && (m_in.getFilePointer() >= m_segment_end))
    return nullptr;

  m_resynced         = false;
  m_resync_start_pos = 0;

  // Read the next ID.
  auto search_start_pos = m_in.getFilePointer();
  auto actual_id        = vint_c::read_ebml_id(m_in);
  m_in.setFilePointer(search_start_pos, seek_beginning);

  if (m_debug_read_next)
    mxinfo(boost::format("kax_file::read_next_level1_element(): search at %1% for %|4$x| act id %|2$x| is_valid %3%\n") % search_start_pos % actual_id.m_value % actual_id.is_valid() % wanted_id);

  // If no valid ID was read then re-sync right away. No other tests
  // can be run.
  if (!actual_id.is_valid())
    return resync_to_level1_element(wanted_id);


  // Easiest case: next level 1 element following the previous one
  // without any element inbetween.
  if (   (wanted_id == actual_id.m_value)
      || (   (0 == wanted_id)
          && (   is_level1_element_id(actual_id)
              || is_global_element_id(actual_id)))) {
    auto l1 = read_one_element();
    if (l1)
      return l1;
  }

  // If a specific level 1 is wanted then look for next ID by skipping
  // other level 1 or special elements. If that files fallback to a
  // byte-for-byte search for the ID.
  if ((0 != wanted_id) && (is_level1_element_id(actual_id) || is_global_element_id(actual_id))) {
    m_in.setFilePointer(search_start_pos, seek_beginning);
    auto l1 = read_one_element();

    if (l1) {
      auto element_size = get_element_size(l1);
      auto ok           = (0 != element_size) && m_in.setFilePointer2(l1->GetElementPosition() + element_size, seek_beginning);

      if (m_debug_read_next)
        mxinfo(boost::format("kax_file::read_next_level1_element(): other level 1 element %1% new pos %2% fsize %3% epos %4% esize %5%\n")
               % EBML_NAME(l1)  % (l1->GetElementPosition() + element_size) % m_file_size
               % l1->GetElementPosition() % element_size);

      delete l1;

      return ok ? read_next_level1_element(wanted_id) : nullptr;
    }
  }

  // Last case: no valid ID found. Try a byte-for-byte search for the
  // next wanted/level 1 ID. Also try to locate at least three valid
  // ID/sizes, not just one ID.
  m_in.setFilePointer(search_start_pos, seek_beginning);
  return resync_to_level1_element(wanted_id);
}

EbmlElement *
kax_file_c::read_one_element() {
  if (m_segment_end && (m_in.getFilePointer() >= m_segment_end))
    return nullptr;

  auto upper_lvl_el = 0;
  auto l1           = m_es->FindNextElement(EBML_CLASS_CONTEXT(KaxSegment), upper_lvl_el, 0xFFFFFFFFL, true);

  if (!l1)
    return nullptr;

  auto callbacks = find_ebml_callbacks(EBML_INFO(KaxSegment), EbmlId(*l1));
  if (!callbacks)
    callbacks = &EBML_CLASS_CALLBACK(KaxSegment);

  auto l2 = static_cast<EbmlElement *>(nullptr);
  try {
    l1->Read(*m_es.get(), EBML_INFO_CONTEXT(*callbacks), upper_lvl_el, l2, true);

  } catch (std::runtime_error &e) {
    mxdebug_if(m_debug_resync, boost::format("exception reading element data: %1%\n") % e.what());
    m_in.setFilePointer(l1->GetElementPosition() + 1);
    delete l1;
    return nullptr;
  }

  auto element_size = get_element_size(l1);
  if (m_debug_resync)
    mxinfo(boost::format("kax_file::read_one_element(): read element at %1% calculated size %2% stored size %3%\n")
           % l1->GetElementPosition() % element_size % (l1->IsFiniteSize() ? (boost::format("%1%") % l1->ElementSize()).str() : std::string("unknown")));
  m_in.setFilePointer(l1->GetElementPosition() + element_size, seek_beginning);

  return l1;
}

bool
kax_file_c::is_level1_element_id(vint_c id) const {
  auto &context = EBML_CLASS_CONTEXT(KaxSegment);
  for (int segment_idx = 0, end = EBML_CTX_SIZE(context); end > segment_idx; ++segment_idx)
    if (EBML_ID_VALUE(EBML_CTX_IDX_ID(context,segment_idx)) == id.m_value)
      return true;

  return false;
}

bool
kax_file_c::is_global_element_id(vint_c id) const {
  return (EBML_ID_VALUE(EBML_ID(EbmlVoid))  == id.m_value)
    ||   (EBML_ID_VALUE(EBML_ID(EbmlCrc32)) == id.m_value);
}

EbmlElement *
kax_file_c::resync_to_level1_element(uint32_t wanted_id) {
  try {
    return resync_to_level1_element_internal(wanted_id);
  } catch (...) {
    if (m_debug_resync)
      mxinfo("kax_file::resync_to_level1_element(): exception\n");
    return nullptr;
  }
}

EbmlElement *
kax_file_c::resync_to_level1_element_internal(uint32_t wanted_id) {
  if (m_segment_end && (m_in.getFilePointer() >= m_segment_end))
    return nullptr;

  m_resynced         = true;
  m_resync_start_pos = m_in.getFilePointer();

  auto actual_id     = m_in.read_uint32_be();
  auto start_time    = mtx::sys::get_current_time_millis();
  auto is_cluster_id = !wanted_id || (EBML_ID_VALUE(EBML_ID(KaxCluster)) == wanted_id); // 0 means: any level 1 element will do

  report(boost::format(Y("%1%: Error in the Matroska file structure at position %2%. Resyncing to the next level 1 element.\n"))
         % m_in.get_file_name() % m_resync_start_pos);

  if (is_cluster_id && (-1 != m_last_timestamp)) {
    report(boost::format(Y("The last timestamp processed before the error was encountered was %1%.\n")) % format_timestamp(m_last_timestamp));
    m_last_timestamp = -1;
  }

  if (m_debug_resync)
    mxinfo(boost::format("kax_file::resync_to_level1_element(): starting at %1% potential ID %|2$08x|\n") % m_resync_start_pos % actual_id);

  while (m_in.getFilePointer() < m_file_size) {
    auto now = mtx::sys::get_current_time_millis();
    if ((now - start_time) >= 10000) {
      report(boost::format("Still resyncing at position %1%.\n") % m_in.getFilePointer());
      start_time = now;
    }

    actual_id = (actual_id << 8) | m_in.read_uint8();

    if (   ((0 != wanted_id) && (wanted_id != actual_id))
        || ((0 == wanted_id) && !is_level1_element_id(vint_c(actual_id, 4))))
      continue;

    auto current_start_pos  = m_in.getFilePointer() - 4;
    auto element_pos        = current_start_pos;
    auto num_headers        = 1u;
    auto valid_unknown_size = false;

    if (m_debug_resync)
      mxinfo(boost::format("kax_file::resync_to_level1_element(): byte-for-byte search, found level 1 ID %|2$x| at %1%\n") % current_start_pos % actual_id);

    try {
      for (auto idx = 0; 3 > idx; ++idx) {
        auto length = vint_c::read(m_in);

        if (m_debug_resync)
          mxinfo(boost::format("kax_file::resync_to_level1_element():   read ebml length %1%/%2% valid? %3% unknown? %4%\n")
                 % length.m_value % length.m_coded_size % length.is_valid() % length.is_unknown());

        if (length.is_unknown()) {
          valid_unknown_size = true;
          break;
        }

        if (   !length.is_valid()
            || ((element_pos + length.m_value + length.m_coded_size + 2 * 4) >= m_file_size)
            || !m_in.setFilePointer2(element_pos + 4 + length.m_value + length.m_coded_size, seek_beginning))
          break;

        element_pos  = m_in.getFilePointer();
        auto next_id = m_in.read_uint32_be();

        if (m_debug_resync)
          mxinfo(boost::format("kax_file::resync_to_level1_element():   next ID is %|1$x| at %2%\n") % next_id % element_pos);

        if (   ((0 != wanted_id) && (wanted_id != next_id))
            || ((0 == wanted_id) && !is_level1_element_id(vint_c(next_id, 4))))
          break;

        ++num_headers;
      }
    } catch (...) {
    }

    if ((4 == num_headers) || valid_unknown_size) {
      report(boost::format(Y("Resyncing successful at position %1%.\n")) % current_start_pos);
      m_in.setFilePointer(current_start_pos, seek_beginning);
      return read_next_level1_element(wanted_id, is_cluster_id);
    }

    m_in.setFilePointer(current_start_pos + 4, seek_beginning);
  }

  report(Y("Resync failed: no valid Matroska level 1 element found.\n"));

  return nullptr;
}

KaxCluster *
kax_file_c::resync_to_cluster() {
  return static_cast<KaxCluster *>(resync_to_level1_element(EBML_ID_VALUE(EBML_ID(KaxCluster))));
}

KaxCluster *
kax_file_c::read_next_cluster() {
  return static_cast<KaxCluster *>(read_next_level1_element(EBML_ID_VALUE(EBML_ID(KaxCluster))));
}

bool
kax_file_c::was_resynced() const {
  return m_resynced;
}

int64_t
kax_file_c::get_resync_start_pos() const {
  return m_resync_start_pos;
}

unsigned long
kax_file_c::get_element_size(EbmlElement *e) {
  auto m = dynamic_cast<EbmlMaster *>(e);

  if (!m || e->IsFiniteSize())
    return e->GetSizeLength() + EBML_ID_LENGTH(static_cast<const EbmlId &>(*e)) + e->GetSize();

  auto max_end_pos = e->GetElementPosition() + EBML_ID_LENGTH(static_cast<const EbmlId &>(*e));
  for (int idx = 0, end = m->ListSize(); end > idx; ++idx)
    max_end_pos = std::max(max_end_pos, (*m)[idx]->GetElementPosition() + get_element_size((*m)[idx]));

  return max_end_pos - e->GetElementPosition();
}

void
kax_file_c::set_timestamp_scale(int64_t timestamp_scale) {
  m_timestamp_scale = timestamp_scale;
}

void
kax_file_c::set_last_timestamp(int64_t last_timestamp) {
  m_last_timestamp = last_timestamp;
}

void
kax_file_c::set_segment_end(EbmlElement const &segment) {
  m_segment_end = segment.IsFiniteSize() ? segment.GetElementPosition() + segment.HeadSize() + segment.GetSize() : m_in.get_size();
}

uint64_t
kax_file_c::get_segment_end()
  const {
  return m_segment_end;
}

void
kax_file_c::enable_reporting(bool enable) {
  m_reporting_enabled = enable;
}

void
kax_file_c::report(boost::format const &message) {
  if (m_reporting_enabled)
    mxwarn(message);
}

void
kax_file_c::report(std::string const &message) {
  if (m_reporting_enabled)
    mxwarn(message);
}
