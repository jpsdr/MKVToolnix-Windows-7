/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   MPEG4 part 2 video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/math.h"
#include "common/strings/formatting.h"
#include "merge/generic_reader.h"
#include "merge/output_control.h"
#include "output/p_mpeg4_p2.h"

mpeg4_p2_video_packetizer_c::
mpeg4_p2_video_packetizer_c(generic_reader_c *p_reader,
                            track_info_c &p_ti,
                            double fps,
                            int width,
                            int height,
                            bool input_is_native)
  : video_for_windows_packetizer_c(p_reader, p_ti, fps, width, height)
  , m_timestamps_generated(0)
  , m_previous_timestamp(0)
  , m_aspect_ratio_extracted(false)
  , m_input_is_native(input_is_native)
  , m_output_is_native(hack_engaged(ENGAGE_NATIVE_MPEG4) || input_is_native)
  , m_size_extracted(false)
{
  if (!m_output_is_native)
    m_timestamp_factory_application_mode = TFA_SHORT_QUEUEING;

  else {
    set_codec_id(MKV_V_MPEG4_ASP);
    if (!m_input_is_native)
      m_ti.m_private_data.reset();

    // If no external timestamp file has been specified then mkvmerge
    // might have created a factory due to the --default-duration
    // command line argument. This factory must be disabled for this
    // packetizer because it takes care of handling the default
    // duration/FPS itself.
    if (m_ti.m_ext_timestamps.empty())
      m_timestamp_factory.reset();

    if (m_default_duration_forced)
      m_fps = 1000000000.0 / m_htrack_default_duration;

    else if (0.0 != m_fps)
      m_htrack_default_duration = static_cast<int64_t>(1000000000ll / m_fps);

    m_timestamp_factory_application_mode = TFA_FULL_QUEUEING;
  }
}

mpeg4_p2_video_packetizer_c::~mpeg4_p2_video_packetizer_c() {
  if (!debugging_c::requested("mpeg4_p2_statistics"))
    return;

  mxinfo(boost::format("mpeg4_p2_video_packetizer_c statistics:\n"
                       "  # I frames:            %1%\n"
                       "  # P frames:            %2%\n"
                       "  # B frames:            %3%\n"
                       "  # NVOPs:               %4%\n"
                       "  # generated timestamps: %5%\n"
                       "  # dropped timestamps:   %6%\n")
         % m_statistics.m_num_i_frames % m_statistics.m_num_p_frames % m_statistics.m_num_b_frames % m_statistics.m_num_n_vops
         % m_statistics.m_num_generated_timestamps % m_statistics.m_num_dropped_timestamps);
}

int
mpeg4_p2_video_packetizer_c::process(packet_cptr packet) {
  extract_size(packet->data->get_buffer(), packet->data->get_size());
  extract_aspect_ratio(packet->data->get_buffer(), packet->data->get_size());

  int result = m_input_is_native == m_output_is_native ? video_for_windows_packetizer_c::process(packet)
             : m_input_is_native                       ?                     process_native(packet)
             :                                                               process_non_native(packet);

  ++m_frames_output;

  return result;
}

int
mpeg4_p2_video_packetizer_c::process_non_native(packet_cptr packet) {
  extract_config_data(packet);

  // Add a timestamp and a duration if they've been given.
  if (-1 != packet->timestamp) {
    if (!m_default_duration_forced)
      m_available_timestamps.push_back(timestamp_duration_t(packet->timestamp, packet->duration));

    else {
      m_available_timestamps.push_back(timestamp_duration_t(m_timestamps_generated * m_htrack_default_duration, m_htrack_default_duration));
      ++m_timestamps_generated;
    }

  } else if (0.0 == m_fps)
    mxerror_tid(m_ti.m_fname, m_ti.m_id, Y("Cannot convert non-native MPEG4 video frames into native ones if the source container "
                                       "provides neither timestamps nor a number of frames per second.\n"));

  std::vector<video_frame_t> frames;
  mpeg4::p2::find_frame_types(packet->data->get_buffer(), packet->data->get_size(), frames, m_config_data);

  for (auto &frame : frames) {
    if (!frame.is_coded) {
      ++m_statistics.m_num_n_vops;

      int num_surplus_timestamps = static_cast<int>(m_available_timestamps.size()) - static_cast<int>(m_ref_frames.size() + m_b_frames.size());
      if (0 < num_surplus_timestamps) {
        std::deque<timestamp_duration_t>::iterator start = m_available_timestamps.begin() + m_ref_frames.size() + m_b_frames.size();
        std::deque<timestamp_duration_t>::iterator end   = start + num_surplus_timestamps;

        if (0 != (m_ref_frames.size() + m_b_frames.size())) {
          std::deque<timestamp_duration_t>::iterator last = m_available_timestamps.begin() + m_ref_frames.size() + m_b_frames.size() - 1;
          std::deque<timestamp_duration_t>::iterator cur  = start;
          while (cur != end) {
            last->m_duration = std::max(last->m_duration, static_cast<int64_t>(0)) + std::max(cur->m_duration, static_cast<int64_t>(0));
            ++cur;
          }
        }

        m_available_timestamps.erase(start, end);
        m_statistics.m_num_dropped_timestamps += num_surplus_timestamps;
      }

      continue;
    }

    if (FRAME_TYPE_I == frame.type)
      ++m_statistics.m_num_i_frames;
    else if (FRAME_TYPE_P == frame.type)
      ++m_statistics.m_num_p_frames;
    else
      ++m_statistics.m_num_b_frames;

    // Maybe we can flush queued frames now. But only if we don't have
    // a B frame.
    if (FRAME_TYPE_B != frame.type)
      flush_frames(false);

    frame.data      = (unsigned char *)safememdup(packet->data->get_buffer() + frame.pos, frame.size);
    frame.timestamp = -1;

    if (FRAME_TYPE_B == frame.type)
      m_b_frames.push_back(frame);
    else
      m_ref_frames.push_back(frame);
  }

  m_previous_timestamp = m_available_timestamps.back().m_timestamp;

  return FILE_STATUS_MOREDATA;
}

