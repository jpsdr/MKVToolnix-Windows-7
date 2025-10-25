/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the Vorbis packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "merge/generic_packetizer.h"

namespace mtx {
  namespace output {
    class vorbis_x: public exception {
    protected:
      std::string m_message;
    public:
      vorbis_x(const std::string &message) : m_message{message} { }
      virtual ~vorbis_x() throw() { }

      virtual const char *what() const throw() {
        return m_message.c_str();
      }
    };
  }
}

class vorbis_packetizer_c: public generic_packetizer_c {
private:
  int64_t m_previous_bs{}, m_samples{}, m_previous_samples_sum{}, m_previous_timestamp{}, m_timestamp_offset{};
  std::vector<memory_cptr> m_headers;
  vorbis_info m_vi;
  vorbis_comment m_vc;

public:
  vorbis_packetizer_c(generic_reader_c *reader, track_info_c &ti, std::vector<memory_cptr> const &headers);
  virtual ~vorbis_packetizer_c();

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("Vorbis");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

  virtual bool is_compatible_with(output_compatibility_e compatibility);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
