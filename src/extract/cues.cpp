/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extract cues from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlStream.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>

#include "common/ebml.h"
#include "common/kax_analyzer.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/strings/formatting.h"
#include "extract/mkvextract.h"



struct cue_point_t {
  uint64_t timestamp;
  std::optional<uint64_t> cluster_position, relative_position, duration;

  cue_point_t(uint64_t p_timestamp)
    : timestamp{p_timestamp}
  {
  }
};

static void
write_cues(std::vector<track_spec_t> const &tracks,
           std::map<int64_t, int64_t> const &track_number_map,
           std::unordered_map<int64_t, std::vector<cue_point_t> > const &cue_points,
           uint64_t segment_data_start_pos,
           uint64_t timestamp_scale) {
  for (auto const &track : tracks) {
    auto track_number_itr = track_number_map.find(track.tid);
    if (track_number_itr == track_number_map.end())
      mxerror(fmt::format(FY("The file does not contain track ID {0}.\n"), track.tid));

    auto cue_points_itr = cue_points.find(track_number_itr->second);
    if (cue_points_itr == cue_points.end())
      mxerror(fmt::format(FY("There are no cues for track ID {0}.\n"), track.tid));

    auto &track_cue_points = cue_points_itr->second;

    try {
      mxinfo(fmt::format(FY("The cues for track {0} are written to '{1}'.\n"), track.tid, track.out_name));

       mm_file_io_c out{track.out_name, libebml::MODE_CREATE};

      for (auto const &p : track_cue_points) {
        auto line = fmt::format("timestamp={0} duration={1} cluster_position={2} relative_position={3}\n",
                                mtx::string::format_timestamp(p.timestamp * timestamp_scale, 9),
                                p.duration          ? mtx::string::format_timestamp(p.duration.value() * timestamp_scale, 9)      : "-",
                                p.cluster_position  ? fmt::to_string(p.cluster_position.value() + segment_data_start_pos) : "-",
                                p.relative_position ? fmt::to_string(p.relative_position.value())                         : "-");
        out.puts(line);
      }

    } catch (mtx::mm_io::exception &ex) {
      mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), track.out_name, ex));
    }
  }
}

static std::map<int64_t, int64_t>
generate_track_number_map(kax_analyzer_c &analyzer) {
  auto track_number_map = std::map<int64_t, int64_t>{};
  auto tracks_m         = analyzer.read_all(EBML_INFO(libmatroska::KaxTracks));
  auto tracks           = dynamic_cast<libmatroska::KaxTracks *>(tracks_m.get());

  if (!tracks)
    return track_number_map;

  auto tid = 0;

  for (auto const &elt : *tracks) {
    auto ktrack_entry = dynamic_cast<libmatroska::KaxTrackEntry *>(elt);
    if (!ktrack_entry)
      continue;

    auto ktrack_number = find_child<libmatroska::KaxTrackNumber>(ktrack_entry);
    if (ktrack_number)
      track_number_map[tid++] = ktrack_number->GetValue();
  }

  return track_number_map;
}

static uint64_t
find_timestamp_scale(kax_analyzer_c &analyzer) {
  auto info_m = analyzer.read_all(EBML_INFO(libmatroska::KaxInfo));
  auto info   = dynamic_cast<libmatroska::KaxInfo *>(info_m.get());

  return info ? find_child_value<kax_timestamp_scale_c>(info, 1000000ull) : 1000000ull;
}

static std::unordered_map<int64_t, std::vector<cue_point_t> >
parse_cue_points(kax_analyzer_c &analyzer) {
  auto cues_m = analyzer.read_all(EBML_INFO(libmatroska::KaxCues));
  auto cues   = dynamic_cast<libmatroska::KaxCues *>(cues_m.get());

  if (!cues)
    mxerror(Y("No cues were found.\n"));

  auto cue_points = std::unordered_map<int64_t, std::vector<cue_point_t> >{};

  for (auto const &elt : *cues) {
    auto kcue_point = dynamic_cast<libmatroska::KaxCuePoint *>(elt);
    if (!kcue_point)
      continue;

    auto ktime = find_child<libmatroska::KaxCueTime>(*kcue_point);
    if (!ktime)
      continue;

    auto p = cue_point_t{ktime->GetValue()};
    auto ktrack_pos = find_child<libmatroska::KaxCueTrackPositions>(*kcue_point);
    if (!ktrack_pos)
      continue;

    for (auto const &pos_elt : *ktrack_pos) {
      if (is_type<libmatroska::KaxCueClusterPosition>(pos_elt))
        p.cluster_position = static_cast<libmatroska::KaxCueClusterPosition *>(pos_elt)->GetValue();

      else if (is_type<libmatroska::KaxCueRelativePosition>(pos_elt))
        p.relative_position = static_cast<libmatroska::KaxCueRelativePosition *>(pos_elt)->GetValue();

      else if (is_type<libmatroska::KaxCueDuration>(pos_elt))
        p.duration = static_cast<libmatroska::KaxCueDuration *>(pos_elt)->GetValue();
    }

    for (auto const &pos_elt : *ktrack_pos)
      if (is_type<libmatroska::KaxCueTrack>(pos_elt))
        cue_points[ static_cast<libmatroska::KaxCueTrack *>(pos_elt)->GetValue() ].push_back(p);
  }

  return cue_points;
}

static void
determine_cluster_data_start_positions(mm_io_c &file,
                                       uint64_t segment_data_start_pos,
                                       std::unordered_map<int64_t, std::vector<cue_point_t> > &cue_points) {
  auto es           = std::make_shared<libebml::EbmlStream>(file);
  auto upper_lvl_el = 0;

  for (auto &track_cue_points_pair : cue_points) {
    for (auto &cue_point : track_cue_points_pair.second) {
      if (!cue_point.cluster_position || !cue_point.relative_position)
        continue;

      try {
        file.setFilePointer(segment_data_start_pos + cue_point.cluster_position.value());
        auto elt = std::shared_ptr<libebml::EbmlElement>(es->FindNextElement(EBML_CLASS_CONTEXT(libmatroska::KaxSegment), upper_lvl_el, std::numeric_limits<int64_t>::max(), true));

        if (elt && is_type<libmatroska::KaxCluster>(*elt))
          cue_point.relative_position = cue_point.relative_position.value() + get_head_size(*elt);

      } catch (mtx::mm_io::exception &) {
      }
    }
  }
}

bool
extract_cues(kax_analyzer_c &analyzer,
             options_c::mode_options_c &options) {
  if (options.m_tracks.empty())
    return false;

  auto cue_points             = parse_cue_points(analyzer);
  auto timestamp_scale        = find_timestamp_scale(analyzer);
  auto track_number_map       = generate_track_number_map(analyzer);
  auto segment_data_start_pos = analyzer.get_segment_data_start_pos();

  determine_cluster_data_start_positions(analyzer.get_file(), segment_data_start_pos, cue_points);
  write_cues(options.m_tracks, track_number_map, cue_points, segment_data_start_pos, timestamp_scale);

  return true;
}
