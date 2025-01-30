/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the  HEVC ES video output module

*/

#pragma once

#include "common/common_pch.h"

#include "common/hevc/es_parser.h"
#include "merge/generic_packetizer.h"

class xyzvc_es_video_packetizer_c: public generic_packetizer_c {
protected:
  std::unique_ptr<mtx::xyzvc::es_parser_c> m_parser_base;

  int64_t m_default_duration_for_interlaced_content{-1};
  std::optional<int64_t> m_parser_default_duration_to_force, m_framed_nalu_size;
  bool m_first_frame{true}, m_set_display_dimensions{false};
  uint32_t m_width{}, m_height{};
  debugging_option_c m_debug_timestamps, m_debug_aspect_ratio;

public:
  xyzvc_es_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, std::string const &p_debug_type, std::unique_ptr<mtx::xyzvc::es_parser_c> &&parser_base, uint32_t width, uint32_t height);

  virtual void set_headers() override;

  virtual void add_extra_data(memory_cptr const &data);
  virtual void set_container_default_field_duration(int64_t default_duration);
  virtual void set_is_framed(unsigned int nalu_size);
  virtual unsigned int get_nalu_size_length() const;

  virtual void connect(generic_packetizer_c *src, int64_t p_append_timestamp_offset) override;

protected:
  virtual void process_impl(packet_cptr const &packet) override;

  virtual void flush_impl() override;
  virtual void flush_frames();

  virtual void check_if_default_duration_available() const;

  virtual void handle_delayed_headers();
  virtual void handle_aspect_ratio();
  virtual void handle_actual_default_duration();
};
