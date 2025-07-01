/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   OGG media stream reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "input/r_flac.h"
#include "merge/id_result.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/format.h>
#include <FLAC/ordinals.h>
#include <FLAC/stream_decoder.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "common/at_scope_exit.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/flac.h"
#include "common/id_info.h"
#include "common/id3.h"
#include "common/list_utils.h"
#include "common/mime.h"
#include "common/tags/tags.h"
#include "common/tags/vorbis.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "merge/output_control.h"

bool
flac_reader_c::probe_file() {
  std::string data;
  mtx::id3::skip_v2_tag(*m_in);
  return (m_in->read(data, 4) == 4) && (data == "fLaC");
}

void
flac_reader_c::read_headers() {
  if (!parse_file(g_identifying))
    throw mtx::input::header_parsing_x();

  if (g_identifying)
    return;

  show_demuxer_info();

  try {
    m_in->setFilePointer(tag_size_start);
    m_header = m_in->read(4); // "fLaC";

    std::vector<flac_block_t> to_keep;

    for (auto const &info : m_metadata_block_info)
      if (!mtx::included_in(info.type, FLAC__METADATA_TYPE_PICTURE, FLAC__METADATA_TYPE_PADDING))
        to_keep.emplace_back(info);

    m_metadata_block_info = to_keep;

    auto info_num = 0u;
    for (auto const &info : m_metadata_block_info) {
      ++info_num;

      m_in->setFilePointer(info.filepos + tag_size_start);
      auto block = m_in->read(info.len);

      if (block->get_size() == 0)
        continue;

      auto buffer = block->get_buffer();

      if (info_num == m_metadata_block_info.size())
        buffer[0] = buffer[0] | 0x80; // set "last metadata block" flag
      else
        buffer[0] = buffer[0] & 0x7f; // clear "last metadata block" flag

      m_header->add(*block);
    }

  } catch (mtx::exception &) {
    mxerror(Y("flac_reader: could not initialize the FLAC packetizer.\n"));
  }
}

void
flac_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty())
    return;

  m_ti.m_language = m_language;

  if (m_tags && demuxing_requested('T', 0))
    m_ti.m_tags = clone(m_tags);

  add_packetizer(new flac_packetizer_c(this, m_ti, m_header->get_buffer(), m_header->get_size()));
  show_packetizer_info(0, ptzr(0));
}

bool
flac_reader_c::parse_file(bool for_identification_only) {
  FLAC__StreamDecoderState state;
  flac_block_t block;
  uint64_t u, old_pos;
  int result, progress, old_progress;
  bool ok;

  m_in->setFilePointer(0);

  tag_size_start  = std::max<int64_t>(mtx::id3::skip_v2_tag(*m_in),        0);
  tag_size_end    = std::max<int64_t>(mtx::id3::tag_present_at_end(*m_in), 0);
  metadata_parsed = false;

  mxdebug_if(m_debug, fmt::format("flac_reader: tag_size_start {0} tag_size_end {1} total size {2} size without tags {3}\n", tag_size_start, tag_size_end, m_size, m_size - tag_size_start - tag_size_end));

  if (!for_identification_only)
    mxinfo(Y("+-> Parsing the FLAC file. This can take a LONG time.\n"));

  init_flac_decoder();
  result = FLAC__stream_decoder_process_until_end_of_metadata(m_flac_decoder.get());

  mxdebug_if(m_debug, fmt::format("flac_reader: extract->metadata, result: {0}, mdp: {1}, num blocks: {2}\n", result, metadata_parsed, blocks.size()));

  if (!metadata_parsed)
    mxerror_fn(m_ti.m_fname, Y("No metadata block found. This file is broken.\n"));

  if (for_identification_only)
    return true;

  FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &u);

  mxdebug_if(m_debug, fmt::format("flac_reader: headers: block at 0 with size {0}\n", u));

  old_pos              = u;
  old_progress         = -5;
  current_frame_broken = false;
  ok                   = FLAC__stream_decoder_skip_single_frame(m_flac_decoder.get());

  while (ok) {
    state = FLAC__stream_decoder_get_state(m_flac_decoder.get());

    progress = m_in->getFilePointer() * 100 / (m_size - tag_size_end);
    if ((progress - old_progress) >= 5) {
      mxinfo(fmt::format(FY("+-> Pre-parsing FLAC file: {0}%{1}"), progress, "\r"));
      old_progress = progress;
    }

    if (FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &u) && (u != old_pos)) {
      block.type    = mtx::flac::BLOCK_TYPE_DATA;
      block.filepos = old_pos;
      block.len     = u - old_pos;
      old_pos       = u;
      blocks.push_back(block);

      mxdebug_if(m_debug, fmt::format("flac_reader: skip/decode frame, block at {0} with size {1}\n", block.filepos, block.len));
    }

    if (state > FLAC__STREAM_DECODER_READ_FRAME)
      break;

    current_frame_broken = false;
    ok                   = FLAC__stream_decoder_skip_single_frame(m_flac_decoder.get());
    ok                   = ok && !current_frame_broken;
  }

  if (100 != old_progress)
    mxinfo(Y("+-> Pre-parsing FLAC file: 100%\n"));
  else
    mxinfo("\n");

  if (m_metadata_block_info.size() == 0)
    mxerror(Y("flac_reader: Could not read all header packets.\n"));

  current_block = blocks.begin();

  return metadata_parsed;
}

