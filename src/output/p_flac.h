/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the Flac packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/format.h>

#include "merge/generic_packetizer.h"

class flac_packetizer_c: public generic_packetizer_c {
private:
  memory_cptr m_header;
  FLAC__StreamMetadata_StreamInfo m_stream_info;
  int64_t m_num_packets{};

public:
  flac_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, uint8_t const *header, std::size_t l_header);
  virtual ~flac_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("FLAC");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
  virtual split_result_e can_be_split(std::string &error_message) override;

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};

#endif  // HAVE_FLAC_STREAM_DECODER_H
