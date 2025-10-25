/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the TrueHD output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/truehd.h"
#include "merge/generic_packetizer.h"
#include "merge/stream_property_preserver.h"
#include "merge/timestamp_calculator.h"

class truehd_packetizer_c: public generic_packetizer_c {
protected:
  bool m_first_frame, m_remove_dialog_normalization_gain;
  mtx::truehd::frame_t m_first_truehd_header;

  int64_t m_current_samples_per_frame, m_ref_timestamp;
  timestamp_calculator_c m_timestamp_calculator;
  stream_property_preserver_c<timestamp_c> m_discard_padding;
  mtx::truehd::parser_c m_parser;

public:
  truehd_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, mtx::truehd::frame_t::codec_e codec, int sampling_rate, int channels);
  virtual ~truehd_packetizer_c();

  virtual void process_framed(mtx::truehd::frame_cptr const &frame, std::optional<int64_t> provided_timestamp = {});
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("TrueHD");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void adjust_header_values(mtx::truehd::frame_cptr const &frame);

  virtual void flush_impl();
  virtual void flush_frames();
};
