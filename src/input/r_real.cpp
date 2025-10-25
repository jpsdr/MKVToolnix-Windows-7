/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   RealMedia demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/id_info.h"
#include "input/r_real.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_passthrough.h"
#include "output/p_realaudio.h"
#include "output/p_generic_video.h"

namespace {
debugging_option_c s_debug{"real_reader"};
}

/*
   Description of the RealMedia file format:
   http://www.pcisys.net/~melanson/codecs/rmff.htm
*/

extern "C" {

static void *
mm_io_file_open(const char *path,
                int) {
  try {
    return reinterpret_cast<mm_io_c *>(const_cast<char *>(path));
  } catch(...) {
    return nullptr;
  }
}

static int
mm_io_file_close(void *) {
  return 0;
}

static int64_t
mm_io_file_tell(void *file) {
  return file ? static_cast<mm_io_c *>(file)->getFilePointer() : -1;
}

static int64_t
mm_io_file_seek(void *file,
                int64_t offset,
                int whence) {
  if (!file)
    return -1;

  auto smode = SEEK_END == whence ? libebml::seek_end
             : SEEK_CUR == whence ? libebml::seek_current
             :                      libebml::seek_beginning;
  return static_cast<mm_io_c *>(file)->setFilePointer2(offset, smode) ? 0 : -1;
}

static int64_t
mm_io_file_read(void *file,
                void *buffer,
                int64_t bytes) {
  return !file ? -1 : static_cast<mm_io_c *>(file)->read(buffer, bytes);
}

static int64_t
mm_io_file_write(void *file,
                 const void *buffer,
                 int64_t bytes) {
  return !file ? -1 : static_cast<mm_io_c *>(file)->write(buffer, bytes);
}

}

mb_file_io_t mm_io_file_io = {
  mm_io_file_open,
  mm_io_file_close,
  mm_io_file_read,
  mm_io_file_write,
  mm_io_file_tell,
  mm_io_file_seek
};

bool
real_reader_c::probe_file() {
  uint8_t data[4];
  return (m_in->read(data, 4) == 4) && (memcmp(data, ".RMF", 4) == 0);
}

void
real_reader_c::read_headers() {
  file = rmff_open_file_with_io(reinterpret_cast<const char *>(m_in.get()), RMFF_OPEN_MODE_READING, &mm_io_file_io);
  if (!file) {
    if (RMFF_ERR_NOT_RMFF == rmff_last_error)
      throw mtx::input::invalid_format_x();
    else
      throw mtx::input::open_x();
  }
  m_in->setFilePointer(0);

  done = false;

  show_demuxer_info();

  parse_headers();
  get_information_from_data();
}

real_reader_c::~real_reader_c() {
  rmff_close_file(file);
}

