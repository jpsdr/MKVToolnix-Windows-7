/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the RealMedia demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"
#include "librmff/librmff.h"
#include "output/p_video_for_windows.h"
#include "merge/generic_reader.h"

struct rv_segment_t {
  memory_cptr data;
  uint64_t flags;
};

using rv_segment_cptr = std::shared_ptr<rv_segment_t>;

struct real_demuxer_t {
  int ptzr;
  rmff_track_t *track;

  int bsid;
  unsigned int channels, samples_per_second, bits_per_sample;
  unsigned int width, height;
  char fourcc[5];
  bool is_aac;
  bool rv_dimensions;
  bool force_keyframe_flag;
  bool cook_audio_fix;
  double fps;

  real_video_props_t *rvp;
  real_audio_v4_props_t *ra4p;
  real_audio_v5_props_t *ra5p;

  memory_cptr private_data, extra_data;

  bool first_frame;
  int num_packets;
  uint64_t last_timestamp;
  int64_t ref_timestamp;         // can be negative

  std::vector<rv_segment_cptr> segments;

  real_demuxer_t(rmff_track_t *n_track):
    ptzr(-1),
    track(n_track),
    bsid(0),
    channels(0),
    samples_per_second(0),
    bits_per_sample(0),
    width(0),
    height(0),
    is_aac(false),
    rv_dimensions(false),
    force_keyframe_flag(false),
    cook_audio_fix(false),
    fps(0.0),
    rvp(nullptr),
    ra4p(nullptr),
    ra5p(nullptr),
    first_frame(true),
    num_packets(0),
    last_timestamp(0),
    ref_timestamp(0) {

    memset(fourcc, 0, 5);
  };
};

using real_demuxer_cptr = std::shared_ptr<real_demuxer_t>;

class real_reader_c: public generic_reader_c {
private:
  rmff_file_t *file;
  std::vector<std::shared_ptr<real_demuxer_t> > demuxers;
  bool done;

public:
  virtual ~real_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::real;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual void parse_headers();
  virtual real_demuxer_cptr find_demuxer(unsigned int id);
  virtual void assemble_video_packet(real_demuxer_cptr const &dmx, rmff_frame_t *frame);
  virtual file_status_e finish();
  virtual bool get_rv_dimensions(uint8_t *buf, int size, uint32_t &width, uint32_t &height);
  virtual void set_dimensions(real_demuxer_cptr const &dmx, uint8_t *buffer, int size);
  virtual void get_information_from_data();
  virtual void deliver_aac_frames(real_demuxer_cptr const &dmx, memory_c &mem);
  virtual void queue_audio_frames(real_demuxer_cptr const &dmx, memory_cptr const &mem, uint64_t timestamp, uint32_t flags);
  virtual void queue_one_audio_frame(real_demuxer_cptr const &dmx, memory_cptr const &mem, uint64_t timestamp, uint32_t flags);
  virtual void deliver_audio_frames(real_demuxer_cptr const &dmx, uint64_t duration);

  virtual void create_audio_packetizer(real_demuxer_cptr const &dmx);
  virtual void create_aac_audio_packetizer(real_demuxer_cptr const &dmx);
  virtual void create_dnet_audio_packetizer(real_demuxer_cptr const &dmx);
  virtual void create_video_packetizer(real_demuxer_cptr const &dmx);
};