void
mpeg4_p2_video_packetizer_c::extract_config_data(packet_cptr &packet) {
  if (m_ti.m_private_data)
    return;

  m_ti.m_private_data = memory_cptr{mpeg4::p2::parse_config_data(packet->data->get_buffer(), packet->data->get_size(), m_config_data)};
  if (!m_ti.m_private_data)
    mxerror_tid(m_ti.m_fname, m_ti.m_id, Y("Could not find the codec configuration data in the first MPEG-4 part 2 video frame. This track cannot be stored in native mode.\n"));

  fix_codec_string();
  set_codec_private(m_ti.m_private_data);
  rerender_track_headers();
}

void
mpeg4_p2_video_packetizer_c::fix_codec_string() {
  static const unsigned char start_code[4] = {0x00, 0x00, 0x01, 0xb2};

  if (!m_ti.m_private_data || (0 == m_ti.m_private_data->get_size()))
    return;

  auto private_data = m_ti.m_private_data->get_buffer();
  int size = m_ti.m_private_data->get_size();
  int i;
  for (i = 0; 9 < size;) {
    if (memcmp(&private_data[i], start_code, 4) != 0) {
      ++i;
      --size;
      continue;
    }

    i    += 8;
    size -= 8;
    if (strncasecmp((const char *)&private_data[i - 4], "divx", 4) != 0)
      continue;

    unsigned char *end_pos = (unsigned char *)memchr(&private_data[i], 0, size);
    if (!end_pos)
      end_pos = &private_data[i + size];

    --end_pos;
    if ('p' == *end_pos)
      *end_pos = 'n';

    return;
  }
}

int
mpeg4_p2_video_packetizer_c::process_native(packet_cptr) {
  // Not implemented yet.
  return FILE_STATUS_MOREDATA;
}

/** \brief Handle frame sequences in which too few timestamps are available

   This function gets called if mkvmerge wants to flush its frame queue
   but it doesn't have enough timestamps and/or durations available for
   each queued frame. This can happen in two cases:

   1. A picture sequence is found that mkvmerge does not support. An
      example: Two frames have been read. The first contained a
      P and a B frame (that's OK so far), but the second one contained
      another P or I frame without an intermediate dummy frame.

   2. The end of the file has been reached but the frame queue contains
      more frames than the timestamp queue. For example: The last frame
      contained two frames, a P and a B frame. Right afterwards the
      end of the file is reached. In this case a dummy frame is missing.

   Both cases can be solved if the source file provides a FPS for this
   track. The other case is not supported.
*/
void
mpeg4_p2_video_packetizer_c::generate_timestamp_and_duration() {
  if (0.0 >= m_fps) {
    // TODO: error
    mxexit(1);
  }

  if (m_available_timestamps.empty()) {
    m_previous_timestamp = (int64_t)(m_previous_timestamp + 1000000000.0 / m_fps);
    m_available_timestamps.push_back(timestamp_duration_t(m_previous_timestamp, (int64_t)(1000000000.0 / m_fps)));

    mxverb(3, boost::format("mpeg4_p2::flush_frames(): Needed new timestamp %1%\n") % m_previous_timestamp);
    ++m_statistics.m_num_generated_timestamps;
  }
}