void
real_reader_c::parse_headers() {

  if (rmff_read_headers(file) != RMFF_ERR_OK)
    return;

  int ndx;
  for (ndx = 0; ndx < file->num_tracks; ndx++) {
    rmff_track_t *track = file->tracks[ndx];

    if ((RMFF_TRACK_TYPE_UNKNOWN == track->type) || (get_uint32_be(&track->mdpr_header.type_specific_size) == 0))
      continue;
    if ((RMFF_TRACK_TYPE_VIDEO == track->type) && !demuxing_requested('v', track->id))
      continue;
    if ((RMFF_TRACK_TYPE_AUDIO == track->type) && !demuxing_requested('a', track->id))
      continue;
    if (   !track->mdpr_header.mime_type
        || (   strcmp(track->mdpr_header.mime_type, "audio/x-pn-realaudio")
            && strcmp(track->mdpr_header.mime_type, "video/x-pn-realvideo")))
      continue;

    auto ts_data = track->mdpr_header.type_specific_data;
    auto ts_size = get_uint32_be(&track->mdpr_header.type_specific_size);

    real_demuxer_cptr dmx(new real_demuxer_t(track));

    if (RMFF_TRACK_TYPE_VIDEO == track->type) {
      dmx->rvp = (real_video_props_t *)track->mdpr_header.type_specific_data;

      memcpy(dmx->fourcc, &dmx->rvp->fourcc2, 4);
      dmx->fourcc[4]    = 0;
      dmx->width        = get_uint16_be(&dmx->rvp->width);
      dmx->height       = get_uint16_be(&dmx->rvp->height);
      uint32_t i        = get_uint32_be(&dmx->rvp->fps);
      dmx->fps          = static_cast<double>((i & 0xffff0000) >> 16) + (i & 0x0000ffff) / 65536.0;
      dmx->private_data = memory_c::clone(ts_data, ts_size);

      demuxers.push_back(dmx);

    } else if (RMFF_TRACK_TYPE_AUDIO == track->type) {
      bool ok     = true;

      dmx->ra4p   = (real_audio_v4_props_t *)track->mdpr_header.type_specific_data;
      dmx->ra5p   = (real_audio_v5_props_t *)track->mdpr_header.type_specific_data;

      int version = get_uint16_be(&dmx->ra4p->version1);

      if (3 == version) {
        dmx->samples_per_second = 8000;
        dmx->channels           = 1;
        dmx->bits_per_sample    = 16;
        strcpy(dmx->fourcc, "14_4");

      } else if (4 == version) {
        dmx->samples_per_second  = get_uint16_be(&dmx->ra4p->sample_rate);
        dmx->channels            = get_uint16_be(&dmx->ra4p->channels);
        dmx->bits_per_sample     = get_uint16_be(&dmx->ra4p->sample_size);

        auto p    = (uint8_t *)(dmx->ra4p + 1);
        int slen  = p[0];
        p        += (slen + 1);
        slen      = p[0];
        p++;

        if (4 != slen) {
          mxwarn(fmt::format(FY("real_reader: Couldn't find RealAudio FourCC for id {0} (description length: {1}) Skipping track.\n"), track->id, slen));
          ok = false;

        } else {
          memcpy(dmx->fourcc, p, 4);
          dmx->fourcc[4]  = 0;
          p              += 4;

          if (ts_size > static_cast<unsigned int>(p - ts_data))
            dmx->extra_data = memory_c::clone(p, ts_size - (p - ts_data));
        }

      } else if (5 == version) {
        dmx->samples_per_second = get_uint16_be(&dmx->ra5p->sample_rate);
        dmx->channels           = get_uint16_be(&dmx->ra5p->channels);
        dmx->bits_per_sample    = get_uint16_be(&dmx->ra5p->sample_size);

        memcpy(dmx->fourcc, &dmx->ra5p->fourcc3, 4);
        dmx->fourcc[4] = 0;

        if ((sizeof(real_audio_v5_props_t) + 4) < ts_size)
          dmx->extra_data = memory_c::clone(reinterpret_cast<uint8_t *>(dmx->ra5p) + 4 + sizeof(real_audio_v5_props_t), ts_size - 4 - sizeof(real_audio_v5_props_t));

      } else {
        mxwarn(fmt::format(FY("real_reader: Only audio header versions 3, 4 and 5 are supported. Track ID {0} uses version {1} and will be skipped.\n"),
                           track->id, version));
        ok = false;
      }

      mxdebug_if(s_debug, fmt::format("real_reader: extra_data_size: {0}\n", dmx->extra_data->get_size()));

      if (ok) {
        dmx->private_data = memory_c::clone(ts_data, ts_size);
        demuxers.push_back(dmx);
      }
    }
  }
}

void
real_reader_c::create_video_packetizer(real_demuxer_cptr const &dmx) {
  m_ti.m_private_data  = dmx->private_data;
  std::string codec_id = fmt::format("V_REAL/{0}", dmx->fourcc);
  dmx->ptzr            = add_packetizer(new generic_video_packetizer_c(this, m_ti, codec_id.c_str(), 0.0, dmx->width, dmx->height));

  if (strcmp(dmx->fourcc, "RV40"))
    dmx->rv_dimensions = true;

  show_packetizer_info(dmx->track->id, ptzr(dmx->ptzr));
}

