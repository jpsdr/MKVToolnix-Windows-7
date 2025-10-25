/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   FLAC packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "common/checksums/base.h"
#include "common/codec.h"
#include "common/flac.h"
#include "common/hacks.h"
#include "merge/connection_checks.h"
#include "output/p_flac.h"

flac_packetizer_c::flac_packetizer_c(generic_reader_c *p_reader,
                                     track_info_c &p_ti,
                                     uint8_t const *header,
                                     std::size_t l_header)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_header{memory_c::clone(header, l_header)}
{
  int result;

  if ((4 > l_header) || memcmp(header, "fLaC", 4)) {
    m_header->resize(l_header + 4);
    memcpy(m_header->get_buffer(),     "fLaC", 4);
    memcpy(m_header->get_buffer() + 4, header, l_header);
  }


  result = mtx::flac::decode_headers(m_header->get_buffer(), m_header->get_size(), 1, mtx::flac::HEADER_STREAM_INFO, &m_stream_info);
  if (!(result & mtx::flac::HEADER_STREAM_INFO))
    mxerror_tid(m_ti.m_fname, m_ti.m_id, Y("The FLAC headers could not be parsed: the stream info structure was not found.\n"));

  if (m_stream_info.min_blocksize == m_stream_info.max_blocksize)
    set_track_default_duration((int64_t)(1000000000ll * m_stream_info.min_blocksize / m_stream_info.sample_rate));
}

flac_packetizer_c::~flac_packetizer_c() {
}

void
flac_packetizer_c::set_headers() {
  set_codec_id(MKV_A_FLAC);
  set_codec_private(m_header);
  set_audio_sampling_freq(m_stream_info.sample_rate);
  set_audio_channels(m_stream_info.channels);
  set_audio_bit_depth(m_stream_info.bits_per_sample);

  generic_packetizer_c::set_headers();
}

void
flac_packetizer_c::process_impl(packet_cptr const &packet) {
  m_num_packets++;

  packet->duration = mtx::flac::get_num_samples(packet->data->get_buffer(), packet->data->get_size(), m_stream_info);

  if (-1 == packet->duration) {
    mxwarn_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Packet number {0} contained an invalid FLAC header and is being skipped.\n"), m_num_packets));
    return;
  }

  packet->duration = packet->duration * 1000000000ll / m_stream_info.sample_rate;
  add_packet(packet);
}

connection_result_e
flac_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                  std::string &error_message) {
  if (!mtx::hacks::is_engaged(mtx::hacks::APPEND_AND_SPLIT_FLAC))
    return CAN_CONNECT_NO_UNSUPPORTED;

  flac_packetizer_c *fsrc = dynamic_cast<flac_packetizer_c *>(src);
  if (!fsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_stream_info.sample_rate, fsrc->m_stream_info.sample_rate);
  connect_check_a_channels(m_stream_info.channels, fsrc->m_stream_info.channels);
  connect_check_a_bitdepth(m_stream_info.bits_per_sample, fsrc->m_stream_info.bits_per_sample);

  return CAN_CONNECT_YES;
}

split_result_e
flac_packetizer_c::can_be_split(std::string &/* error_message */) {
  if (mtx::hacks::is_engaged(mtx::hacks::APPEND_AND_SPLIT_FLAC)) {
    return CAN_SPLIT_YES;
  }

  return CAN_SPLIT_NO_UNSUPPORTED;
}

#endif
