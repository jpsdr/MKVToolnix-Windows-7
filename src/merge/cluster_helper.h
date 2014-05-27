/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the cluster helper

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_CLUSTER_HELPER_C
#define MTX_CLUSTER_HELPER_C

#include "common/common_pch.h"

#include <boost/date_time/posix_time/ptime.hpp>

#include <matroska/KaxBlock.h>
#include <matroska/KaxCluster.h>

#include "common/split_point.h"
#include "merge/libmatroska_extensions.h"
#include "merge/pr_generic.h"


#define RND_TIMECODE_SCALE(a) (irnd((double)(a) / (double)((int64_t)g_timecode_scale)) * (int64_t)g_timecode_scale)

class render_groups_c;

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
  int64_t get_timecode();
  int render();
  int get_cluster_content_size();
  int64_t get_duration() const;
  int64_t get_first_timecode_in_file() const;
  int64_t get_first_timecode_in_part() const;
  int64_t get_max_timecode_in_file() const;
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

private:
  void set_duration(render_groups_c *rg);
  bool must_duration_be_set(render_groups_c *rg, packet_cptr &new_packet);

  void render_before_adding_if_necessary(packet_cptr &packet);
  void render_after_adding_if_necessary(packet_cptr &packet);
  void split_if_necessary(packet_cptr &packet);
  void split(packet_cptr &packet);

  bool add_to_cues_maybe(packet_cptr &pack);
};

extern cluster_helper_c *g_cluster_helper;

#endif // MTX_CLUSTER_HELPER_C