void
real_reader_c::create_dnet_audio_packetizer(real_demuxer_cptr const &dmx) {
  dmx->ptzr = add_packetizer(new ac3_bs_packetizer_c(this, m_ti, dmx->samples_per_second, dmx->channels, dmx->bsid));
  show_packetizer_info(dmx->track->id, ptzr(dmx->ptzr));
}

void
real_reader_c::create_aac_audio_packetizer(real_demuxer_cptr const &dmx) {
  auto audio_config     = mtx::aac::audio_config_t{};
  bool profile_detected = false;

  int64_t tid           = dmx->track->id;

  if ((dmx->extra_data) && (4 < dmx->extra_data->get_size())) {
    auto extra_data = dmx->extra_data->get_buffer();
    auto extra_len  = get_uint32_be(extra_data);
    mxdebug_if(s_debug, fmt::format("real_reader: extra_len: {0}\n", extra_len));

    if ((4 + extra_len) <= dmx->extra_data->get_size()) {
      auto parsed_audio_config = mtx::aac::parse_audio_specific_config(&extra_data[4 + 1], extra_len - 1);
      if (!parsed_audio_config)
        mxerror_tid(m_ti.m_fname, tid, Y("This AAC track does not contain valid headers. Could not parse the AAC information.\n"));

      audio_config = *parsed_audio_config;

      mxdebug_if(s_debug, fmt::format("real_reader: 1. profile: {0}, channels: {1}, sample_rate: {2}, output_sample_rate: {3}, sbr: {4}\n", audio_config.profile, audio_config.channels, audio_config.sample_rate, audio_config.output_sample_rate, audio_config.sbr));

      if (audio_config.sbr)
        audio_config.profile = mtx::aac::PROFILE_SBR;

      profile_detected = true;
    }
  }

  if (!profile_detected) {
    audio_config.channels    = dmx->channels;
    audio_config.sample_rate = dmx->samples_per_second;
    if (!strcasecmp(dmx->fourcc, "racp") || (44100 > audio_config.sample_rate)) {
      audio_config.output_sample_rate = 2 * audio_config.sample_rate;
      audio_config.sbr                = true;
    }

  } else {
    dmx->channels           = audio_config.channels;
    dmx->samples_per_second = audio_config.sample_rate;
  }

  auto detected_profile = audio_config.profile;
  if (audio_config.sbr)
    audio_config.profile = mtx::aac::PROFILE_SBR;

  if (   (mtx::includes(m_ti.m_all_aac_is_sbr, tid) && m_ti.m_all_aac_is_sbr[tid])
      || (mtx::includes(m_ti.m_all_aac_is_sbr, -1)  && m_ti.m_all_aac_is_sbr[-1]))
    audio_config.profile = mtx::aac::PROFILE_SBR;

  if (profile_detected
      &&
      (   (mtx::includes(m_ti.m_all_aac_is_sbr, tid) && !m_ti.m_all_aac_is_sbr[tid])
       || (mtx::includes(m_ti.m_all_aac_is_sbr, -1)  && !m_ti.m_all_aac_is_sbr[-1])))
    audio_config.profile = detected_profile;

  mxdebug_if(s_debug, fmt::format("real_reader: 2. profile: {0}, channels: {1}, sample_rate: {2}, output_sample_rate: {3}, sbr: {4}\n", audio_config.profile, audio_config.channels, audio_config.sample_rate, audio_config.output_sample_rate, audio_config.sbr));

  dmx->is_aac = true;
  dmx->ptzr   = add_packetizer(new aac_packetizer_c(this, m_ti, audio_config, aac_packetizer_c::headerless));

  show_packetizer_info(tid, ptzr(dmx->ptzr));

  if (mtx::aac::PROFILE_SBR == audio_config.profile)
    ptzr(dmx->ptzr).set_audio_output_sampling_freq(audio_config.output_sample_rate);

  // AAC packetizers might need the timestamp of the first packet in order
  // to fill in stuff. Let's misuse ref_timestamp for that.
  dmx->ref_timestamp = -1;
}

