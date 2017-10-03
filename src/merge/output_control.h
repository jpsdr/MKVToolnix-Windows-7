/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   output handling

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>
#include <unordered_map>

#include "common/bitvalue.h"
#include "common/chapters/chapters.h"
#include "common/segmentinfo.h"
#include "merge/file_status.h"
#include "merge/packet.h"

namespace libmatroska {
  class KaxChapters;
  class KaxCues;
  class KaxSeekHead;
  class KaxSegment;
  class KaxTag;
  class KaxTags;
  class KaxTrackEntry;
  class KaxTracks;
  class KaxSegmentFamily;
  class KaxInfo;
};

using namespace libmatroska;

class mm_io_c;
class generic_packetizer_c;
class track_info_c;
struct filelist_t;

struct append_spec_t {
  size_t src_file_id, src_track_id, dst_file_id, dst_track_id;
};

inline
bool operator ==(const append_spec_t &a1,
                 const append_spec_t &a2) {
  return (a1.src_file_id  == a2.src_file_id)
      && (a1.src_track_id == a2.src_track_id)
      && (a1.dst_file_id  == a2.dst_file_id)
      && (a1.dst_track_id == a2.dst_track_id);
}

inline
bool operator !=(const append_spec_t &a1,
                 const append_spec_t &a2) {
  return !(a1 == a2);
}

inline std::ostream &
operator<<(std::ostream &str,
           append_spec_t const &spec) {
  str << spec.src_file_id << ":" << spec.src_track_id << ":" << spec.dst_file_id << ":" << spec.dst_track_id;
  return str;
}

struct packetizer_t {
  file_status_e status, old_status;
  packet_cptr pack;
  generic_packetizer_c *packetizer, *orig_packetizer;
  int64_t file, orig_file;
  bool deferred;

  packetizer_t()
    : status{FILE_STATUS_MOREDATA}
    , old_status{FILE_STATUS_MOREDATA}
    , packetizer{}
    , orig_packetizer{}
    , file{}
    , orig_file{}
    , deferred{}
  {
  }
};

struct attachment_t {
  std::string name, stored_name, mime_type, description, source_file;
  uint64_t id{};
  bool to_all_files{};
  memory_cptr data;
  int64_t ui_id{};
};
using attachment_cptr = std::shared_ptr<attachment_t>;

struct track_order_t {
  int64_t file_id;
  int64_t track_id;
};

enum timestamp_scale_mode_e {
  TIMESTAMP_SCALE_MODE_NORMAL = 0,
  TIMESTAMP_SCALE_MODE_FIXED,
  TIMESTAMP_SCALE_MODE_AUTO
};

enum append_mode_e {
  APPEND_MODE_TRACK_BASED,
  APPEND_MODE_FILE_BASED,
};

enum class identification_output_format_e {
  text,
  verbose_text,
  gui,
  json,
};

class family_uids_c: public std::vector<mtx::bits::value_c> {
public:
  bool add_family_uid(const KaxSegmentFamily &family);
};

extern std::vector<packetizer_t> g_packetizers;
extern std::vector<attachment_cptr> g_attachments;
extern std::vector<track_order_t> g_track_order;
extern std::vector<append_spec_t> g_append_mapping;
extern std::unordered_map<int64_t, generic_packetizer_c *> g_packetizers_by_track_num;

extern std::string g_outfile;

extern double g_timestamp_scale;
extern timestamp_scale_mode_e g_timestamp_scale_mode;

extern mtx::bits::value_cptr g_seguid_link_previous, g_seguid_link_next;
extern std::deque<mtx::bits::value_cptr> g_forced_seguids;
extern family_uids_c g_segfamily_uids;

extern kax_info_cptr g_kax_info_chap;

extern bool g_write_meta_seek_for_clusters;

extern std::string g_chapter_file_name;
extern std::string g_chapter_language;
extern std::string g_chapter_charset;

extern std::string g_segmentinfo_file_name;

extern std::unique_ptr<KaxTags> g_tags_from_cue_chapters;

extern std::unique_ptr<KaxSegment> g_kax_segment;
extern std::unique_ptr<KaxTracks> g_kax_tracks;
extern KaxTrackEntry *g_kax_last_entry;
extern std::unique_ptr<KaxSeekHead> g_kax_sh_main, g_kax_sh_cues;
extern mtx::chapters::kax_cptr g_kax_chapters;
extern int64_t g_tags_size;
extern std::string g_segment_title;
extern bool g_segment_title_set;
extern std::string g_segment_filename, g_previous_segment_filename, g_next_segment_filename;
extern std::string g_default_language;

extern float g_video_fps;
extern generic_packetizer_c *g_video_packetizer;

extern bool g_write_cues, g_cue_writing_requested, g_write_date;
extern bool g_no_lacing, g_no_linking, g_use_durations, g_no_track_statistics_tags;

extern bool g_identifying;
extern identification_output_format_e g_identification_output_format;

extern int g_file_num;
extern int64_t g_file_sizes;

extern int64_t g_max_ns_per_cluster;
extern int g_max_blocks_per_cluster;
extern int g_default_tracks[3], g_default_tracks_priority[3];

extern int g_split_max_num_files;
extern std::string g_splitting_by_chapters_arg;

extern append_mode_e g_append_mode;

void create_packetizers();
void calc_attachment_sizes();
void calc_max_chapter_size();
void check_track_id_validity();
void check_append_mapping();

void cleanup();
void main_loop();

void add_packetizer_globally(generic_packetizer_c *packetizer);
void add_tags(KaxTag *tags);
void add_chapter_atom(timestamp_c const &start_timestamp, std::string const &name, std::string const &language);

void create_next_output_file();
void finish_file(bool last_file, bool create_new_file = false, bool previously_discarding = false);
void force_close_output_file();
void rerender_track_headers();
void rerender_ebml_head();
std::string create_output_name();

bool set_required_matroska_version(unsigned int required_version);
bool set_required_matroska_read_version(unsigned int required_version);

int64_t add_attachment(attachment_cptr const &attachment);

#if defined(SYS_UNIX) || defined(SYS_APPLE)
void sighandler(int signum);
#endif
