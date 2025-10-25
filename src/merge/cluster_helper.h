/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the cluster helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <cmath>

#include <QDateTime>

#include <matroska/KaxBlock.h>
#include <matroska/KaxCluster.h>

#include "common/bcp47.h"
#include "common/split_point.h"
#include "common/timestamp.h"
#include "merge/libmatroska_extensions.h"

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
  libmatroska::KaxCluster *get_cluster();
  void add_packet(packet_cptr const &packet);
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

  void create_tags_for_track_statistics(libmatroska::KaxTags &tags, std::string const &writing_app, QDateTime const &writing_date);

  void register_new_packetizer(generic_packetizer_c &ptzr);

  void enable_chapter_generation(chapter_generation_mode_e mode, mtx::bcp47::language_c const &language = {});
  chapter_generation_mode_e get_chapter_generation_mode() const;
  void set_chapter_generation_interval(timestamp_c const &interval);
  void verify_and_report_chapter_generation_parameters() const;

private:
  void set_duration(render_groups_c *rg);
  bool must_duration_be_set(render_groups_c *rg, packet_cptr const &new_packet);

  void render_before_adding_if_necessary(packet_cptr const &packet);
  void render_after_adding_if_necessary(packet_cptr const &packet);
  void split_if_necessary(packet_cptr const &packet);
  void generate_chapters_if_necessary(packet_cptr const &packet);
  void generate_one_chapter(timestamp_c const &timestamp);
  void split(packet_cptr const &packet);

  bool add_to_cues_maybe(packet_cptr const &pack);

#if LIBEBML_VERSION >= 0x020000
  bool write_element_pred(libebml::EbmlElement const &elt);
#endif
};

extern std::unique_ptr<cluster_helper_c> g_cluster_helper;