void
real_reader_c::create_audio_packetizer(real_demuxer_cptr const &dmx) {
  if (!strncmp(dmx->fourcc, "dnet", 4))
    create_dnet_audio_packetizer(dmx);

  else if (!strcasecmp(dmx->fourcc, "raac") || !strcasecmp(dmx->fourcc, "racp"))
    create_aac_audio_packetizer(dmx);

  else {
    if (!strcasecmp(dmx->fourcc, "COOK"))
      dmx->cook_audio_fix = true;

    m_ti.m_private_data = dmx->private_data;
    dmx->ptzr           = add_packetizer(new ra_packetizer_c(this, m_ti, dmx->samples_per_second, dmx->channels, dmx->bits_per_sample, get_uint32_be(dmx->fourcc)));

    show_packetizer_info(dmx->track->id, ptzr(dmx->ptzr));
  }
}

void
real_reader_c::create_packetizer(int64_t tid) {

  real_demuxer_cptr dmx = find_demuxer(tid);
  if (!dmx)
    return;

  if (-1 != dmx->ptzr)
    return;

  rmff_track_t *track = dmx->track;
  m_ti.m_id           = track->id;
  m_ti.m_private_data.reset();

  if (RMFF_TRACK_TYPE_VIDEO == track->type)
    create_video_packetizer(dmx);
  else
    create_audio_packetizer(dmx);
}

void
real_reader_c::create_packetizers() {
  uint32_t i;

  for (i = 0; i < demuxers.size(); i++)
    create_packetizer(demuxers[i]->track->id);
}

real_demuxer_cptr
real_reader_c::find_demuxer(unsigned int id) {
  size_t i;

  for (i = 0; i < demuxers.size(); i++)
    if (demuxers[i]->track->id == id)
      return demuxers[i];

  return real_demuxer_cptr{};
}

file_status_e
real_reader_c::finish() {
  size_t i;

  for (i = 0; i < demuxers.size(); i++) {
    real_demuxer_cptr dmx = demuxers[i];
    if (dmx && dmx->track && (dmx->track->type == RMFF_TRACK_TYPE_AUDIO) && !dmx->segments.empty())
      deliver_audio_frames(dmx, dmx->num_packets ? dmx->last_timestamp / dmx->num_packets : 0);
  }

  done = true;

  return flush_packetizers();
}

file_status_e
real_reader_c::read(generic_packetizer_c *,
                    bool) {
  if (done)
    return flush_packetizers();

  int size = rmff_get_next_frame_size(file);
  if (0 >= size) {
    if (file->num_packets_read < file->num_packets_in_chunk)
      mxwarn_fn(m_ti.m_fname, fmt::format(FY("File contains fewer frames than expected or is corrupt after frame {0}.\n"), file->num_packets_read));
    return finish();
  }

  auto mem   = memory_c::alloc(size);
  auto frame = rmff_read_next_frame(file, mem->get_buffer());

  if (!frame) {
    if (file->num_packets_read < file->num_packets_in_chunk)
      mxwarn_fn(m_ti.m_fname, fmt::format(FY("File contains fewer frames than expected or is corrupt after frame {0}.\n"), file->num_packets_read));
    return finish();
  }

  int64_t timestamp     = (int64_t)frame->timecode * 1000000ll;
  real_demuxer_cptr dmx = find_demuxer(frame->id);

  if (!dmx || (-1 == dmx->ptzr)) {
    rmff_release_frame(frame);
    return FILE_STATUS_MOREDATA;
  }

  if (dmx->cook_audio_fix && dmx->first_frame && ((frame->flags & RMFF_FRAME_FLAG_KEYFRAME) != RMFF_FRAME_FLAG_KEYFRAME))
    dmx->force_keyframe_flag = true;

  if (dmx->force_keyframe_flag && ((frame->flags & RMFF_FRAME_FLAG_KEYFRAME) == RMFF_FRAME_FLAG_KEYFRAME))
    dmx->force_keyframe_flag = false;

  if (dmx->force_keyframe_flag)
    frame->flags |= RMFF_FRAME_FLAG_KEYFRAME;

  if (RMFF_TRACK_TYPE_VIDEO == dmx->track->type)
    assemble_video_packet(dmx, frame);

  else if (dmx->is_aac) {
    // If the first AAC packet does not start at 0 then let the AAC
    // packetizer adjust its data accordingly.
    if (dmx->first_frame) {
      dmx->ref_timestamp = timestamp;
      ptzr(dmx->ptzr).set_displacement_maybe(timestamp);
    }

    deliver_aac_frames(dmx, *mem);

  } else
    queue_audio_frames(dmx, mem, timestamp, frame->flags);

  rmff_release_frame(frame);

  dmx->first_frame = false;

  return FILE_STATUS_MOREDATA;
}

