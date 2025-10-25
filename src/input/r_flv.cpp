/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Flash Video (FLV) demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "avilib.h"
#include "common/amf.h"
#include "common/avc/avcc.h"
#include "common/avc/es_parser.h"
#include "common/bit_reader.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/hevc/hevcc.h"
#include "common/hevc/es_parser.h"
#include "common/mm_io_x.h"
#include "common/id_info.h"
#include "common/strings/formatting.h"
#include "input/r_flv.h"
#include "merge/input_x.h"
#include "output/p_aac.h"
#include "output/p_avc.h"
#include "output/p_hevc.h"
#include "output/p_mp3.h"
#include "output/p_video_for_windows.h"

constexpr auto FLV_DETECT_SIZE = 1024 * 1024;

flv_header_t::flv_header_t()
  : signature{ 0, 0, 0 }
  , version{0}
  , type_flags{0}
  , data_offset{0}
{
}

bool
flv_header_t::has_video()
  const {
  return (type_flags & 0x01) == 0x01;
}

bool
flv_header_t::has_audio()
  const {
  return (type_flags & 0x04) == 0x04;
}

bool
flv_header_t::is_valid()
  const {
  return std::string(signature, 3) == "FLV";
}

bool
flv_header_t::read(mm_io_cptr const &in) {
  return read(*in);
}

bool
flv_header_t::read(mm_io_c &in) {
  if (sizeof(*this) != in.read(this, sizeof(*this)))
    return false;

  data_offset = get_uint32_be(&data_offset);

  return true;
}

// --------------------------------------------------

flv_tag_c::flv_tag_c()
  : m_previous_tag_size{}
  , m_flags{}
  , m_data_size{}
  , m_timestamp{}
  , m_timestamp_extended{}
  , m_next_position{}
  , m_ok{}
  , m_debug{"flv_full|flv_tags|flv_tag"}
{
}

bool
flv_tag_c::is_encrypted()
  const {
  return (m_flags & 0x20) == 0x20;
}

bool
flv_tag_c::is_audio()
  const {
  return 0x08 == m_flags;
}

bool
flv_tag_c::is_video()
  const {
  return 0x09 == m_flags;
}

bool
flv_tag_c::is_script_data()
  const {
  return 0x12 == m_flags;
}

bool
flv_tag_c::read(mm_io_cptr const &in) {
  try {
    auto position        = in->getFilePointer();
    m_ok                 = false;
    m_previous_tag_size  = in->read_uint32_be();
    m_flags              = in->read_uint8();
    m_data_size          = in->read_uint24_be();
    m_timestamp          = in->read_uint24_be();
    m_timestamp_extended = in->read_uint8();
    in->skip(3);
    m_next_position      = in->getFilePointer() + m_data_size;
    m_ok                 = true;

    mxdebug_if(m_debug, fmt::format("Tag @ {0}: {1}\n", position, *this));

    return true;

  } catch (mtx::mm_io::exception &) {
    return false;
  }
}

// --------------------------------------------------

flv_track_c::flv_track_c(char type)
  : m_type{type}
  , m_headers_read{}
  , m_ptzr{-1}
  , m_timestamp{}
  , m_v_version{}
  , m_v_width{}
  , m_v_height{}
  , m_v_dwidth{}
  , m_v_dheight{}
  , m_v_cts_offset{}
  , m_v_frame_type{}
  , m_a_channels{}
  , m_a_sample_rate{}
  , m_a_bits_per_sample{}
  , m_a_profile{}
{
}

bool
flv_track_c::is_audio()
  const {
  return 'a' == m_type;
}

bool
flv_track_c::is_video()
  const {
  return 'v' == m_type;
}

bool
flv_track_c::is_valid()
  const {
  return m_headers_read && !!m_fourcc;
}

bool
flv_track_c::is_ptzr_set()
  const {
  return -1 != m_ptzr;
}

void
flv_track_c::postprocess_header_data() {
  if (m_headers_read)
    return;

  if (m_fourcc == "FLV1")
    extract_flv1_width_and_height();
}

