/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   high level Matroska file reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxSegment.h>
#include <matroska/KaxCluster.h>

#include "common/vint.h"

class kax_file_c {
protected:
  mm_io_c &m_in;
  bool m_resynced, m_reporting_enabled{true};
  uint64_t m_resync_start_pos, m_file_size, m_segment_end;
  int64_t m_timestamp_scale, m_last_timestamp;
  std::shared_ptr<libebml::EbmlStream> m_es;

  debugging_option_c m_debug_read_next, m_debug_resync;

public:
  kax_file_c(mm_io_c &in);
  virtual ~kax_file_c() = default;

  virtual bool was_resynced() const;
  virtual int64_t get_resync_start_pos() const;

  virtual std::shared_ptr<libebml::EbmlElement> read_next_level1_element(uint32_t wanted_id = 0, bool report_cluster_timestamp = false);
  virtual std::shared_ptr<libmatroska::KaxCluster> read_next_cluster();

  virtual std::shared_ptr<libebml::EbmlElement> resync_to_level1_element(uint32_t wanted_id = 0);
  virtual std::shared_ptr<libmatroska::KaxCluster> resync_to_cluster();

  virtual void set_timestamp_scale(int64_t timestamp_scale);
  virtual void set_last_timestamp(int64_t last_timestamp);
  virtual void set_segment_end(libmatroska::KaxSegment const &segment);
  virtual uint64_t get_segment_end() const;

  virtual void enable_reporting(bool enable);

protected:
  virtual std::shared_ptr<libebml::EbmlElement> read_one_element();

  virtual std::shared_ptr<libebml::EbmlElement> read_next_level1_element_internal(uint32_t wanted_id = 0);
  virtual std::shared_ptr<libebml::EbmlElement> resync_to_level1_element_internal(uint32_t wanted_id = 0);

  virtual void report(std::string const &message);

public:
  static bool is_level1_element_id(vint_c id);
  static bool is_global_element_id(vint_c id);
  static unsigned long get_element_size(libebml::EbmlElement &e);
};
using kax_file_cptr = std::shared_ptr<kax_file_c>;