void
real_reader_c::queue_one_audio_frame(real_demuxer_cptr const &dmx,
                                     memory_cptr const &mem,
                                     uint64_t timestamp,
                                     uint32_t flags) {
  rv_segment_cptr segment(new rv_segment_t);

  segment->data  = mem;
  segment->flags = flags;
  dmx->segments.push_back(segment);

  dmx->last_timestamp = timestamp;

  mxdebug_if(s_debug, fmt::format("'{0}' track {1}: enqueueing one length {2} timestamp {3} flags 0x{4:08x}\n", m_ti.m_fname, dmx->track->id, mem->get_size(), timestamp, flags));
}

void
real_reader_c::queue_audio_frames(real_demuxer_cptr const &dmx,
                                  memory_cptr const &mem,
                                  uint64_t timestamp,
                                  uint32_t flags) {
  // Enqueue the packets if no packets are in the queue or if the current
  // packet's timestamp is the same as the timestamp of those before.
  if (dmx->segments.empty() || (dmx->last_timestamp == timestamp)) {
    queue_one_audio_frame(dmx, mem, timestamp, flags);
    return;
  }

  // This timestamp is different. So let's push the packets out.
  deliver_audio_frames(dmx, (timestamp - dmx->last_timestamp) / dmx->segments.size());

  // Enqueue this packet.
  queue_one_audio_frame(dmx, mem, timestamp, flags);
}

void
real_reader_c::deliver_audio_frames(real_demuxer_cptr const &dmx,
                                    uint64_t duration) {
  uint32_t i;

  if (dmx->segments.empty() || (-1 == dmx->ptzr))
    return;

  for (i = 0; i < dmx->segments.size(); i++) {
    rv_segment_cptr segment = dmx->segments[i];
    mxdebug_if(s_debug, fmt::format("'{0}' track {1}: delivering audio length {2} timestamp {3} flags 0x{4:08x} duration {5}\n", m_ti.m_fname, dmx->track->id, segment->data->get_size(), dmx->last_timestamp, segment->flags, duration));

    ptzr(dmx->ptzr).process(std::make_shared<packet_t>(segment->data, dmx->last_timestamp, duration, (segment->flags & RMFF_FRAME_FLAG_KEYFRAME) == RMFF_FRAME_FLAG_KEYFRAME ? -1 : dmx->ref_timestamp));
    if ((segment->flags & 2) == 2)
      dmx->ref_timestamp = dmx->last_timestamp;
  }

  dmx->num_packets += dmx->segments.size();
  dmx->segments.clear();
}

