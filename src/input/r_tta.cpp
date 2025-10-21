/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   TTA demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cmath>

#include "common/ape.h"
#include "common/codec.h"
#include "common/common_urls.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/id3.h"
#include "common/id_info.h"
#include "input/r_tta.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_tta.h"

namespace {
debugging_option_c s_debug{"tta_reader"};
}

bool
tta_reader_c::probe_file() {
  uint8_t buf[4];

  int tag_size = mtx::id3::skip_v2_tag(*m_in);
  if (-1 == tag_size)
    return false;

  return (m_in->read(buf, 4) == 4) && (memcmp(buf, "TTA1", 4) == 0);
}

void
tta_reader_c::read_headers() {
  try {
    int tag_size = mtx::id3::skip_v2_tag(*m_in);
    if (0 > tag_size)
      mxerror_fn(m_ti.m_fname, fmt::format(FY("tta_reader: tag_size < 0 in the c'tor. {0}\n"), BUGMSG));
    m_size -= tag_size;

    if (m_in->read(&header, sizeof(mtx::tta::file_header_t)) != sizeof(mtx::tta::file_header_t))
      mxerror_fn(m_ti.m_fname, Y("The file header is too short.\n"));

    if (g_identifying)
      return;

    uint64_t seek_sum  = m_in->getFilePointer() + 4 - tag_size;
    m_size            -= mtx::id3::tag_present_at_end(*m_in);
    m_size            -= mtx::ape::tag_present_at_end(*m_in);

    uint32_t seek_point;

    do {
      seek_point  = m_in->read_uint32_le();
      seek_sum   += seek_point + 4;
      seek_points.push_back(seek_point);
    } while (seek_sum < m_size);

    mxdebug_if(s_debug,
               fmt::format("tta: ch {0} bps {1} sr {2} dl {3} seek_sum {4} size {5} num {6}\n",
                           get_uint16_le(&header.channels),    get_uint16_le(&header.bits_per_sample),
                           get_uint32_le(&header.sample_rate), get_uint32_le(&header.data_length),
                           seek_sum, m_size, seek_points.size()));

    if (seek_sum != m_size)
      mxerror_fn(m_ti.m_fname, Y("The seek table in this TTA file seems to be broken.\n"));

    m_in->skip(4);

    pos = 0;

  } catch (...) {
    throw mtx::input::open_x();
  }
  show_demuxer_info();
}

void
tta_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty())
    return;

  add_packetizer(new tta_packetizer_c(this, m_ti, get_uint16_le(&header.channels), get_uint16_le(&header.bits_per_sample), get_uint32_le(&header.sample_rate)));
  show_packetizer_info(0, ptzr(0));
}

file_status_e
tta_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (seek_points.size() <= pos)
    return flush_packetizers();

  auto mem  = memory_c::alloc(seek_points[pos]);
  int nread = m_in->read(mem->get_buffer(), seek_points[pos]);

  if (nread != static_cast<int>(seek_points[pos]))
    return flush_packetizers();
  pos++;

  if (seek_points.size() <= pos) {
    double samples_left = (double)get_uint32_le(&header.data_length) - (seek_points.size() - 1) * mtx::tta::FRAME_TIME * get_uint32_le(&header.sample_rate);
    mxdebug_if(s_debug, fmt::format("tta: samples_left {0}\n", samples_left));

    ptzr(0).process(std::make_shared<packet_t>(mem, -1, std::llround(samples_left * 1000000000.0 / get_uint32_le(&header.sample_rate))));
  } else
    ptzr(0).process(std::make_shared<packet_t>(mem));

  return seek_points.size() <= pos ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

void
tta_reader_c::identify() {
  auto info = mtx::id::info_c{};

  info.add(mtx::id::audio_channels,           get_uint16_le(&header.channels));
  info.add(mtx::id::audio_sampling_frequency, get_uint32_le(&header.sample_rate));
  info.add(mtx::id::audio_bits_per_sample,    get_uint16_le(&header.bits_per_sample));

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, codec_c::get_name(codec_c::type_e::A_TTA, "TTA"), info.get());
}
