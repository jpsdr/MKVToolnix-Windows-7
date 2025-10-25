/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   FLAC helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/format.h>
#include <FLAC/stream_decoder.h>

namespace mtx::flac {

constexpr auto HEADER_STREAM_INFO     = 1;
constexpr auto HEADER_VORBIS_COMMENTS = 2;
constexpr auto HEADER_CUESHEET        = 4;
constexpr auto HEADER_APPLICATION     = 8;
constexpr auto HEADER_SEEKTABLE       = 16;

constexpr auto BLOCK_TYPE_HEADERS     = 0;
constexpr auto BLOCK_TYPE_DATA        = 1;

class decoder_deleter_c {
public:
  void operator()(FLAC__StreamDecoder *decoder) {
    if (decoder) {
      FLAC__stream_decoder_reset(decoder);
      FLAC__stream_decoder_delete(decoder);
    }
  }
};
using stream_decoder_uptr = std::unique_ptr<FLAC__StreamDecoder, decoder_deleter_c>;

class decoder_c {
protected:
  stream_decoder_uptr m_flac_decoder;

protected:
  decoder_c() = default;
  virtual ~decoder_c() = default;

public:
  virtual void flac_metadata_cb(const FLAC__StreamMetadata *metadata);
  virtual void flac_error_cb(FLAC__StreamDecoderErrorStatus status);
  virtual FLAC__StreamDecoderSeekStatus flac_seek_cb(uint64_t new_pos);
  virtual FLAC__StreamDecoderTellStatus flac_tell_cb(uint64_t &absolute_byte_offset);
  virtual FLAC__StreamDecoderLengthStatus flac_length_cb(uint64_t &stream_length);
  virtual FLAC__bool flac_eof_cb();
  virtual FLAC__StreamDecoderReadStatus flac_read_cb(FLAC__byte buffer[], size_t *bytes);
  virtual FLAC__StreamDecoderWriteStatus flac_write_cb(const FLAC__Frame *frame, const FLAC__int32 * const data[]);
  virtual void init_flac_decoder();
};

int get_num_samples(uint8_t const *buf, int size, FLAC__StreamMetadata_StreamInfo const &stream_info);
int decode_headers(uint8_t const *mem, int size, int num_elements, ...);

std::string file_base_name_for_picture_type(unsigned int type);

}                              // namespace mtx::flac

#endif // HAVE_FLAC_FORMAT_H