void
real_reader_c::deliver_aac_frames(real_demuxer_cptr const &dmx,
                                  memory_c &mem) {
  auto chunk  = mem.get_buffer();
  auto length = mem.get_size();
  if (2 > length) {
    mxwarn_tid(m_ti.m_fname, dmx->track->id, fmt::format(FY("Short AAC audio packet (length: {0} < 2)\n"), length));
    return;
  }

  std::size_t num_sub_packets = chunk[1] >> 4;
  mxdebug_if(s_debug, fmt::format("real_reader: num_sub_packets = {0}\n", num_sub_packets));
  if ((2 + num_sub_packets * 2) > length) {
    mxwarn_tid(m_ti.m_fname, dmx->track->id, fmt::format(FY("Short AAC audio packet (length: {0} < {1})\n"), length, 2 + num_sub_packets * 2));
    return;
  }

  std::size_t i, len_check = 2 + num_sub_packets * 2;
  for (i = 0; i < num_sub_packets; i++) {
    int sub_length  = get_uint16_be(&chunk[2 + i * 2]);
    len_check      += sub_length;

    mxdebug_if(s_debug, fmt::format("real_reader: {0}: length {1}\n", i, sub_length));
  }

  if (len_check != length) {
    mxwarn_tid(m_ti.m_fname, dmx->track->id, fmt::format(FY("Inconsistent AAC audio packet (length: {0} != {1})\n"), length, len_check));
    return;
  }

  int data_idx = 2 + num_sub_packets * 2;
  for (i = 0; i < num_sub_packets; i++) {
    int sub_length = get_uint16_be(&chunk[2 + i * 2]);
    ptzr(dmx->ptzr).process(std::make_shared<packet_t>(memory_c::borrow(&chunk[data_idx], sub_length)));
    data_idx += sub_length;
  }
}

void
real_reader_c::identify() {
  id_result_container();

  size_t i;
  for (i = 0; i < demuxers.size(); i++) {
    auto info    = mtx::id::info_c{};
    auto demuxer = demuxers[i];
    auto type    = RMFF_TRACK_TYPE_AUDIO == demuxer->track->type ? ID_RESULT_TRACK_AUDIO : ID_RESULT_TRACK_VIDEO;

    info.set(mtx::id::number, demuxer->track->id);

    if (RMFF_TRACK_TYPE_VIDEO == demuxer->track->type)
      info.add_joined(mtx::id::pixel_dimensions, "x"s, demuxer->width, demuxer->height);

    else if (RMFF_TRACK_TYPE_AUDIO == demuxer->track->type) {
      info.add(mtx::id::audio_channels,           demuxer->channels);
      info.add(mtx::id::audio_sampling_frequency, demuxer->samples_per_second);
      info.add(mtx::id::audio_bits_per_sample,    demuxer->bits_per_sample);
    }

    id_result_track(demuxer->track->id, type, codec_c::get_name(demuxer->fourcc, demuxer->fourcc), info.get());
  }
}

void
real_reader_c::assemble_video_packet(real_demuxer_cptr const &dmx,
                                     rmff_frame_t *frame) {
  int result = rmff_assemble_packed_video_frame(dmx->track, frame);
  if (0 > result) {
    mxwarn_tid(m_ti.m_fname, dmx->track->id, fmt::format(FY("Video packet assembly failed. Error code: {0} ({1})\n"), rmff_last_error, rmff_last_error_msg));
    return;
  }

  rmff_frame_t *assembled = rmff_get_packed_video_frame(dmx->track);
  while (assembled) {
    if (!dmx->rv_dimensions)
      set_dimensions(dmx, assembled->data, assembled->size);

    auto packet = std::make_shared<packet_t>(memory_c::take_ownership(assembled->data, assembled->size),
                                             (int64_t)assembled->timecode * 1000000,
                                             0,
                                             (assembled->flags & RMFF_FRAME_FLAG_KEYFRAME) == RMFF_FRAME_FLAG_KEYFRAME ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC,
                                             VFT_NOBFRAME);
    ptzr(dmx->ptzr).process(packet);

    assembled->allocated_by_rmff = 0;
    rmff_release_frame(assembled);
    assembled = rmff_get_packed_video_frame(dmx->track);
  }
}

