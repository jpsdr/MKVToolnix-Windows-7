/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the WAV reader module

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modified by Peter Niemayer <niemayer@isg.de>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/dts.h"
#include "common/error.h"
#include "common/mm_io.h"
#include "merge/generic_reader.h"

class wav_reader_c;
struct wave_header;

class wav_demuxer_c {
public:
  wav_reader_c         *m_reader;
  wave_header          *m_wheader;
  generic_packetizer_c *m_ptzr;
  track_info_c         &m_ti;
  codec_c               m_codec;

public:
  wav_demuxer_c(wav_reader_c *reader, wave_header *wheader);
  virtual ~wav_demuxer_c() {};

  virtual int64_t get_preferred_input_size() = 0;
  virtual unsigned char *get_buffer() = 0;
  virtual void process(int64_t len) = 0;

  virtual generic_packetizer_c *create_packetizer() = 0;

  virtual bool probe(mm_io_cptr &io) = 0;

  virtual unsigned int get_channels() const = 0;
  virtual unsigned int get_sampling_frequency() const = 0;
  virtual unsigned int get_bits_per_sample() const = 0;
};

using wav_demuxer_cptr = std::shared_ptr<wav_demuxer_c>;

struct wav_chunk_t {
  uint64_t pos, len;
  memory_cptr id;
};

struct wave64_chunk_t {
  unsigned char guid[16];
  uint64_t size;
};

struct wave64_header_t {
  wave64_chunk_t riff;
  unsigned char wave_guid[16];
};

class wav_reader_c: public generic_reader_c {
public:
  enum class type_e {
    unknown,
    wave,
    wave64,
  };

private:
  type_e m_type;
  struct wave_header m_wheader;
  int64_t m_bytes_in_data_chunks, m_remaining_bytes_in_current_data_chunk;

  std::vector<wav_chunk_t> m_chunks;
  boost::optional<std::size_t> m_cur_data_chunk_idx;

  wav_demuxer_cptr m_demuxer;

  uint32_t m_format_tag;

public:
  wav_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~wav_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::wav;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t tid);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  static int probe_file(mm_io_c *in, uint64_t size);

protected:
  static type_e determine_type(mm_io_c &in, uint64_t size);

  boost::optional<std::size_t> find_chunk(const char *id, int start_idx = 0, bool allow_empty = true);

  void scan_chunks();
  void scan_chunks_wave();
  void scan_chunks_wave64();

  void dump_headers();

  void parse_file();
  void parse_fmt_chunk();

  void create_demuxer();
};