void
flv_track_c::extract_flv1_width_and_height() {
  static auto const s_dimensions = std::vector< std::pair<unsigned int, unsigned int> >{
    { 352, 288 }, { 176, 144 }, { 128, 96 }, { 320, 240 }, { 160, 120 }
  };

  if (m_v_width && m_v_height) {
    m_headers_read = true;
    return;
  }

  if (!m_payload || !m_payload->get_size())
    return;

  try {
    auto r = mtx::bits::reader_c{m_payload->get_buffer(), static_cast<unsigned int>(m_payload->get_size())};
    if (r.get_bits(17) != 1)
      return;                     // bad picture start code

    auto format = r.get_bits(5);
    if ((0 != format) && (1 != format))
      return;                     // bad picture format

    r.skip_bits(8);             // picture timestamp

    format = r.get_bits(3);
    if ((0 == format) || (1 == format)) {
      auto num_bits   = 0 == format ? 8 : 16;
      m_v_width       = r.get_bits(num_bits);
      m_v_height      = r.get_bits(num_bits);

    } else if (format < (s_dimensions.size() + 2)) {
      auto dimensions = s_dimensions[format - 2];
      m_v_width       = dimensions.first;
      m_v_height      = dimensions.second;
    }

  } catch (mtx::exception &) {
  }

  m_headers_read = m_v_width && m_v_height;
}

// --------------------------------------------------

bool
flv_reader_c::probe_file() {
  return m_header.read(*m_in) && m_header.is_valid();
}

unsigned int
flv_reader_c::add_track(char type) {
  m_tracks.push_back(flv_track_cptr{new flv_track_c(type)});
  return m_tracks.size() - 1;
}

void
flv_reader_c::read_headers() {
  mxdebug_if(m_debug, fmt::format("Header dump: {0}\n", m_header));

  m_in->setFilePointer(sizeof(m_header));

  auto audio_track_valid = false;
  auto video_track_valid = false;

  try {
    while (!m_file_done && (m_in->getFilePointer() < FLV_DETECT_SIZE)) {
      if (process_tag(true))
        for (auto &track : m_tracks) {
          if (!track->is_valid())
            continue;

          else if (track->is_audio())
            audio_track_valid = true;

          else if (track->is_video())
            video_track_valid = true;
        }

      if (!m_tag.m_ok)
        break;
      m_in->setFilePointer(m_tag.m_next_position);
    }

  } catch (...) {
    throw mtx::input::invalid_format_x();
  }

  m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(), [](flv_track_cptr const &t) { return !t->is_valid(); }), m_tracks.end());

  m_audio_track_idx = -1;
  m_video_track_idx = -1;
  for (int idx = m_tracks.size(); idx > 0; --idx) {
    // auto &track = m_tracks
    if (m_tracks[idx - 1]->is_video())
      m_video_track_idx = idx - 1;
    else
      m_audio_track_idx = idx - 1;
  }

  mxdebug_if(m_debug, fmt::format("Detection finished at {0}; audio valid? {1}; video valid? {2}; number valid tracks: {3}; min timestamp: {4}\n",
                                  m_in->getFilePointer(), audio_track_valid, video_track_valid, m_tracks.size(), mtx::string::format_timestamp(m_min_timestamp.value_or(0) * 1'000'000ll)));

  m_in->setFilePointer(9); // rewind file for later remux
  m_file_done     = false;
  m_min_timestamp = m_min_timestamp.value_or(0);
}

void
flv_reader_c::identify() {
  id_result_container();

  size_t idx = 0;
  for (auto track : m_tracks) {
    auto info = mtx::id::info_c{};

    if (track->m_fourcc.equiv("avc1"))
      info.add(mtx::id::packetizer, mtx::id::mpeg4_p10_video);

    if (track->is_video())
      info.add_joined(mtx::id::pixel_dimensions, "x"s, track->m_v_width, track->m_v_height);

    else if (track->is_audio()) {
      info.add(mtx::id::audio_channels,           track->m_a_channels);
      info.add(mtx::id::audio_sampling_frequency, track->m_a_sample_rate);
      info.add(mtx::id::audio_bits_per_sample,    track->m_a_bits_per_sample);
    }

    id_result_track(idx, track->is_audio() ? ID_RESULT_TRACK_AUDIO : ID_RESULT_TRACK_VIDEO,
                      track->m_fourcc.equiv("AVC1") ? "AVC/H.264"
                    : track->m_fourcc.equiv("FLV1") ? "Sorenson h.263 (Flash version)"
                    : track->m_fourcc.equiv("HVC1") ? "HEVC/H.265/MPEG-H"
                    : track->m_fourcc.equiv("VP6F") ? "On2 VP6 (Flash version)"
                    : track->m_fourcc.equiv("VP6A") ? "On2 VP6 (Flash version with alpha channel)"
                    : track->m_fourcc.equiv("AAC ") ? "AAC"
                    : track->m_fourcc.equiv("MP3 ") ? "MP3"
                    :                                 "Unknown",
                    info.get());
    ++idx;
  }
}

