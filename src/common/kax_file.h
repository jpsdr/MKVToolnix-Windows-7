/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   high level Matroska file reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxSegment.h>
#include <matroska/KaxCluster.h>

#include "common/vint.h"

using namespace libebml;
using namespace libmatroska;

class kax_file_c {
protected:
  mm_io_c &m_in;
  bool m_resynced, m_reporting_enabled{true};
  uint64_t m_resync_start_pos, m_file_size, m_segment_end;
  int64_t m_timestamp_scale, m_last_timestamp;
  std::shared_ptr<EbmlStream> m_es;

  debugging_option_c m_debug_read_next, m_debug_resync;

public:
  kax_file_c(mm_io_c &in);
  virtual ~kax_file_c();

  virtual bool was_resynced() const;
  virtual int64_t get_resync_start_pos() const;
  virtual bool is_level1_element_id(vint_c id) const;
  virtual bool is_global_element_id(vint_c id) const;

  virtual EbmlElement *read_next_level1_element(uint32_t wanted_id = 0, bool report_cluster_timestamp = false);
  virtual KaxCluster *read_next_cluster();

  virtual EbmlElement *resync_to_level1_element(uint32_t wanted_id = 0);
  virtual KaxCluster *resync_to_cluster();

  static unsigned long get_element_size(EbmlElement *e);

  virtual void set_timestamp_scale(int64_t timestamp_scale);
  virtual void set_last_timestamp(int64_t last_timestamp);
  virtual void set_segment_end(EbmlElement const &segment);
  virtual uint64_t get_segment_end() const;

  virtual void enable_reporting(bool enable);

protected:
  virtual EbmlElement *read_one_element();

  virtual EbmlElement *read_next_level1_element_internal(uint32_t wanted_id = 0);
  virtual EbmlElement *resync_to_level1_element_internal(uint32_t wanted_id = 0);

  virtual void report(boost::format const &message);
  virtual void report(std::string const &message);
};
using kax_file_cptr = std::shared_ptr<kax_file_c>;
