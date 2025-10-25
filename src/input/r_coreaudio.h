/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the CoreAudio reader module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/mm_io.h"
#include "common/error.h"
#include "common/samples_to_timestamp_converter.h"

#include "merge/generic_reader.h"

struct coreaudio_chunk_t {
  std::string m_type;
  uint64_t m_position, m_data_position, m_size;
};
using coreaudio_chunk_itr = std::vector<coreaudio_chunk_t>::iterator;

struct coreaudio_packet_t {
  uint64_t m_position, m_size, m_duration, m_timestamp;
};
using coreaudio_packet_itr = std::vector<coreaudio_packet_t>::iterator;

class coreaudio_reader_c: public generic_reader_c {
private:
  memory_cptr m_magic_cookie;

  std::vector<coreaudio_chunk_t> m_chunks;
  std::vector<coreaudio_packet_t> m_packets;

  coreaudio_packet_itr m_current_packet;

  codec_c m_codec;
  std::string m_codec_name;
  bool m_supported{};

  double m_sample_rate{};
  unsigned int m_flags{}, m_bytes_per_packet{}, m_frames_per_packet{}, m_channels{}, m_bits_per_sample{};

  samples_to_timestamp_converter_c m_frames_to_timestamp;

  debugging_option_c
      m_debug_headers{"coreaudio_reader|coreaudio_reader_headers"}
    , m_debug_chunks{ "coreaudio_reader|coreaudio_reader_chunks"}
    , m_debug_packets{"coreaudio_reader|coreaudio_reader_packets"};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::coreaudio;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t tid);

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  void scan_chunks();
  coreaudio_chunk_itr find_chunk(std::string const &type, bool throw_on_error, coreaudio_chunk_itr start);

  coreaudio_chunk_itr
  find_chunk(std::string const &type, bool throw_on_error = false) {
    return find_chunk(type, throw_on_error, m_chunks.begin());
  }

  memory_cptr read_chunk(std::string const &type, bool throw_on_error = true);

  void parse_desc_chunk();
  void parse_pakt_chunk();
  void parse_kuki_chunk();

  void handle_alac_magic_cookie(memory_cptr const &chunk);

  generic_packetizer_c *create_alac_packetizer();

  void debug_error_and_throw(std::string const &message) const;
  void dump_headers() const;
};
