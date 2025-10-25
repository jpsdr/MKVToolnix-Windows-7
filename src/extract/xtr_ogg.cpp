/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <vorbis/codec.h>

#include "common/ebml.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/kate.h"
#include "common/opus.h"
#include "common/random.h"
#include "common/version.h"
#include "extract/xtr_ogg.h"

// ------------------------------------------------------------------------

xtr_flac_c::xtr_flac_c(const std::string &_codec_id,
                       int64_t _tid,
                       track_spec_t &tspec)
  : xtr_base_c(_codec_id, _tid, tspec)
{
}

void
xtr_flac_c::create_file(xtr_base_c *_master,
                        libmatroska::KaxTrackEntry &track) {
  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  xtr_base_c::create_file(_master, track);

  memory_cptr mpriv = decode_codec_private(priv);
  m_out->write(mpriv);
}

// ------------------------------------------------------------------------

xtr_oggbase_c::xtr_oggbase_c(const std::string &codec_id,
                             int64_t tid,
                             track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_packetno(2)
  , m_queued_granulepos(-1)
  , m_previous_end(0)
{
}

void
xtr_oggbase_c::create_file(xtr_base_c *master,
                           libmatroska::KaxTrackEntry &track) {
  m_sfreq = (int)kt_get_a_sfreq(track);

  xtr_base_c::create_file(master, track);

  ogg_stream_init(&m_os, mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA) ? 1804289383 : random_c::generate_32bits() & 0x7ffff'ffff);
}

void
xtr_oggbase_c::create_standard_file(xtr_base_c *master,
                                    libmatroska::KaxTrackEntry &track,
                                    libmatroska::LacingType lacing) {
  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  init_content_decoder(track);
  memory_cptr mpriv = decode_codec_private(priv);

  std::vector<memory_cptr> header_packets;

  try {
    if (lacing == libmatroska::LACING_NONE)
      header_packets.push_back(mpriv);

    else {
      header_packets = unlace_memory_xiph(mpriv);

      if (header_packets.empty())
        throw false;
    }

    header_packets_unlaced(header_packets);

  } catch (...) {
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' does not contain valid headers.\n"), m_tid, m_codec_id));
  }

  xtr_oggbase_c::create_file(master, track);

  ogg_packet op;
  for (m_packetno = 0; header_packets.size() > m_packetno; ++m_packetno) {
    // Handle all the header packets: ID header, comments, etc
    op.b_o_s      = (0 == m_packetno ? 1 : 0);
    op.e_o_s      = 0;
    op.packetno   = m_packetno;
    op.packet     = header_packets[m_packetno]->get_buffer();
    op.bytes      = header_packets[m_packetno]->get_size();
    op.granulepos = 0;
    ogg_stream_packetin(&m_os, &op);

    if (0 == m_packetno) /* ID header must be alone on a separate page */
      flush_pages();
  }

  /* flush at last header, data must start on a new page */
  flush_pages();
}

void
xtr_oggbase_c::header_packets_unlaced(std::vector<memory_cptr> &) {
}

void
xtr_oggbase_c::handle_frame(xtr_frame_t &f) {
  if (-1 != m_queued_granulepos)
    m_queued_granulepos = f.timestamp * m_sfreq / 1000000000;

  queue_frame(f.frame, 0);

  m_previous_end = f.timestamp + f.duration;
}

xtr_oggbase_c::~xtr_oggbase_c() {
  ogg_stream_clear(&m_os);
}

void
xtr_oggbase_c::finish_file() {
  if (-1 == m_queued_granulepos)
    return;

  // Set the "end of stream" marker on the last packet, handle it
  // and flush all remaining Ogg pages.
  m_queued_granulepos = m_previous_end * m_sfreq / 1000000000;
  write_queued_frame(true);
  flush_pages();
}

void
xtr_oggbase_c::flush_pages() {
  ogg_page page;

  while (ogg_stream_flush(&m_os, &page)) {
    m_out->write(page.header, page.header_len);
    m_out->write(page.body,   page.body_len);
  }
}

void
xtr_oggbase_c::write_pages() {
  ogg_page page;

  while (ogg_stream_pageout(&m_os, &page)) {
    m_out->write(page.header, page.header_len);
    m_out->write(page.body,   page.body_len);
  }
}

void
xtr_oggbase_c::queue_frame(memory_cptr &frame,
                           int64_t granulepos) {
  if (-1 != m_queued_granulepos)
    write_queued_frame(false);

  m_queued_granulepos = granulepos;
  m_queued_frame      = frame;
  m_queued_frame->take_ownership();
}

void
xtr_oggbase_c::write_queued_frame(bool eos) {
  if (-1 == m_queued_granulepos)
    return;

  ogg_packet op;

  op.b_o_s      = 0;
  op.e_o_s      = eos ? 1 : 0;
  op.packetno   = m_packetno;
  op.packet     = m_queued_frame->get_buffer();
  op.bytes      = m_queued_frame->get_size();
  op.granulepos = m_queued_granulepos;

  ++m_packetno;

  ogg_stream_packetin(&m_os, &op);
  write_pages();

  m_queued_granulepos = -1;
  m_queued_frame.reset();
}

// ------------------------------------------------------------------------

xtr_oggvorbis_c::xtr_oggvorbis_c(const std::string &codec_id,
                                 int64_t tid,
                                 track_spec_t &tspec)
  : xtr_oggbase_c(codec_id, tid, tspec)
  , m_previous_block_size(-1)
  , m_samples(0)
{
  vorbis_info_init(&m_vorbis_info);
  vorbis_comment_init(&m_vorbis_comment);
}

xtr_oggvorbis_c::~xtr_oggvorbis_c() {
  vorbis_info_clear(&m_vorbis_info);
  vorbis_comment_clear(&m_vorbis_comment);
}

void
xtr_oggvorbis_c::create_file(xtr_base_c *master,
                             libmatroska::KaxTrackEntry &track) {
  create_standard_file(master, track, libmatroska::LACING_AUTO);
}

void
xtr_oggvorbis_c::header_packets_unlaced(std::vector<memory_cptr> &header_packets) {
  int packet_num = 0;
  for (auto &packet : header_packets) {
    ogg_packet op;
    op.packet   = packet->get_buffer();
    op.bytes    = packet->get_size();
    op.b_o_s    = 0 == packet_num;
    op.packetno = packet_num;
    ++packet_num;

    vorbis_synthesis_headerin(&m_vorbis_info, &m_vorbis_comment, &op);
  }
}


void
xtr_oggvorbis_c::handle_frame(xtr_frame_t &f) {
  ogg_packet op;
  op.packet               = f.frame->get_buffer();
  op.bytes                = f.frame->get_size();
  int64_t this_block_size = vorbis_packet_blocksize(&m_vorbis_info, &op);

  if (-1 != m_previous_block_size) {
    m_queued_granulepos   = m_samples;
    m_samples            += (this_block_size + m_previous_block_size) / 4;
  }

  m_previous_end        = m_samples * 1000000000 / m_sfreq;
  m_previous_block_size = this_block_size;

  queue_frame(f.frame, 0);
}

// ------------------------------------------------------------------------

xtr_oggkate_c::xtr_oggkate_c(const std::string &codec_id,
                             int64_t tid,
                             track_spec_t &tspec)
  : xtr_oggbase_c(codec_id, tid, tspec)
{
}

void
xtr_oggkate_c::create_file(xtr_base_c *master,
                           libmatroska::KaxTrackEntry &track) {
  create_standard_file(master, track, libmatroska::LACING_AUTO);
}

void
xtr_oggkate_c::header_packets_unlaced(std::vector<memory_cptr> &header_packets) {
  mtx::kate::parse_identification_header(header_packets[0]->get_buffer(), header_packets[0]->get_size(), m_kate_id_header);
  if (m_kate_id_header.nheaders != header_packets.size())
    throw false;
}

void
xtr_oggkate_c::handle_frame(xtr_frame_t &f) {
  ogg_packet op;
  op.b_o_s    = 0;
  op.e_o_s    = (f.frame->get_size() == 1) && (f.frame->get_buffer()[0] == 0x7f);
  op.packetno = m_packetno;
  op.packet   = f.frame->get_buffer();
  op.bytes    = f.frame->get_size();

  /* we encode the backlink in the granulepos */
  auto f_timestamp   = f.timestamp / 1000000000.0;
  int64_t g_backlink = 0;

  if (op.bytes >= static_cast<long>(1 + 3 * sizeof(int64_t)))
    g_backlink = get_uint64_le(op.packet + 1 + 2 * sizeof(int64_t));

  auto f_backlink  = g_backlink * static_cast<double>(m_kate_id_header.gden) / m_kate_id_header.gnum;
  auto f_base      = f_timestamp - f_backlink;
  auto f_offset    = f_timestamp - f_base;
  int64_t g_base   = (int64_t)(f_base   * m_kate_id_header.gnum / m_kate_id_header.gden);
  int64_t g_offset = (int64_t)(f_offset * m_kate_id_header.gnum / m_kate_id_header.gden);
  op.granulepos    = (g_base << m_kate_id_header.kfgshift) | g_offset;

  ogg_stream_packetin(&m_os, &op);
  flush_pages(); /* Kate is a data packet per page */

  ++m_packetno;
}

// ------------------------------------------------------------------------

xtr_oggtheora_c::xtr_oggtheora_c(const std::string &codec_id,
                                 int64_t tid,
                                 track_spec_t &tspec)
  : xtr_oggbase_c(codec_id, tid, tspec)
  , m_keyframe_number(0)
  , m_non_keyframe_number(-1)
{
}

void
xtr_oggtheora_c::create_file(xtr_base_c *master,
                             libmatroska::KaxTrackEntry &track) {
  create_standard_file(master, track, libmatroska::LACING_AUTO);
}

void
xtr_oggtheora_c::header_packets_unlaced(std::vector<memory_cptr> &header_packets) {
  mtx::theora::parse_identification_header(header_packets[0]->get_buffer(), header_packets[0]->get_size(), m_theora_header);
}

void
xtr_oggtheora_c::handle_frame(xtr_frame_t &f) {
  if (f.frame->get_size() && (0x00 == (f.frame->get_buffer()[0] & 0x40))) {
    m_keyframe_number     += m_non_keyframe_number + 1;
    m_non_keyframe_number  = 0;

  } else
    m_non_keyframe_number += 1;

  queue_frame(f.frame, (m_keyframe_number << m_theora_header.kfgshift) | (m_non_keyframe_number & ((1 << m_theora_header.kfgshift) - 1)));
}

void
xtr_oggtheora_c::finish_file() {
  write_queued_frame(true);
  flush_pages();
}

// ------------------------------------------------------------------------

xtr_oggopus_c::xtr_oggopus_c(const std::string &codec_id,
                             int64_t tid,
                             track_spec_t &tspec)
  : xtr_oggbase_c{codec_id, tid, tspec}
  , m_position{timestamp_c::ns(0)}
{
  m_debug = debugging_c::requested("opus|opus_extrator");
}

void
xtr_oggopus_c::create_file(xtr_base_c *master,
                           libmatroska::KaxTrackEntry &track) {
  create_standard_file(master, track, libmatroska::LACING_NONE);
}

void
xtr_oggopus_c::header_packets_unlaced(std::vector<memory_cptr> &header_packets) {
  auto signature = "OpusTags"s;
  auto version   = "unknown encoder; extracted from Matroska with "s + (!mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA) ? get_version_info("mkvextract") : "mkvextract"s);
  auto ver_len   = version.length();
  auto mem       = memory_c::alloc(8 + 4 + ver_len + 4);
  auto buffer    = reinterpret_cast<char *>(mem->get_buffer());

  signature.copy(buffer,                      8);
  put_uint32_le(buffer + 8,                   ver_len);
  version.copy(buffer + 8 + 4,                ver_len);
  put_uint32_le(buffer + mem->get_size() - 4, 0);

  header_packets.push_back(mem);
}

void
xtr_oggopus_c::handle_frame(xtr_frame_t &f) {
  try {
    auto toc = mtx::opus::toc_t::decode(f.frame);
    mxdebug_if(m_debug, fmt::format("Position: {0} discard_duration: {1} TOC: {2}\n", m_position, f.discard_duration, toc));

    m_position = m_position + toc.packet_duration - f.discard_duration;
    queue_frame(f.frame, m_position.to_samples(48000));

  } catch (mtx::opus::exception &ex) {
    mxdebug_if(m_debug, fmt::format("Exception: {0}\n", ex.what()));
  }
}

void
xtr_oggopus_c::finish_file() {
  write_queued_frame(true);
  flush_pages();
}
