/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the generic reader and packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <unordered_set>

#include "common/file_types.h"
#include "common/chapters/chapters.h"
#include "common/math_fwd.h"
#include "common/translation.h"
#include "merge/file_status.h"
#include "merge/id_result.h"
#include "merge/packet.h"
#include "merge/probe_range_info.h"
#include "merge/timestamp_factory.h"
#include "merge/track_info.h"
#include "merge/webm.h"

class generic_packetizer_c;

constexpr auto DEFTRACK_TYPE_AUDIO = 0;
constexpr auto DEFTRACK_TYPE_VIDEO = 1;
constexpr auto DEFTRACK_TYPE_SUBS  = 2;

class generic_reader_c {
public:
  track_info_c m_ti;
  mm_io_cptr m_in;
  uint64_t m_size{};

  std::vector<std::shared_ptr<generic_packetizer_c>> m_reader_packetizers;
  generic_packetizer_c *m_ptzr_first_packet{};
  std::vector<int64_t> m_available_track_ids, m_used_track_ids;
  std::unordered_set<int64_t> m_requested_track_ids;
  int64_t m_max_timestamp_seen{};
  mtx::chapters::kax_cptr m_chapters;
  bool m_appending{};
  int m_num_video_tracks{}, m_num_audio_tracks{}, m_num_subtitle_tracks{};

  int64_t m_reference_timestamp_tolerance{};

  probe_range_info_t m_probe_range_info{};

protected:
  id_result_t m_id_results_container;
  std::vector<id_result_t> m_id_results_tracks, m_id_results_attachments, m_id_results_chapters, m_id_results_tags;

  timestamp_c m_restricted_timestamps_min, m_restricted_timestamps_max;

public:
  virtual ~generic_reader_c() = default;

  virtual mtx::file_type_e get_format_type() const = 0;
  virtual translatable_string_c get_format_name() const;
  virtual bool is_providing_timestamps() const {
    return true;
  }

  virtual void set_timestamp_restrictions(timestamp_c const &min, timestamp_c const &max);
  virtual timestamp_c const &get_timestamp_restriction_min() const;
  virtual timestamp_c const &get_timestamp_restriction_max() const;

  virtual void set_file_to_read(mm_io_cptr const &io);
  virtual void set_probe_range_info(probe_range_info_t const &info);
  virtual void set_track_info(track_info_c const &info);
  virtual void read_headers() = 0;
  virtual file_status_e read_next(generic_packetizer_c *packetizer, bool force = false);
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) = 0;
  virtual void read_all();
  virtual int64_t get_progress();
  virtual int64_t get_maximum_progress();
  virtual void set_headers();
  virtual void set_headers_for_track(int64_t tid);
  virtual void identify() = 0;
  virtual void create_packetizer(int64_t tid) = 0;
  virtual void create_packetizers() {
    create_packetizer(0);
  }

  virtual int add_packetizer(generic_packetizer_c *packetizer);
  virtual size_t get_num_packetizers() const;
  virtual generic_packetizer_c *find_packetizer_by_id(int64_t id) const;
  virtual void set_timestamp_offset(int64_t offset);

  virtual void check_track_ids_and_packetizers();
  virtual void add_requested_track_id(int64_t id);
  virtual void add_available_track_id(int64_t id);
  virtual void add_available_track_ids();
  virtual void add_available_track_id_range(int64_t start, int64_t end);
  virtual void add_available_track_id_range(int64_t num) {
    add_available_track_id_range(0, num - 1);
  }

  virtual int64_t get_file_size() {
    return m_in->get_size();
  }
  virtual int64_t get_queued_bytes() const;
  virtual bool is_simple_subtitle_container() {
    return false;
  }

  virtual file_status_e flush_packetizer(int num);
  virtual file_status_e flush_packetizer(generic_packetizer_c *packetizer);
  virtual file_status_e flush_packetizers();

  virtual attach_mode_e attachment_requested(int64_t id);

  virtual void display_identification_results();

  virtual int64_t calculate_probe_range(int64_t file_size, int64_t fixed_minimum) const;
  virtual bool probe_file() = 0;

  virtual void show_packetizer_info(int64_t track_id, generic_packetizer_c const &packetizer);

public:
  static void set_probe_range_percentage(mtx_mp_rational_t const &probe_range_percentage);

protected:
  virtual void show_demuxer_info();

  virtual bool demuxing_requested(char type, int64_t id, mtx::bcp47::language_c const &language = {}) const;

  virtual void id_result_container(mtx::id::verbose_info_t const &verbose_info = mtx::id::verbose_info_t{});
  virtual void id_result_track(int64_t track_id, const std::string &type, const std::string &info, mtx::id::verbose_info_t const &verbose_info = mtx::id::verbose_info_t{});
  virtual void id_result_attachment(int64_t attachment_id, std::string const &type, int size, std::string const &file_name = {}, std::string const &description = {},
                                    std::optional<uint64_t> id = std::optional<uint64_t>{});
  virtual void id_result_chapters(int num_entries);
  virtual void id_result_tags(int64_t track_id, int num_entries);

  virtual mm_io_c *get_underlying_input(mm_io_c *actual_in = nullptr) const;

  virtual void display_identification_results_as_json();
  virtual void display_identification_results_as_text();

  virtual void add_track_tags_to_identification(libmatroska::KaxTags const &tags, mtx::id::info_c &info);

  virtual generic_packetizer_c &ptzr(int64_t track_idx);
};