void
flv_reader_c::add_available_track_ids() {
  add_available_track_id_range(m_tracks.size());
}

void
flv_reader_c::create_packetizer(int64_t id) {
  if ((0 > id) || (m_tracks.size() <= static_cast<size_t>(id)) || m_tracks[id]->is_ptzr_set())
    return;

  auto &track = m_tracks[id];

  if (!demuxing_requested(track->m_type, id))
    return;

  m_ti.m_id = id;
  m_ti.m_private_data.reset();

  if (track->m_fourcc.equiv("AVC1"))
    create_v_avc_packetizer(track);

  else if (track->m_fourcc.equiv("FLV1") || track->m_fourcc.equiv("VP6F") || track->m_fourcc.equiv("VP6A"))
    create_v_generic_packetizer(track);

  else if (track->m_fourcc.equiv("HVC1"))
    create_v_hevc_packetizer(track);

  else if (track->m_fourcc.equiv("AAC "))
    create_a_aac_packetizer(track);

  else if (track->m_fourcc.equiv("MP3 "))
    create_a_mp3_packetizer(track);
}

void
flv_reader_c::create_v_avc_packetizer(flv_track_cptr &track) {
  m_ti.m_private_data = track->m_private_data;
  track->m_ptzr       = add_packetizer(new avc_video_packetizer_c(this, m_ti, track->m_v_frame_rate ? mtx::to_int_rounded(1'000'000'000 / track->m_v_frame_rate) : 0, track->m_v_width, track->m_v_height));
  show_packetizer_info(m_video_track_idx, ptzr(track->m_ptzr));
}

void
flv_reader_c::create_v_hevc_packetizer(flv_track_cptr &track) {
  m_ti.m_private_data = track->m_private_data;
  track->m_ptzr       = add_packetizer(new hevc_video_packetizer_c(this, m_ti, track->m_v_frame_rate ? mtx::to_int_rounded(1'000'000'000 / track->m_v_frame_rate) : 0, track->m_v_width, track->m_v_height));
  show_packetizer_info(m_video_track_idx, ptzr(track->m_ptzr));
}

void
flv_reader_c::create_v_generic_packetizer(flv_track_cptr &track) {
  alBITMAPINFOHEADER bih;

  memset(&bih, 0, sizeof(bih));
  put_uint32_le(&bih.bi_size,        sizeof(bih));
  put_uint32_le(&bih.bi_width,       track->m_v_width);
  put_uint32_le(&bih.bi_height,      track->m_v_height);
  put_uint16_le(&bih.bi_planes,      1);
  put_uint16_le(&bih.bi_bit_count,   24);
  put_uint32_le(&bih.bi_size_image,  track->m_v_width * track->m_v_height * 3);
  track->m_fourcc.write(reinterpret_cast<uint8_t *>(&bih.bi_compression));

  m_ti.m_private_data = memory_c::clone(&bih, sizeof(bih));

  track->m_ptzr = add_packetizer(new video_for_windows_packetizer_c(this, m_ti, track->m_v_frame_rate ? mtx::to_int_rounded(1'000'000'000 / track->m_v_frame_rate) : 0, track->m_v_width, track->m_v_height));
  show_packetizer_info(m_video_track_idx, ptzr(track->m_ptzr));
}

void
flv_reader_c::create_a_aac_packetizer(flv_track_cptr &track) {
  mtx::aac::audio_config_t audio_config{};

  audio_config.profile     = track->m_a_profile;
  audio_config.sample_rate = track->m_a_sample_rate;
  audio_config.channels    = track->m_a_channels;
  track->m_ptzr            = add_packetizer(new aac_packetizer_c(this, m_ti, audio_config, aac_packetizer_c::headerless));

  show_packetizer_info(m_audio_track_idx, ptzr(track->m_ptzr));
}

void
flv_reader_c::create_a_mp3_packetizer(flv_track_cptr &track) {
  track->m_ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, track->m_a_sample_rate, track->m_a_channels, true));
  show_packetizer_info(m_audio_track_idx, ptzr(track->m_ptzr));
}

void
flv_reader_c::create_packetizers() {
  for (auto id = 0u; m_tracks.size() > id; ++id)
    create_packetizer(id);
}

bool
flv_reader_c::new_stream_v_avc(flv_track_cptr &track,
                               memory_cptr const &data) {
  try {
    auto avcc = mtx::avc::avcc_c::unpack(data);
    avcc.parse_sps_list(true);

    for (auto &sps_info : avcc.m_sps_info_list) {
      if (!track->m_v_width)
        track->m_v_width = sps_info.width;
      if (!track->m_v_height)
        track->m_v_height = sps_info.height;
      if (!track->m_v_frame_rate && sps_info.timing_info.num_units_in_tick && sps_info.timing_info.time_scale)
        track->m_v_frame_rate = mtx::rational(sps_info.timing_info.time_scale, sps_info.timing_info.num_units_in_tick);
    }

    if (!track->m_v_frame_rate)
      track->m_v_frame_rate = 25;

  } catch (mtx::mm_io::exception &) {
  }

  mxdebug_if(m_debug, fmt::format("new_stream_v_avc: video width: {0}, height: {1}, frame rate: {2}\n", track->m_v_width, track->m_v_height, track->m_v_frame_rate));

  return true;
}

bool
flv_reader_c::new_stream_v_hevc(flv_track_cptr &track,
                                memory_cptr const &data) {
  try {
    auto hevcc = mtx::hevc::hevcc_c::unpack(data);
    hevcc.parse_vps_list(true);
    hevcc.parse_sps_list(true);

    for (auto &sps_info : hevcc.m_sps_info_list) {
      if (!track->m_v_width)
        track->m_v_width = sps_info.width;
      if (!track->m_v_height)
        track->m_v_height = sps_info.height;
      if (!track->m_v_frame_rate && sps_info.timing_info_present && sps_info.num_units_in_tick && sps_info.time_scale)
        track->m_v_frame_rate = mtx::rational(sps_info.time_scale, sps_info.num_units_in_tick);
    }

    if (!track->m_v_frame_rate)
      track->m_v_frame_rate = 25;

  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug, fmt::format("new_stream_v_hevc: HEVCC parsing failed: {0}\n", ex.what()));
  }

  mxdebug_if(m_debug, fmt::format("new_stream_v_hevc: video width: {0}, height: {1}, frame rate: {2}\n", track->m_v_width, track->m_v_height, track->m_v_frame_rate));

  return true;
}

bool
flv_reader_c::process_audio_tag_sound_format(flv_track_cptr &track,
                                             uint8_t sound_format) {
  static const std::vector<std::string> s_formats{
      "Linear PCM platform endian"
    , "ADPCM"
    , "MP3"
    , "Linear PCM little endian"
    , "Nellymoser 16 kHz mono"
    , "Nellymoser 8 kHz mono"
    , "Nellymoser"
    , "G.711 A-law logarithmic PCM"
    , "G.711 mu-law logarithmic PCM"
    , ""
    , "AAC"
    , "Speex"
    , "MP3 8 kHz"
    , "Device-specific sound"
  };

  if (15 < sound_format) {
    mxdebug_if(m_debug, fmt::format("Sound format: not handled ({0})\n", static_cast<unsigned int>(sound_format)));
    return true;
  }

  mxdebug_if(m_debug, fmt::format("Sound format: {0}\n", s_formats[sound_format]));

  // AAC
  if (10 == sound_format) {
    if (!m_tag.m_data_size)
      return false;

    track->m_fourcc = "AAC ";
    uint8_t aac_packet_type = m_in->read_uint8();
    m_tag.m_data_size--;
    if (aac_packet_type != 0) {
      // Raw AAC
      mxdebug_if(m_debug, fmt::format("  AAC sub type: raw\n"));
      return true;
    }

    if (!m_tag.m_data_size)
      return false;

    auto specific_codec_buf = memory_c::alloc(m_tag.m_data_size);
    if (m_in->read(specific_codec_buf, m_tag.m_data_size) != m_tag.m_data_size)
       return false;

    auto audio_config = mtx::aac::parse_audio_specific_config(specific_codec_buf->get_buffer(), specific_codec_buf->get_size());
    if (!audio_config)
      return false;

    mxdebug_if(m_debug,
               fmt::format("  AAC sub type: sequence header (profile: {0}, channels: {1}, s_rate: {2}, out_s_rate: {3}, sbr {4})\n",
                           audio_config->profile, audio_config->channels, audio_config->sample_rate, audio_config->output_sample_rate, audio_config->sbr));

    track->m_a_profile     = audio_config->sbr ? mtx::aac::PROFILE_SBR : audio_config->profile;
    track->m_a_channels    = audio_config->channels;
    track->m_a_sample_rate = audio_config->sample_rate;
    m_tag.m_data_size      = 0;

    return true;
  }

  switch (sound_format) {
    case 2:  track->m_fourcc = "MP3 "; break;
    case 14: track->m_fourcc = "MP3 "; break;
    default:
      return false;
  }

  return true;
}

bool
flv_reader_c::process_audio_tag(flv_track_cptr &track) {
  uint8_t audiotag_header = m_in->read_uint8();
  uint8_t format          = (audiotag_header & 0xf0) >> 4;
  uint8_t rate            = (audiotag_header & 0x0c) >> 2;
  uint8_t size            = (audiotag_header & 0x02) >> 1;
  uint8_t type            =  audiotag_header & 0x01;

  mxdebug_if(m_debug, fmt::format("Audio packet found\n"));

  if (!m_tag.m_data_size)
    return false;
  m_tag.m_data_size--;

  process_audio_tag_sound_format(track, format);

  if (!track->m_a_sample_rate && (4 > rate)) {
    static unsigned int s_rates[] = { 5512, 11025, 22050, 44100 };
    track->m_a_sample_rate = s_rates[rate];
  }

  if (!track->m_a_channels)
    track->m_a_channels = 0 == type ? 1 : 2;

  track->m_headers_read = true;

  mxdebug_if(m_debug, fmt::format("  sampling frequency: {0}; sample size: {1}; channels: {2}\n", track->m_a_sample_rate, 0 == size ? "8 bits" : "16 bits", track->m_a_channels));

  return true;
}

bool
flv_reader_c::process_video_tag_avc(flv_track_cptr &track) {
  if (4 > m_tag.m_data_size)
    return false;

  track->m_fourcc          = "AVC1";
  uint8_t avc_packet_type  = m_in->read_uint8();
  track->m_v_cts_offset    = m_in->read_int24_be();
  m_tag.m_data_size       -= 4;

  // The CTS offset is only valid for NALUs.
  if (1 != avc_packet_type)
    track->m_v_cts_offset = 0;

  // Only sequence headers need more processing
  if (0 != avc_packet_type)
    return true;

  mxdebug_if(m_debug, fmt::format("  AVC sequence header at {0}\n", m_in->getFilePointer()));

  auto data         = m_in->read(m_tag.m_data_size);
  m_tag.m_data_size = 0;

  if (!track->m_headers_read) {
    if (!new_stream_v_avc(track, data))
      return false;

    track->m_headers_read = true;
  }

  track->m_extra_data = data;
  if (!track->m_private_data)
    track->m_private_data = data->clone();

  return true;
}

bool
flv_reader_c::process_video_tag_hevc(flv_track_cptr &track) {
  if (4 > m_tag.m_data_size)
    return false;

  track->m_fourcc         = "HVC1";
  auto hevc_packet_type   = m_in->read_uint8();
  track->m_v_cts_offset   = m_in->read_int24_be();
  m_tag.m_data_size      -= 4;

  mxdebug_if(m_debug, fmt::format("  process_video_tag_hevc: hevc_packet_type {0} size {1}\n", hevc_packet_type, m_tag.m_data_size));

  // The CTS offset is only valid for NALUs.
  if (1 != hevc_packet_type)
    track->m_v_cts_offset = 0;

  // Only sequence headers need more processing
  if (0 != hevc_packet_type)
    return true;

  mxdebug_if(m_debug, fmt::format("  HEVC sequence header at {0} size {1}\n", m_in->getFilePointer(), m_tag.m_data_size));

  auto data         = m_in->read(m_tag.m_data_size);
  m_tag.m_data_size = 0;

  if (!track->m_headers_read) {
    if (!new_stream_v_hevc(track, data))
      return false;

    track->m_headers_read = true;
  }

  track->m_extra_data = data;
  if (!track->m_private_data)
    track->m_private_data = data->clone();

  return true;
}

bool
flv_reader_c::process_video_tag_generic(flv_track_cptr &track,
                                        flv_tag_c::codec_type_e codec_id) {
  track->m_fourcc       = flv_tag_c::CODEC_SORENSON_H263  == codec_id ? "FLV1"
                        : flv_tag_c::CODEC_VP6            == codec_id ? "VP6F"
                        : flv_tag_c::CODEC_VP6_WITH_ALPHA == codec_id ? "VP6A"
                        :                                               "BUG!";

  if ((track->m_fourcc == "VP6A") || (track->m_fourcc == "VP6F")) {
    if (!m_tag.m_data_size)
      return false;
    m_tag.m_data_size--;
    m_in->skip(1);
    track->m_headers_read = true;

  } else if (track->m_fourcc == "FLV1")
    track->m_headers_read = track->m_v_width && track->m_v_height;

  return true;

}

bool
flv_reader_c::process_video_tag(flv_track_cptr &track) {
  static struct {
    std::string name;
    bool is_key;
  } s_frame_types[] = {
      { "key frame",                true  }
    , { "inter frame",              false }
    , { "disposable inter frame",   false }
    , { "generated key frame",      true  }
    , { "video info/command frame", false }
  };

  mxdebug_if(m_debug, fmt::format("Video packet found\n"));

  if (!m_tag.m_data_size)
    return false;

  uint8_t video_tag_header = m_in->read_uint8();
  m_tag.m_data_size--;

  uint8_t frame_type = (video_tag_header >> 4) & 0x0f;

  if ((1 <= frame_type) && (5 >= frame_type)) {
    auto type = s_frame_types[frame_type - 1];
    mxdebug_if(m_debug, fmt::format("  Frame type: {0}\n", type.name));
    track->m_v_frame_type = type.is_key ? 'I' : 'P';
  } else {
    mxdebug_if(m_debug, fmt::format("  Frame type unknown ({0})\n", static_cast<unsigned int>(frame_type)));
    track->m_v_frame_type = 'P';
  }

  static std::string s_codecs[] {
      "Sorenson H.263"
    , "Screen video"
    , "On2 VP6"
    , "On2 VP6 with alpha channel"
    , "Screen video version 2"
    , "H.264"
  };

  auto codec_id = static_cast<flv_tag_c::codec_type_e>(video_tag_header & 0x0f);

  if ((flv_tag_c::CODEC_SORENSON_H263 <= codec_id) && (flv_tag_c::CODEC_H264 >= codec_id)) {
    mxdebug_if(m_debug, fmt::format("  Codec type: {0}\n", s_codecs[codec_id - flv_tag_c::CODEC_SORENSON_H263]));

    if (flv_tag_c::CODEC_H264 == codec_id)
      return process_video_tag_avc(track);

    else if (   (flv_tag_c::CODEC_SORENSON_H263  == codec_id)
             || (flv_tag_c::CODEC_VP6            == codec_id)
             || (flv_tag_c::CODEC_VP6_WITH_ALPHA == codec_id))
      return process_video_tag_generic(track, codec_id);

    else
      track->m_headers_read = true;

  } else if (flv_tag_c::CODEC_H265 == codec_id) {
    mxdebug_if(m_debug, "  Codec type: H.265\n");
    return process_video_tag_hevc(track);

  } else {
    mxdebug_if(m_debug, fmt::format("  Codec type unknown ({0})\n", static_cast<unsigned int>(codec_id)));
    track->m_headers_read = true;
  }

  return true;
}

bool
flv_reader_c::process_script_tag() {
  if (!m_tag.m_data_size)
    return true;

  if (-1 == m_video_track_idx)
    m_video_track_idx = add_track('v');

  try {
    mtx::amf::script_parser_c parser{m_in->read(m_tag.m_data_size)};
    parser.parse();

    std::optional<double> number;

    if (number = parser.get_meta_data_value<double>("framerate"); number && *number) {
      m_tracks[m_video_track_idx]->m_v_frame_rate = *number;
      mxdebug_if(m_debug, fmt::format("Video frame rate from meta data: {0}\n", *number));
    }

    if ((number = parser.get_meta_data_value<double>("width"))) {
      m_tracks[m_video_track_idx]->m_v_width = *number;
      mxdebug_if(m_debug, fmt::format("Video width from meta data: {0}\n", *number));
    }

    if ((number = parser.get_meta_data_value<double>("height"))) {
      m_tracks[m_video_track_idx]->m_v_height = *number;
      mxdebug_if(m_debug, fmt::format("Video height from meta data: {0}\n", *number));
    }

  } catch (mtx::mm_io::exception &) {
  }

  return true;
}

bool
flv_reader_c::process_tag(bool skip_payload) {
  m_selected_track_idx = -1;

  if (!m_tag.read(m_in)) {
    m_file_done = true;
    return false;
  }

  if (m_tag.is_encrypted())
    return false;

  if (m_tag.is_script_data())
    return process_script_tag();

  if (m_tag.is_audio()) {
    if (-1 == m_audio_track_idx)
      m_audio_track_idx = add_track('a');
    m_selected_track_idx = m_audio_track_idx;

  } else if (m_tag.is_video()) {
    if (-1 == m_video_track_idx)
      m_video_track_idx = add_track('v');
    m_selected_track_idx = m_video_track_idx;

  } else
    return false;

  if ((0 > m_selected_track_idx) || (static_cast<int>(m_tracks.size()) <= m_selected_track_idx))
    return false;

  auto &track = m_tracks[m_selected_track_idx];

  if (m_tag.is_audio() && !process_audio_tag(track))
    return false;

  else if (m_tag.is_video() && !process_video_tag(track))
    return false;

  auto timestamp = m_tag.m_timestamp + (m_tag.m_timestamp_extended << 24);

  mxdebug_if(m_debug, fmt::format("Data size after processing: {0}; timestamp in ms: {1}\n", m_tag.m_data_size, timestamp));

  if (!m_tag.m_data_size)
    return true;

  track->m_payload   = m_in->read(m_tag.m_data_size);
  track->m_timestamp = timestamp;

  track->postprocess_header_data();

  if (skip_payload) {
    track->m_payload.reset();
    m_min_timestamp = m_min_timestamp ? std::min(*m_min_timestamp, timestamp) : timestamp;
  }

  return true;
}

file_status_e
flv_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (m_file_done || m_in->eof())
    return flush_packetizers();

  bool tag_processed = false;
  try {
    tag_processed = process_tag();
  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug, fmt::format("Exception: {0}\n", ex));
    m_tag.m_ok = false;
  }

  if (!tag_processed) {
    if (m_tag.m_ok && m_in->setFilePointer2(m_tag.m_next_position))
      return FILE_STATUS_MOREDATA;

    m_file_done = true;
    return flush_packetizers();
  }

  if (-1 == m_selected_track_idx)
    return FILE_STATUS_MOREDATA;

  auto &track = m_tracks[m_selected_track_idx];

  if (!track->m_payload)
    return FILE_STATUS_MOREDATA;

  if (-1 != track->m_ptzr) {
    track->m_timestamp = (track->m_timestamp + track->m_v_cts_offset - *m_min_timestamp) * 1000000ll;
    mxdebug_if(m_debug, fmt::format(" PTS in nanoseconds: {0}\n", track->m_timestamp));

    int64_t duration = -1;
    if (track->m_v_frame_rate && track->m_fourcc.equiv("AVC1"))
      duration = mtx::to_int(mtx::rational(1'000'000'000, track->m_v_frame_rate));

    auto packet = std::make_shared<packet_t>(track->m_payload, track->m_timestamp, duration, 'I' == track->m_v_frame_type ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC, VFT_NOBFRAME);

    if (track->m_extra_data)
      packet->codec_state = track->m_extra_data;

    ptzr(track->m_ptzr).process(packet);
  }

  track->m_payload.reset();
  track->m_extra_data.reset();

  if (m_in->setFilePointer2(m_tag.m_next_position))
    return FILE_STATUS_MOREDATA;

  m_file_done = true;
  return flush_packetizers();
}