bool
real_reader_c::get_rv_dimensions(uint8_t *buf,
                                 int size,
                                 uint32_t &width,
                                 uint32_t &height) {
  static const uint32_t cw[8]  = { 160, 176, 240, 320, 352, 640, 704, 0 };
  static const uint32_t ch1[8] = { 120, 132, 144, 240, 288, 480,   0, 0 };
  static const uint32_t ch2[4] = { 180, 360, 576,   0 };
  mtx::bits::reader_c bc(buf, size);

  try {
    bc.skip_bits(13);
    bc.skip_bits(13);
    int v = bc.get_bits(3);

    int w = cw[v];
    if (0 == w) {
      int c;
      do {
        c = bc.get_bits(8);
        w += (c << 2);
      } while (c == 255);
    }

    int c = bc.get_bits(3);
    int h = ch1[c];
    if (0 == h) {
      v = bc.get_bits(1);
      c = ((c << 1) | v) & 3;
      h = ch2[c];
      if (0 == h) {
        do {
          c  = bc.get_bits(8);
          h += (c << 2);
        } while (c == 255);
      }
    }

    width  = w;
    height = h;

    return true;

  } catch (...) {
    return false;
  }
}

void
real_reader_c::set_dimensions(real_demuxer_cptr const &dmx,
                              uint8_t *buffer,
                              int size) {
  auto ptr  = buffer;
  ptr      += 1 + 2 * 4 * (*ptr + 1);

  if ((ptr + 10) >= (buffer + size))
    return;

  buffer = ptr;

  uint32_t width, height;
  if (!get_rv_dimensions(buffer, size, width, height))
    return;

  if ((dmx->width != width) || (dmx->height != height)) {
    uint32_t disp_width  = 0;
    uint32_t disp_height = 0;

    if (!m_ti.display_dimensions_or_aspect_ratio_set()) {
      disp_width  = dmx->width;
      disp_height = dmx->height;

      dmx->width  = width;
      dmx->height = height;

    } else if (m_ti.m_display_dimensions_given) {
      disp_width  = m_ti.m_display_width;
      disp_height = m_ti.m_display_height;

      dmx->width  = width;
      dmx->height = height;

    } else if (m_ti.m_aspect_ratio_given) {
      dmx->width  = width;
      dmx->height = height;

      if ((static_cast<double>(width) / height) < m_ti.m_aspect_ratio) {
        disp_width  = (uint32_t)(height * m_ti.m_aspect_ratio);
        disp_height = height;

      } else {
        disp_width  = width;
        disp_height = (uint32_t)(width / m_ti.m_aspect_ratio);
      }

    }

    auto video = get_child<libmatroska::KaxTrackVideo>(*ptzr(dmx->ptzr).get_track_entry());
    get_child<libmatroska::KaxVideoPixelWidth>(video).SetValue(width);
    get_child<libmatroska::KaxVideoPixelHeight>(video).SetValue(height);

    if ((0 != disp_width) && (0 != disp_height)) {
      get_child<libmatroska::KaxVideoDisplayWidth>(video).SetValue(disp_width);
      get_child<libmatroska::KaxVideoDisplayHeight>(video).SetValue(disp_height);
    }

    rerender_track_headers();
  }

  dmx->rv_dimensions = true;
}

void
real_reader_c::get_information_from_data() {
  int64_t old_pos        = m_in->getFilePointer();
  bool information_found = true;

  size_t i;
  for (i = 0; i < demuxers.size(); i++) {
    real_demuxer_cptr dmx = demuxers[i];
    if (!strcasecmp(dmx->fourcc, "DNET")) {
      dmx->bsid         = -1;
      information_found = false;
    }
  }

  while (!information_found) {
    rmff_frame_t *frame   = rmff_read_next_frame(file, nullptr);
    real_demuxer_cptr dmx = find_demuxer(frame->id);

    if (!dmx) {
      rmff_release_frame(frame);
      continue;
    }

    if (!strcasecmp(dmx->fourcc, "DNET"))
      dmx->bsid = frame->data[4] >> 3;

    rmff_release_frame(frame);

    information_found = true;
    for (i = 0; i < demuxers.size(); i++) {
      dmx = demuxers[i];
      if (!strcasecmp(dmx->fourcc, "DNET") && (-1 == dmx->bsid))
        information_found = false;

    }
  }

  m_in->setFilePointer(old_pos);
  file->num_packets_read = 0;
}

void
real_reader_c::add_available_track_ids() {
  size_t i;

  for (i = 0; i < demuxers.size(); i++)
    add_available_track_id(demuxers[i]->track->id);
}
