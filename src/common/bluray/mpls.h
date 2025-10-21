/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for Blu-ray playlist files (MPLS)

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/bit_reader.h"
#include "common/fourcc.h"
#include "common/timestamp.h"

namespace mtx::bluray::mpls {

// Blu-ray specs 5.3.4.5.2.1 table 5-8
enum class stream_type_e {
  reserved                    = 0,
  used_by_play_item           = 1,
  used_by_sub_path_type_23456 = 2,
  used_by_sub_path_type_7     = 3,
};

// Blu-ray specs 5.4.4.3.2 table 5-16
enum class stream_coding_type_e {
  mpeg2_video_primary_secondary      = 0x02,
  mpeg4_avc_video_primary_secondary  = 0x1b,
  mpegh_hevc_video_primary_secondary = 0x24,
  vc1_video_primary_secondary        = 0xea,
  lpcm_audio_primary                 = 0x80,
  ac3_audio_primary                  = 0x81,
  dts_audio_primary                  = 0x82,
  truehd_audio_primary               = 0x83,
  eac3_audio_primary                 = 0x84,
  dts_hd_audio_primary               = 0x85,
  dts_hd_xll_audio_primary           = 0x86,
  eac3_audio_secondary               = 0xa1,
  dts_hd_audio_secondary             = 0xa2,
  presentation_graphics_subtitles    = 0x90,
  interactive_graphics_menu          = 0x91,
  text_subtitles                     = 0x92,
};

// Blu-ray specs 5.4.4.3.2 table 5-17
enum class video_format_e {
  i480  = 1,
  i576  = 2,
  p480  = 3,
  i1080 = 4,
  p720  = 5,
  p1080 = 6,
  p576  = 7,
};

enum class sub_path_type_e {
  reserved1                                  = 0,
  reserved2                                  = 1,
  primary_audio_of_browsable_slideshow       = 2,
  interactive_graphics_presentation_menu     = 3,
  text_subtitle_presentation                 = 4,
  out_of_mux_synchronous_elementary_streams  = 5,
  out_of_mux_asynchronous_picture_in_picture = 6,
  in_mux_synchronous_picture_in_picture      = 7,
};

class exception: public mtx::exception {
protected:
  std::string m_message;
public:
  explicit exception(std::string const &message) : m_message{message} { }
  virtual ~exception() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

struct header_t {
  fourcc_c type_indicator1, type_indicator2;
  unsigned int playlist_pos, chapter_pos, ext_pos;

  void dump() const;
};

struct stream_t {
  stream_type_e stream_type;
  stream_coding_type_e coding_type;
  unsigned int sub_path_id, sub_clip_id, pid, format, rate, char_code;
  mtx::bcp47::language_c language;

  void dump(std::string const &type) const;
};

struct stn_t {
  unsigned int num_video, num_audio, num_pg, num_ig, num_secondary_audio, num_secondary_video, num_pip_pg;
  std::vector<stream_t> audio_streams, video_streams, pg_streams;

  void dump() const;
};

struct sub_play_item_clip_t {
  std::string clpi_file_name, codec_id;
  unsigned int ref_to_stc_id;

  void dump() const;
};

struct sub_play_item_t {
  std::string clpi_file_name, codec_id;
  unsigned int connection_condition, sync_playitem_id, ref_to_stc_id;
  bool is_multi_clip_entries;
  timestamp_c in_time, out_time, sync_start_pts_of_playitem;
  std::vector<sub_play_item_clip_t> clips;

  void dump() const;
};

struct sub_path_t {
  sub_path_type_e type;
  bool is_repeat_sub_path;
  std::vector<sub_play_item_t> items;

  void dump() const;
};

struct play_item_t {
  std::string clip_id, codec_id;
  unsigned int connection_condition, stc_id;
  timestamp_c in_time, out_time, relative_in_time;
  bool is_multi_angle;
  stn_t stn;

  void dump() const;
};

struct playlist_t {
  unsigned int list_count, sub_count;
  std::vector<play_item_t> items;
  std::vector<sub_path_t> sub_paths;
  timestamp_c duration;

  void dump() const;
};

struct chapter_t {
  struct name_t {
    mtx::bcp47::language_c language;
    std::string name;
  };

  timestamp_c timestamp;
  std::vector<name_t> names;
};

using chapters_t = std::vector<chapter_t>;

class parser_c {
protected:
  bool m_ok, m_drop_last_entry_if_at_end;
  debugging_option_c m_debug;

  header_t m_header;
  playlist_t m_playlist;
  chapters_t m_chapters;

  mtx::bits::reader_cptr m_bc;

public:
  parser_c();
  virtual ~parser_c() = default;

  virtual bool parse(mm_io_c &in);
  virtual bool is_ok() const {
    return m_ok;
  }

  virtual void dump() const;

  virtual playlist_t const &get_playlist() const {
    return m_playlist;
  }

  chapters_t const &get_chapters() const {
    return m_chapters;
  }

  void enable_dropping_last_entry_if_at_end(bool enable);

protected:
  virtual void parse_header();
  virtual void parse_playlist();
  virtual play_item_t parse_play_item();
  virtual sub_path_t parse_sub_path();
  virtual sub_play_item_t parse_sub_play_item();
  virtual sub_play_item_clip_t parse_sub_play_item_clip();
  virtual stn_t parse_stn();
  virtual stream_t parse_stream();
  virtual void parse_chapters();
  virtual std::string read_string(unsigned int length);
  virtual void read_chapter_names(std::string const &base_file_name);
};
using parser_cptr = std::shared_ptr<parser_c>;

std::string get_stream_coding_type_description(uint8_t coding_type);
std::string get_video_format_description(uint8_t video_format);

}
