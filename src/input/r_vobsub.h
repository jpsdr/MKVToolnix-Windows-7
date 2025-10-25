/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the VobSub stream reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"
#include "output/p_vobsub.h"

class vobsub_entry_c {
public:
  int64_t position;
  int64_t timestamp;
  int64_t duration;

  bool operator < (const vobsub_entry_c &cmp) const;
};

class vobsub_track_c {
public:
  mtx::bcp47::language_c language;
  int ptzr;
  std::vector<vobsub_entry_c> entries;
  unsigned int idx;
  int aid;
  bool mpeg_version_warning_printed;
  int64_t packet_num, spu_size, overhead;

public:
  vobsub_track_c(mtx::bcp47::language_c const &new_language):
    language(new_language),
    ptzr(-1),
    idx(0),
    aid(-1),
    mpeg_version_warning_printed(false),
    packet_num(0),
    spu_size(0),
    overhead(0) {
  }
};

class vobsub_reader_c: public generic_reader_c {
private:
  mm_file_io_cptr m_sub_file;
  int version;
  int64_t delay;
  int64_t m_bytes_to_process{}, m_bytes_processed{};
  std::string idx_data;

  std::vector<vobsub_track_c *> tracks;

private:
  static const std::string id_string;

public:
  virtual ~vobsub_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::vobsub;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();
  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual void parse_headers();
  virtual file_status_e flush_packetizers();
  virtual int deliver_packet(uint8_t *buf, int size, int64_t timestamp, int64_t default_duration, generic_packetizer_c *packetizer);

  virtual int extract_one_spu_packet(int64_t track_id);
};