void
mpeg4_p2_video_packetizer_c::get_next_timestamp_and_duration(int64_t &timestamp,
                                                            int64_t &duration) {
  if (m_available_timestamps.empty())
    generate_timestamp_and_duration();

  timestamp = m_available_timestamps.front().m_timestamp;
  duration = m_available_timestamps.front().m_duration;

  m_available_timestamps.pop_front();
}

void
mpeg4_p2_video_packetizer_c::flush_frames(bool end_of_file) {
  if (m_ref_frames.empty())
    return;

  if (m_ref_frames.size() == 1) {
    video_frame_t &frame = m_ref_frames.front();

    // The first frame in the file. Only apply the timestamp, nothing else.
    if (-1 == frame.timestamp) {
      get_next_timestamp_and_duration(frame.timestamp, frame.duration);
      add_packet(new packet_t(new memory_c(frame.data, frame.size, true), frame.timestamp, frame.duration));
    }
    return;
  }

  video_frame_t &bref_frame = m_ref_frames.front();
  video_frame_t &fref_frame = m_ref_frames.back();

  for (auto &frame : m_b_frames)
    get_next_timestamp_and_duration(frame.timestamp, frame.duration);
  get_next_timestamp_and_duration(fref_frame.timestamp, fref_frame.duration);

  add_packet(new packet_t(new memory_c(fref_frame.data, fref_frame.size, true), fref_frame.timestamp, fref_frame.duration, FRAME_TYPE_P == fref_frame.type ? bref_frame.timestamp : VFT_IFRAME));
  for (auto &frame : m_b_frames)
    add_packet(new packet_t(new memory_c(frame.data, frame.size, true), frame.timestamp, frame.duration, bref_frame.timestamp, fref_frame.timestamp));

  m_ref_frames.pop_front();
  m_b_frames.clear();

  if (end_of_file)
    m_ref_frames.clear();
}

void
mpeg4_p2_video_packetizer_c::flush_impl() {
  flush_frames(true);
}

void
mpeg4_p2_video_packetizer_c::extract_aspect_ratio(const unsigned char *buffer,
                                                  int size) {
  if (m_aspect_ratio_extracted)
    return;

  if ((0 != m_connected_to) || display_dimensions_or_aspect_ratio_set()) {
    m_aspect_ratio_extracted = true;
    return;
  }

  uint32_t num, den;
  if (mpeg4::p2::extract_par(buffer, size, num, den)) {
    m_aspect_ratio_extracted = true;
    set_video_aspect_ratio((double)m_hvideo_pixel_width / (double)m_hvideo_pixel_height * (double)num / (double)den, false, OPTION_SOURCE_BITSTREAM);

    generic_packetizer_c::set_headers();
    rerender_track_headers();
    mxinfo_tid(m_ti.m_fname, m_ti.m_id,
               boost::format(Y("Extracted the aspect ratio information from the MPEG4 layer 2 video data and set the display dimensions to %1%/%2%.\n"))
               % m_hvideo_display_width % m_hvideo_display_height);

  } else if (50 <= m_frames_output)
    m_aspect_ratio_extracted = true;
}

void
mpeg4_p2_video_packetizer_c::extract_size(const unsigned char *buffer,
                                          int size) {
  if (m_size_extracted)
    return;

  if (0 != m_connected_to) {
    m_size_extracted = true;
    return;
  }

  uint32_t xtr_width, xtr_height;

  if (mpeg4::p2::extract_size(buffer, size, xtr_width, xtr_height)) {
    m_size_extracted = true;

    if (!m_reader->m_appending && ((xtr_width != static_cast<uint32_t>(m_hvideo_pixel_width)) || (xtr_height != static_cast<uint32_t>(m_hvideo_pixel_height)))) {
      set_video_pixel_width(xtr_width);
      set_video_pixel_height(xtr_height);

      if (!m_output_is_native && m_ti.m_private_data && (sizeof(alBITMAPINFOHEADER) <= m_ti.m_private_data->get_size())) {
        auto bih = reinterpret_cast<alBITMAPINFOHEADER *>(m_ti.m_private_data->get_buffer());
        put_uint32_le(&bih->bi_width,  xtr_width);
        put_uint32_le(&bih->bi_height, xtr_height);
        set_codec_private(m_ti.m_private_data);
      }

      m_hvideo_display_width  = -1;
      m_hvideo_display_height = -1;

      generic_packetizer_c::set_headers();
      rerender_track_headers();

      mxinfo_tid(m_ti.m_fname, m_ti.m_id,
                 boost::format(Y("The extracted values for video width and height from the MPEG4 layer 2 video data bitstream differ from what the values "
                                 "in the source container. The ones from the video data bitstream (%1%x%2%) will be used.\n")) % xtr_width % xtr_height);
    }

  } else if (50 <= m_frames_output)
    m_aspect_ratio_extracted = true;
}
