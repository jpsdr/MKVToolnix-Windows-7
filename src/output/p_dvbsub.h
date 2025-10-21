/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the DVB subtitles output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"

class dvbsub_packetizer_c: public generic_packetizer_c {
public:
  dvbsub_packetizer_c(generic_reader_c *reader, track_info_c &ti, memory_cptr const &private_data);
  virtual ~dvbsub_packetizer_c();

  virtual void set_headers() override;

  virtual translatable_string_c get_format_name() const override {
    return YT("DVB subtitles");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
