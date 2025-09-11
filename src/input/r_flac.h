/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the raw FLAC stream reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/export.h>
#include <FLAC/stream_decoder.h>

#include "common/bcp47.h"
#include "common/debugging.h"
#include "common/flac.h"
#include "common/tags/vorbis.h"
#include "matroska/KaxSemantic.h"
#include "merge/generic_reader.h"
#include "output/p_flac.h"

struct flac_block_t {
  uint64_t filepos;
  unsigned int type, len;
};

class flac_reader_c: public generic_reader_c, public mtx::flac::decoder_c {
private:
  memory_cptr m_header;
  int sample_rate{}, channels{}, bits_per_sample{};
  bool metadata_parsed{};
  uint64_t samples{};
  std::vector<flac_block_t> blocks;
  std::vector<flac_block_t>::iterator current_block;
  FLAC__StreamMetadata_StreamInfo stream_info;
  unsigned int m_attachment_id{};
  debugging_option_c m_debug{"flac_reader|flac"};
  int64_t tag_size_start{}, tag_size_end{};
  bool current_frame_broken{};

  std::vector<flac_block_t> m_metadata_block_info;

  mtx::bcp47::language_c m_language;
  std::shared_ptr<libmatroska::KaxTags> m_tags;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::flac;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t id);
  virtual bool is_providing_timestamps() const {
    return false;
  }

  virtual bool probe_file() override;

  virtual FLAC__StreamDecoderReadStatus flac_read_cb(FLAC__byte buffer[], size_t *bytes);
  virtual void flac_metadata_cb(const FLAC__StreamMetadata *metadata);
  virtual void flac_error_cb(FLAC__StreamDecoderErrorStatus status);
  virtual FLAC__StreamDecoderSeekStatus flac_seek_cb(uint64_t new_pos);
  virtual FLAC__StreamDecoderTellStatus flac_tell_cb(uint64_t &absolute_byte_offset);
  virtual FLAC__StreamDecoderLengthStatus flac_length_cb(uint64_t &stream_length);
  virtual FLAC__bool flac_eof_cb();

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual bool parse_file(bool for_identification_only);
  virtual void handle_picture_metadata(FLAC__StreamMetadata const *metadata);
  virtual void handle_stream_info_metadata(FLAC__StreamMetadata const *metadata);
  virtual void handle_vorbis_comment_metadata(FLAC__StreamMetadata const *metadata);
  virtual std::string attachment_name_from_metadata(FLAC__StreamMetadata_Picture const &picture) const;
  virtual void handle_language_and_title(mtx::tags::converted_vorbis_comments_t const &converted);
};
#else  // HAVE_FLAC_FORMAT_H

class flac_reader_c {
public:
  static void probe_file(mm_io_c &in);
};

#endif // HAVE_FLAC_FORMAT_H