file_status_e
flac_reader_c::read(generic_packetizer_c *,
                    bool) {
  if (current_block == blocks.end())
    return flush_packetizers();

  memory_cptr buf = memory_c::alloc(current_block->len);
  m_in->setFilePointer(current_block->filepos + tag_size_start);
  if (m_in->read(buf, current_block->len) != current_block->len)
    return flush_packetizers();

  unsigned int samples_here = mtx::flac::get_num_samples(buf->get_buffer(), current_block->len, stream_info);
  ptzr(0).process(std::make_shared<packet_t>(buf, samples * 1000000000 / sample_rate));

  samples += samples_here;
  current_block++;

  return (current_block == blocks.end()) ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

FLAC__StreamDecoderReadStatus
flac_reader_c::flac_read_cb(FLAC__byte buffer[],
                            size_t *bytes)
{
  unsigned bytes_read, wanted_bytes;

  wanted_bytes = *bytes;
  bytes_read   = m_in->read(reinterpret_cast<uint8_t *>(buffer), wanted_bytes);
  *bytes       = bytes_read;

  return bytes_read == wanted_bytes ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

void
flac_reader_c::handle_stream_info_metadata(FLAC__StreamMetadata const *metadata) {
  memcpy(&stream_info, &metadata->data.stream_info, sizeof(FLAC__StreamMetadata_StreamInfo));
  sample_rate     = metadata->data.stream_info.sample_rate;
  channels        = metadata->data.stream_info.channels;
  bits_per_sample = metadata->data.stream_info.bits_per_sample;
  metadata_parsed = true;

  mxdebug_if(m_debug, fmt::format("flac_reader: STREAMINFO block ({0} bytes):\n", metadata->length));
  mxdebug_if(m_debug, fmt::format("flac_reader:   sample_rate: {0} Hz\n",         metadata->data.stream_info.sample_rate));
  mxdebug_if(m_debug, fmt::format("flac_reader:   channels: {0}\n",               metadata->data.stream_info.channels));
  mxdebug_if(m_debug, fmt::format("flac_reader:   bits_per_sample: {0}\n",        metadata->data.stream_info.bits_per_sample));
}

std::string
flac_reader_c::attachment_name_from_metadata(FLAC__StreamMetadata_Picture const &picture)
  const {
  auto mime_type = picture.mime_type;
  auto name      = mtx::flac::file_base_name_for_picture_type(picture.type);
  auto extension = mtx::mime::primary_file_extension_for_type(mime_type);

  if (!extension.empty())
    name += "."s + extension;

  return name;
}

void
flac_reader_c::handle_picture_metadata(FLAC__StreamMetadata const *metadata) {
  auto &picture = metadata->data.picture;

  if (!picture.data || !picture.data_length || !picture.mime_type)
    return;

  auto attachment         = std::make_shared<attachment_t>();
  attachment->description = std::string{ picture.description ? reinterpret_cast<char const *>(picture.description) : "" };
  attachment->mime_type   = picture.mime_type;
  attachment->name        = attachment_name_from_metadata(picture);
  attachment->data        = memory_c::clone(picture.data, picture.data_length);
  attachment->ui_id       = m_attachment_id;

  auto attach_mode        = attachment_requested(m_attachment_id);

  mxdebug_if(m_debug, fmt::format("flac_reader: PICTURE block\n"));
  mxdebug_if(m_debug, fmt::format("flac_reader:   description: {0}\n", attachment->description));
  mxdebug_if(m_debug, fmt::format("flac_reader:   name:        {0}\n", attachment->name));
  mxdebug_if(m_debug, fmt::format("flac_reader:   MIME type:   {0}\n", attachment->mime_type));
  mxdebug_if(m_debug, fmt::format("flac_reader:   data length: {0}\n", picture.data_length));
  mxdebug_if(m_debug, fmt::format("flac_reader:   ID:          {0}\n", m_attachment_id));
  mxdebug_if(m_debug, fmt::format("flac_reader:   mode:        {0}\n", static_cast<unsigned int>(attach_mode)));

  if (attachment->mime_type.empty() || attachment->name.empty())
    return;

  ++m_attachment_id;

  if (ATTACH_MODE_SKIP == attach_mode)
    return;

  attachment->to_all_files   = ATTACH_MODE_TO_ALL_FILES == attach_mode;
  attachment->source_file    = m_ti.m_fname;

  add_attachment(attachment);
}

void
flac_reader_c::handle_vorbis_comment_metadata(FLAC__StreamMetadata const *metadata) {
  if (metadata->length == 0)
    return;

  m_in->save_pos();
  mtx::at_scope_exit_c restore_pos([this]() { m_in->restore_pos(); });

  FLAC__uint64 decode_position = 0;
  FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &decode_position);

  auto start_pos = tag_size_start + decode_position - metadata->length;
  m_in->setFilePointer(start_pos);

  auto raw_data  = m_in->read(metadata->length);
  auto comments  = mtx::tags::parse_vorbis_comments_from_packet(*raw_data, 0);
  auto converted = mtx::tags::from_vorbis_comments(comments);

  if (m_debug) {
    mxdebug(fmt::format("flac_reader: comments: title {0} language {1} num pictures {2} (unsupported at the moment)\n",
                        !converted.m_title.empty()      ? converted.m_title             : "<no title>"s,
                        converted.m_language.is_valid() ? converted.m_language.format() : "<no language>"s,
                        converted.m_pictures.size()));

    mxdebug(fmt::format("flac_reader: comments: track tags:\n"));
    dump_ebml_elements(converted.m_track_tags.get());

    mxdebug(fmt::format("flac_reader: comments: album tags:\n"));
    dump_ebml_elements(converted.m_album_tags.get());
  }

  handle_language_and_title(converted);

  m_tags = mtx::tags::merge(converted.m_track_tags, converted.m_album_tags);
}

void
flac_reader_c::handle_language_and_title(mtx::tags::converted_vorbis_comments_t const &converted) {
  if (converted.m_language.is_valid())
    m_language = converted.m_language;

  if (converted.m_title.empty())
    return;

  m_title = converted.m_title;
  maybe_set_segment_title(m_title);
}

void
flac_reader_c::flac_metadata_cb(const FLAC__StreamMetadata *metadata) {
  FLAC__uint64 new_decode_position = 0;
  FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &new_decode_position);

  auto position = new_decode_position - metadata->length - FLAC__STREAM_METADATA_HEADER_LENGTH;

  m_metadata_block_info.emplace_back(flac_block_t{ position, metadata->type, metadata->length + FLAC__STREAM_METADATA_HEADER_LENGTH });

  mxdebug_if(m_debug,
             fmt::format("flac_reader: metadata type {0} ({1}) size {2} at {3}\n",
                           metadata->type == FLAC__METADATA_TYPE_STREAMINFO     ? "stream info"
                         : metadata->type == FLAC__METADATA_TYPE_PICTURE        ? "picture"
                         : metadata->type == FLAC__METADATA_TYPE_PADDING        ? "padding"
                         : metadata->type == FLAC__METADATA_TYPE_APPLICATION    ? "application"
                         : metadata->type == FLAC__METADATA_TYPE_SEEKTABLE      ? "seektable"
                         : metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT ? "vorbis comment"
                         : metadata->type == FLAC__METADATA_TYPE_CUESHEET       ? "cuesheet"
                         :                                                        "undefined",
                         static_cast<unsigned int>(metadata->type), metadata->length + FLAC__STREAM_METADATA_HEADER_LENGTH, position));

  switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:
      handle_stream_info_metadata(metadata);
      break;

    case FLAC__METADATA_TYPE_PICTURE:
      handle_picture_metadata(metadata);
      break;

    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
      handle_vorbis_comment_metadata(metadata);
      break;

    default:
      break;
  }
}

