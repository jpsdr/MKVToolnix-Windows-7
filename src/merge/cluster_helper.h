/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the cluster helper

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <boost/date_time/posix_time/ptime.hpp>
#include <cmath>

#include <matroska/KaxBlock.h>
#include <matroska/KaxCluster.h>

#include "common/split_point.h"
#include "common/timestamp.h"
#include "merge/libmatroska_extensions.h"

#define ROUND_TIMESTAMP_SCALE(a) (std::llround(static_cast<double>(a) / static_cast<double>(g_timestamp_scale)) * static_cast<int64_t>(g_timestamp_scale))

class generic_packetizer_c;
class render_groups_c;
class packet_t;
using packet_cptr = std::shared_ptr<packet_t>;

enum class chapter_generation_mode_e {
  none,
  when_appending,
  interval,
};

class cluster_helper_c {
private:
  struct impl_t;
  std::unique_ptr<impl_t> m;

public:
  cluster_helper_c();
  ~cluster_helper_c();

  void set_output(mm_io_c *out);
  mm_io_c *get_output();
  void prepare_new_cluster();
  KaxCluster *get_cluster();
  void add_packet(packet_cptr packet);
  int64_t get_timestamp();
  int render();
  int get_cluster_content_size();
  int64_t get_duration() const;
  int64_t get_first_timestamp_in_file() const;
  int64_t get_first_timestamp_in_part() const;
  int64_t get_max_timestamp_in_file() const;
  int64_t get_discarded_duration() const;
  void handle_discarded_duration(bool create_new_file, bool previously_discarding);

  void add_split_point(split_point_c const &split_point);
  void dump_split_points() const;
  bool splitting() const;
  bool split_mode_produces_many_files() const;

  bool discarding() const;

  int get_packet_count() const;

  void discard_queued_packets();
  bool is_splitting_and_processed_fully() const;

  void create_tags_for_track_statistics(KaxTags &tags, std::string const &writing_app, boost::posix_time::ptime const &writing_date);

  void register_new_packetizer(generic_packetizer_c &ptzr);

  void enable_chapter_generation(chapter_generation_mode_e mode, std::string const &language = "");
  chapter_generation_mode_e get_chapter_generation_mode() const;
  void set_chapter_generation_interval(timestamp_c const &interval);
  void set_chapter_generation_name_template(std::string const &name_template);
  void verify_and_report_chapter_generation_parameters() const;

private:
  void set_duration(render_groups_c *rg);
  bool must_duration_be_set(render_groups_c *rg, packet_cptr &new_packet);

  void render_before_adding_if_necessary(packet_cptr &packet);
  void render_after_adding_if_necessary(packet_cptr &packet);
  void split_if_necessary(packet_cptr &packet);
  void generate_chapters_if_necessary(packet_cptr const &packet);
  void generate_one_chapter(timestamp_c const &timestamp);
  void split(packet_cptr &packet);

  bool add_to_cues_maybe(packet_cptr &pack);
};

extern std::unique_ptr<cluster_helper_c> g_cluster_helper;
