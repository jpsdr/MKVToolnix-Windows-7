/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   OGG media stream reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "common/codec.h"
#include "common/extern_data.h"
#include "common/flac.h"
#include "common/id_info.h"
#include "input/r_flac.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "merge/output_control.h"

#define BUFFER_SIZE 4096

#if defined(HAVE_FLAC_FORMAT_H)

bool
flac_reader_c::probe_file(mm_io_c *io,
                          uint64_t size) {
  if (4 > size)
    return false;

  std::string data;
  try {
    io->setFilePointer(0, seek_beginning);
    if (io->read(data, 4) != 4)
      return false;
    io->setFilePointer(0, seek_beginning);

    return data == "fLaC";
  } catch (...) {
    return false;
  }
}

flac_reader_c::flac_reader_c(const track_info_c &ti,
                             const mm_io_cptr &in)
  : generic_reader_c{ti, in}
  , decoder_c()                 // Don't use initializer-list syntax due to a bug in gcc < 4.8
{
}

void
flac_reader_c::read_headers() {
  if (!parse_file(g_identifying))
    throw mtx::input::header_parsing_x();

  if (g_identifying)
    return;

  show_demuxer_info();

  try {
    uint32_t block_size = 0;

    for (current_block = blocks.begin(); (current_block != blocks.end()) && (FLAC_BLOCK_TYPE_HEADERS == current_block->type); current_block++)
      block_size += current_block->len;

    m_header = memory_c::alloc(block_size);

    block_size         = 0;
    for (current_block = blocks.begin(); (current_block != blocks.end()) && (FLAC_BLOCK_TYPE_HEADERS == current_block->type); current_block++) {
      m_in->setFilePointer(current_block->filepos);
      if (m_in->read(m_header->get_buffer() + block_size, current_block->len) != current_block->len)
        mxerror(Y("flac_reader: Could not read a header packet.\n"));
      block_size += current_block->len;
    }

  } catch (mtx::exception &) {
    mxerror(Y("flac_reader: could not initialize the FLAC packetizer.\n"));
  }
}

flac_reader_c::~flac_reader_c() {
}

void
flac_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || (NPTZR() != 0))
    return;

  add_packetizer(new flac_packetizer_c(this, m_ti, m_header->get_buffer(), m_header->get_size()));
  show_packetizer_info(0, PTZR0);
}

bool
flac_reader_c::parse_file(bool for_identification_only) {
  FLAC__StreamDecoderState state;
  flac_block_t block;
  uint64_t u, old_pos;
  int result, progress, old_progress;
  bool ok;

  m_in->setFilePointer(0);
  metadata_parsed = false;

  if (!for_identification_only)
    mxinfo(Y("+-> Parsing the FLAC file. This can take a LONG time.\n"));

  init_flac_decoder();
  result = FLAC__stream_decoder_process_until_end_of_metadata(m_flac_decoder.get());

  mxdebug_if(m_debug, boost::format("flac_reader: extract->metadata, result: %1%, mdp: %2%, num blocks: %3%\n") % result % metadata_parsed % blocks.size());

  if (!metadata_parsed)
    mxerror_fn(m_ti.m_fname, Y("No metadata block found. This file is broken.\n"));

  if (for_identification_only)
    return true;

  FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &u);

  block.type    = FLAC_BLOCK_TYPE_HEADERS;
  block.filepos = 0;
  block.len     = u;
  old_pos       = u;

  blocks.push_back(block);

  mxdebug_if(m_debug, boost::format("flac_reader: headers: block at %1% with size %2%\n") % block.filepos % block.len);

  old_progress = -5;
  ok = FLAC__stream_decoder_skip_single_frame(m_flac_decoder.get());
  while (ok) {
    state = FLAC__stream_decoder_get_state(m_flac_decoder.get());

    progress = m_in->getFilePointer() * 100 / m_size;
    if ((progress - old_progress) >= 5) {
      mxinfo(boost::format(Y("+-> Pre-parsing FLAC file: %1%%%%2%")) % progress % "\r");
      old_progress = progress;
    }

    if (FLAC__stream_decoder_get_decode_position(m_flac_decoder.get(), &u) && (u != old_pos)) {
      block.type    = FLAC_BLOCK_TYPE_DATA;
      block.filepos = old_pos;
      block.len     = u - old_pos;
      old_pos       = u;
      blocks.push_back(block);

      mxdebug_if(m_debug, boost::format("flac_reader: skip/decode frame, block at %1% with size %2%\n") % block.filepos % block.len);
    }

    if (state > FLAC__STREAM_DECODER_READ_FRAME)
      break;

    ok = FLAC__stream_decoder_skip_single_frame(m_flac_decoder.get());
  }

  if (100 != old_progress)
    mxinfo(Y("+-> Pre-parsing FLAC file: 100%\n"));
  else
    mxinfo("\n");

  if ((blocks.size() == 0) || (blocks[0].type != FLAC_BLOCK_TYPE_HEADERS))
    mxerror(Y("flac_reader: Could not read all header packets.\n"));

  m_in->setFilePointer(0);
  blocks[0].len     -= 4;
  blocks[0].filepos  = 4;

  return metadata_parsed;
}