void
flac_reader_c::flac_error_cb(FLAC__StreamDecoderErrorStatus status) {
  auto type = status == FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC          ? "FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC"
            : status == FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER         ? "FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER"
            : status == FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH ? "FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH"
            : status == FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM ? "FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM"
            :                                                                  "<unknown>";

  mxdebug_if(m_debug, fmt::format("flac_reader: error callback: {0} ({1})\n", static_cast<int>(status), type));
  current_frame_broken = true;
}

FLAC__StreamDecoderSeekStatus
flac_reader_c::flac_seek_cb(uint64_t new_pos) {
  new_pos += tag_size_start;

  m_in->setFilePointer(new_pos);
  if (m_in->getFilePointer() == new_pos)
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
  return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}

FLAC__StreamDecoderTellStatus
flac_reader_c::flac_tell_cb(uint64_t &absolute_byte_offset) {
  absolute_byte_offset = std::max<int64_t>(m_in->getFilePointer(), tag_size_start) - tag_size_start;
  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus
flac_reader_c::flac_length_cb(uint64_t &stream_length) {
  stream_length = m_size - tag_size_end - tag_size_start;
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool
flac_reader_c::flac_eof_cb() {
  return m_in->getFilePointer() >= (m_size - tag_size_end);
}

void
flac_reader_c::identify() {
  auto container_info = mtx::id::info_c{};
  container_info.add(mtx::id::title, m_title);

  auto info = mtx::id::info_c{};
  info.add(mtx::id::track_name,               m_title);
  info.add(mtx::id::audio_bits_per_sample,    bits_per_sample);
  info.add(mtx::id::audio_channels,           channels);
  info.add(mtx::id::audio_sampling_frequency, sample_rate);

  if (m_language.is_valid())
    info.add(mtx::id::language, m_language.format());

  if (m_tags)
    add_track_tags_to_identification(*m_tags, info);

  id_result_container(container_info.get());
  id_result_track(0, ID_RESULT_TRACK_AUDIO, "FLAC", info.get());
  for (auto &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description, attachment->id);
  if (m_tags)
    id_result_tags(0, mtx::tags::count_simple(*m_tags));
}

#else  // HAVE_FLAC_FORMAT_H

void
flac_reader_c::probe_file(mm_io_c &in) {
  std::string data;
  if ((in.read(data, 4) == 4) && (data == "fLaC"))
    id_result_container_unsupported(in.get_file_name(), "FLAC");
}

#endif // HAVE_FLAC_FORMAT_H
