/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   OGG media stream reader -- FLAC support

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/stream_decoder.h>

#include "common/debugging.h"
#include "common/mm_file_io.h"
#include "output/p_flac.h"
#include "input/r_ogm.h"
#include "input/r_ogm_flac.h"

namespace {
debugging_option_c s_debug{"ogg_flac"};

constexpr auto BUFFER_SIZE = 4096;
}

static FLAC__StreamDecoderReadStatus
fhe_read_cb(const FLAC__StreamDecoder *,
            FLAC__byte buffer[],
            size_t *bytes,
            void *client_data) {
  ogg_packet op;

  flac_header_extractor_c *fhe = (flac_header_extractor_c *)client_data;
  if (fhe->done)
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

  if (ogg_stream_packetout(&fhe->os, &op) != 1)
    if (!fhe->read_page() || (ogg_stream_packetout(&fhe->os, &op) != 1))
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

  if (*bytes < static_cast<size_t>(op.bytes))
    mxerror(fmt::format(FY("flac_header_extraction: bytes ({0}) < op.bytes ({1}). Could not read the FLAC headers.\n"), *bytes, op.bytes));

  int offset = 0;

  if ((0 == fhe->num_packets) && (ofm_post_1_1_1 == fhe->mode) && (13 < op.bytes))
    offset = 9;

  memcpy(buffer, &op.packet[offset], op.bytes - offset);
  *bytes = op.bytes - offset;

  fhe->num_packets++;
  mxdebug_if(s_debug, fmt::format("flac_header_extraction: read packet number {0} with {1} bytes and offset {2}\n", fhe->num_packets, op.bytes, offset));

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus
fhe_write_cb(const FLAC__StreamDecoder *,
             const FLAC__Frame *,
             const FLAC__int32 * const [],
             void *client_data) {
  mxdebug_if(s_debug, "flac_header_extraction: write cb\n");

  ((flac_header_extractor_c *)client_data)->done = true;
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
fhe_metadata_cb(const FLAC__StreamDecoder *,
                const FLAC__StreamMetadata *metadata,
                void *client_data) {

  flac_header_extractor_c *fhe = (flac_header_extractor_c *)client_data;
  fhe->num_header_packets      = fhe->num_packets;

  mxdebug_if(s_debug, "flac_header_extraction: metadata cb\n");

  switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:
      fhe->sample_rate     = metadata->data.stream_info.sample_rate;
      fhe->channels        = metadata->data.stream_info.channels;
      fhe->bits_per_sample = metadata->data.stream_info.bits_per_sample;
      fhe->metadata_parsed = true;

      mxdebug_if(s_debug, fmt::format("flac_header_extraction: STREAMINFO block ({0} bytes):\n", metadata->length));
      mxdebug_if(s_debug, fmt::format("flac_header_extraction:   sample_rate: {0} Hz\n",         metadata->data.stream_info.sample_rate));
      mxdebug_if(s_debug, fmt::format("flac_header_extraction:   channels: {0}\n",               metadata->data.stream_info.channels));
      mxdebug_if(s_debug, fmt::format("flac_header_extraction:   bits_per_sample: {0}\n",        metadata->data.stream_info.bits_per_sample));
      break;

    default:
      mxdebug_if(s_debug,
                 fmt::format("{0} ({1}) block ({2} bytes)\n",
                               metadata->type == FLAC__METADATA_TYPE_PADDING        ? "PADDING"
                             : metadata->type == FLAC__METADATA_TYPE_APPLICATION    ? "APPLICATION"
                             : metadata->type == FLAC__METADATA_TYPE_SEEKTABLE      ? "SEEKTABLE"
                             : metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT ? "VORBIS COMMENT"
                             : metadata->type == FLAC__METADATA_TYPE_CUESHEET       ? "CUESHEET"
                             :                                                        "UNDEFINED",
                             static_cast<unsigned int>(metadata->type), metadata->length));
      break;
  }
}

static void
fhe_error_cb(const FLAC__StreamDecoder *,
             FLAC__StreamDecoderErrorStatus status,
             void *client_data) {
  ((flac_header_extractor_c *)client_data)->done = true;
  mxdebug_if(s_debug, fmt::format("flac_header_extraction: error ({0})\n", static_cast<int>(status)));
}

flac_header_extractor_c::flac_header_extractor_c(const std::string &file_name,
                                                 int64_t p_sid,
                                                 oggflac_mode_e p_mode)
  : metadata_parsed(false)
  , sid(p_sid)
  , num_packets(0)
  , num_header_packets(0)
  , done(false)
  , mode(p_mode)
{
  file    = new mm_file_io_c(file_name);
  decoder = FLAC__stream_decoder_new();

  if (!decoder)
    mxerror(Y("flac_header_extraction: FLAC__stream_decoder_new() failed.\n"));
  if (!FLAC__stream_decoder_set_metadata_respond_all(decoder))
    mxerror(Y("flac_header_extraction: Could not set metadata_respond_all.\n"));
  if (FLAC__stream_decoder_init_stream(decoder, fhe_read_cb, nullptr, nullptr, nullptr, nullptr, fhe_write_cb, fhe_metadata_cb, fhe_error_cb, this) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    mxerror(Y("flac_header_extraction: Could not initialize the FLAC decoder.\n"));

  ogg_sync_init(&oy);
}

flac_header_extractor_c::~flac_header_extractor_c() {
  FLAC__stream_decoder_reset(decoder);
  FLAC__stream_decoder_delete(decoder);

  ogg_sync_clear(&oy);
  ogg_stream_clear(&os);

  delete file;
}

bool
flac_header_extractor_c::extract() {
  mxdebug_if(s_debug, "flac_header_extraction: extract\n");
  if (!read_page()) {
    mxdebug_if(s_debug, "flac_header_extraction: read_page() failed.\n");
    return false;
  }

  int result = (int)FLAC__stream_decoder_process_until_end_of_stream(decoder);

  mxdebug_if(s_debug, fmt::format("flac_header_extraction: extract, result: {0}, mdp: {1}, num_header_packets: {2}\n", result, metadata_parsed, num_header_packets));

  return metadata_parsed;
}

bool
flac_header_extractor_c::read_page() {
  while (1) {
    int np = ogg_sync_pageseek(&oy, &og);

    if (np <= 0) {
      if (np < 0)
        return false;

      uint8_t *buf = (uint8_t *)ogg_sync_buffer(&oy, BUFFER_SIZE);
      if (!buf)
        return false;

      int nread;
      if ((nread = file->read(buf, BUFFER_SIZE)) <= 0)
        return false;

      ogg_sync_wrote(&oy, nread);

    } else if (ogg_page_serialno(&og) == sid)
      break;
  }

  if (ogg_page_bos(&og))
    ogg_stream_init(&os, sid);
  ogg_stream_pagein(&os, &og);

  return true;
}

// ------------------------------------------

ogm_a_flac_demuxer_c::ogm_a_flac_demuxer_c(ogm_reader_c *p_reader,
                                           oggflac_mode_e p_mode)
  : ogm_demuxer_c(p_reader)
  , flac_header_packets(0)
  , mode(p_mode)
{
  codec = codec_c::look_up(codec_c::type_e::A_FLAC);
}

void
ogm_a_flac_demuxer_c::process_page(int64_t granulepos) {
  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    units_processed++;
    if (units_processed <= flac_header_packets)
      continue;

    for (int i = 0; i < (int)nh_packet_data.size(); i++)
      reader->m_reader_packetizers[ptzr]->process(std::make_shared<packet_t>(nh_packet_data[i]->clone(), 0));

    nh_packet_data.clear();

    if (-1 == last_granulepos)
      reader->m_reader_packetizers[ptzr]->process(std::make_shared<packet_t>(memory_c::borrow(op.packet, op.bytes), -1));
    else {
      reader->m_reader_packetizers[ptzr]->process(std::make_shared<packet_t>(memory_c::borrow(op.packet, op.bytes), last_granulepos * 1000000000 / sample_rate));
      last_granulepos = granulepos;
    }
  }
}

void
ogm_a_flac_demuxer_c::process_header_page() {
  ogg_packet op;

  while ((packet_data.size() < flac_header_packets) && (ogg_stream_packetout(&os, &op) == 1)) {
    eos |= op.e_o_s;
    packet_data.push_back(memory_c::clone(op.packet, op.bytes));
  }

  if (packet_data.size() >= flac_header_packets)
    headers_read = true;

  while (ogg_stream_packetout(&os, &op) == 1)
    nh_packet_data.push_back(memory_c::clone(op.packet, op.bytes));
}

void
ogm_a_flac_demuxer_c::initialize() {
  flac_header_extractor_c fhe(reader->m_ti.m_fname, serialno, mode);

  if (!fhe.extract())
    mxerror_tid(reader->m_ti.m_fname, track_id, Y("Could not read the FLAC header packets.\n"));

  flac_header_packets = fhe.num_header_packets;
  sample_rate         = fhe.sample_rate;
  channels            = fhe.channels;
  bits_per_sample     = fhe.bits_per_sample;
  last_granulepos     = 0;
  units_processed     = 1;

  if ((ofm_post_1_1_1 == mode) && !packet_data.empty() && (13 < packet_data.front()->get_size()))
    packet_data.front()->set_offset(9);
}

generic_packetizer_c *
ogm_a_flac_demuxer_c::create_packetizer() {
  int size = 0, start_at_header = ofm_post_1_1_1 == mode ? 0 : 1;
  int i;

  for (i = start_at_header; i < (int)packet_data.size(); i++)
    size += packet_data[i]->get_size();

  auto af_buf = memory_c::alloc(size);
  auto buf    = af_buf->get_buffer();
  size        = 0;

  for (i = start_at_header; i < (int)packet_data.size(); i++) {
    memcpy(&buf[size], packet_data[i]->get_buffer(), packet_data[i]->get_size());
    size += packet_data[i]->get_size();
  }

  generic_packetizer_c *ptzr_obj = new flac_packetizer_c(reader, m_ti, buf, size);

  reader->show_packetizer_info(m_ti.m_id, *ptzr_obj);

  return ptzr_obj;
}

#endif // HAVE_FLAC_FORMAT_H