file_status_e
flac_reader_c::read(generic_packetizer_c *,
                    bool) {
  if (current_block == blocks.end())
    return flush_packetizers();

  memory_cptr buf = memory_c::alloc(current_block->len);
  m_in->setFilePointer(current_block->filepos);
  if (m_in->read(buf, current_block->len) != current_block->len)
    return flush_packetizers();

  unsigned int samples_here = mtx::flac::get_num_samples(buf->get_buffer(), current_block->len, stream_info);
  PTZR0->process(new packet_t(buf, samples * 1000000000 / sample_rate));

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
  bytes_read   = m_in->read(reinterpret_cast<unsigned char *>(buffer), wanted_bytes);
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

  mxdebug_if(m_debug, boost::format("flac_reader: STREAMINFO block (%1% bytes):\n") % metadata->length);
  mxdebug_if(m_debug, boost::format("flac_reader:   sample_rate: %1% Hz\n")         % metadata->data.stream_info.sample_rate);
  mxdebug_if(m_debug, boost::format("flac_reader:   channels: %1%\n")               % metadata->data.stream_info.channels);
  mxdebug_if(m_debug, boost::format("flac_reader:   bits_per_sample: %1%\n")        % metadata->data.stream_info.bits_per_sample);
}

std::string
flac_reader_c::attachment_name_from_metadata(FLAC__StreamMetadata_Picture const &picture)
  const {
  std::string mime_type = picture.mime_type;
  std::string name      = picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER                ? "other"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD   ? "icon"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON            ? "other icon"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER          ? "cover"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_BACK_COVER           ? "cover (back)"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_LEAFLET_PAGE         ? "leaflet page"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA                ? "media"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_LEAD_ARTIST          ? "lead artist - lead performer - soloist"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_ARTIST               ? "artist - performer"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_CONDUCTOR            ? "conductor"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_BAND                 ? "band - orchestra"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_COMPOSER             ? "composer"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_LYRICIST             ? "lyricist - text writer"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_RECORDING_LOCATION   ? "recording location"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_RECORDING     ? "during recording"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_PERFORMANCE   ? "during performance"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_VIDEO_SCREEN_CAPTURE ? "movie - video screen capture"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FISH                 ? "a bright coloured fish"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_ILLUSTRATION         ? "illustration"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_BAND_LOGOTYPE        ? "band - artist logotype"
                        : picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_PUBLISHER_LOGOTYPE   ? "publisher - Studio logotype"
                        :                                                                           "unknown";

  auto extension = primary_file_extension_for_mime_type(mime_type);

  if (!extension.empty())
    name += std::string{"."} + extension;

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

  mxdebug_if(m_debug, boost::format("flac_reader: PICTURE block\n"));
  mxdebug_if(m_debug, boost::format("flac_reader:   description: %1%\n") % attachment->description);
  mxdebug_if(m_debug, boost::format("flac_reader:   name:        %1%\n") % attachment->name);
  mxdebug_if(m_debug, boost::format("flac_reader:   MIME type:   %1%\n") % attachment->mime_type);
  mxdebug_if(m_debug, boost::format("flac_reader:   data length: %1%\n") % picture.data_length);
  mxdebug_if(m_debug, boost::format("flac_reader:   ID:          %1%\n") % m_attachment_id);
  mxdebug_if(m_debug, boost::format("flac_reader:   mode:        %1%\n") % attach_mode);

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
flac_reader_c::flac_metadata_cb(const FLAC__StreamMetadata *metadata) {
  switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:
      handle_stream_info_metadata(metadata);
      break;

    case FLAC__METADATA_TYPE_PICTURE:
      handle_picture_metadata(metadata);
      break;

    default:
      mxdebug_if(m_debug,
                 boost::format("%1% (%2%) block (%3% bytes)\n")
                 % (  metadata->type == FLAC__METADATA_TYPE_PADDING        ? "PADDING"
                    : metadata->type == FLAC__METADATA_TYPE_APPLICATION    ? "APPLICATION"
                    : metadata->type == FLAC__METADATA_TYPE_SEEKTABLE      ? "SEEKTABLE"
                    : metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT ? "VORBIS COMMENT"
                    : metadata->type == FLAC__METADATA_TYPE_CUESHEET       ? "CUESHEET"
                    :                                                        "UNDEFINED")
                 % metadata->type % metadata->length);
      break;
  }
}

void
flac_reader_c::flac_error_cb(FLAC__StreamDecoderErrorStatus status) {
  mxerror(boost::format(Y("flac_reader: Error parsing the file: %1%\n")) % static_cast<int>(status));
}

FLAC__StreamDecoderSeekStatus
flac_reader_c::flac_seek_cb(uint64_t new_pos) {
  m_in->setFilePointer(new_pos, seek_beginning);
  if (m_in->getFilePointer() == new_pos)
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
  return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}

FLAC__StreamDecoderTellStatus
flac_reader_c::flac_tell_cb(uint64_t &absolute_byte_offset) {
  absolute_byte_offset = m_in->getFilePointer();
  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus
flac_reader_c::flac_length_cb(uint64_t &stream_length) {
  stream_length = m_size;
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool
flac_reader_c::flac_eof_cb() {
  return m_in->getFilePointer() >= m_size;
}

void
flac_reader_c::identify() {
  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_bits_per_sample,    bits_per_sample);
  info.add(mtx::id::audio_channels,           channels);
  info.add(mtx::id::audio_sampling_frequency, sample_rate);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, "FLAC", info.get());
  for (auto &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description, attachment->id);
}

#else  // HAVE_FLAC_FORMAT_H

bool
flac_reader_c::probe_file(mm_io_c *in,
                          uint64_t size) {
  if (4 > size)
    return false;

  std::string data;
  try {
    in->setFilePointer(0, seek_beginning);
    if (in->read(data, 4) != 4)
      return false;
    in->setFilePointer(0, seek_beginning);

    if (data == "fLaC")
      id_result_container_unsupported(in->get_file_name(), "FLAC");
  } catch (...) {
  }
  return false;
}

#endif // HAVE_FLAC_FORMAT_H
