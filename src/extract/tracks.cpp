/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVersion.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>

#include "common/command_line.h"
#include "common/ebml.h"
#include "common/kax_file.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/formatting.h"
#include "extract/mkvextract.h"
#include "extract/xtr_base.h"

struct timestamp_t {
  int64_t m_timestamp, m_duration;

  timestamp_t(int64_t timestamp, int64_t duration)
    : m_timestamp(timestamp)
    , m_duration(duration)
  {
  }
};

bool
operator <(const timestamp_t &t1,
           const timestamp_t &t2) {
  return t1.m_timestamp < t2.m_timestamp;
}

struct timestamp_extractor_t {
  int64_t m_tid, m_tnum;
  mm_io_cptr m_file;
  std::vector<timestamp_t> m_timestamps;
  int64_t m_default_duration;

  timestamp_extractor_t(int64_t tid, int64_t tnum, const mm_io_cptr &file, int64_t default_duration)
    : m_tid(tid)
    , m_tnum(tnum)
    , m_file(file)
    , m_default_duration(default_duration)
  {
  }
};

static std::unordered_map<int64_t, std::shared_ptr<timestamp_extractor_t>> timestamp_extractors;

// ------------------------------------------------------------------------

static std::unordered_map<int64_t, std::shared_ptr<xtr_base_c>> track_extractors_by_track_number;
static std::vector<std::shared_ptr<xtr_base_c>> track_extractor_list;

static void
create_extractors(libmatroska::KaxTracks &kax_tracks,
                  std::vector<track_spec_t> &tracks) {
  size_t i;
  int64_t track_id = -1;

  for (i = 0; i < kax_tracks.ListSize(); i++) {
    if (!is_type<libmatroska::KaxTrackEntry>(kax_tracks[i]))
      continue;

    ++track_id;

    libmatroska::KaxTrackEntry &track = *static_cast<libmatroska::KaxTrackEntry *>(kax_tracks[i]);
    int64_t tnum         = kt_get_number(track);

    // Is the track number present and valid?
    if (0 == tnum)
      continue;

    // Is there more than one track with the same track number?
    if (track_extractors_by_track_number.find(tnum) != track_extractors_by_track_number.end()) {
      mxwarn(fmt::format(FY("More than one track with the track number {0} found.\n"), tnum));
      continue;
    }

    // Does the user want this track to be extracted?
    auto tspec_itr = std::find_if(tracks.begin(), tracks.end(), [track_id](auto const &t) { return (track_spec_t::tm_timestamps != t.target_mode) && (track_id == t.tid); });
    if (tspec_itr == tracks.end())
      continue;

    // Let's find the codec ID and create an extractor for it.
    std::string codec_id = kt_get_codec_id(track);
    if (codec_id.empty())
      mxerror(fmt::format(FY("The track ID {0} does not have a valid CodecID.\n"), track_id));

    auto extractor = xtr_base_c::create_extractor(codec_id, track_id, *tspec_itr);
    if (!extractor)
      mxerror(fmt::format(FY("Extraction of track ID {0} with the CodecID '{1}' is not supported.\n"), track_id, codec_id));

    extractor->m_track_num = tnum;

    // Has there another file been requested with the same name?
    auto extractor_itr = std::find_if(track_extractor_list.begin(), track_extractor_list.end(), [&tspec_itr](auto &x) { return x->m_file_name == tspec_itr->out_name; });
    xtr_base_c *master = extractor_itr != track_extractor_list.end() ? extractor_itr->get() : nullptr;

    // Let the extractor create the file.
    extractor->create_file(master, track);

    // We're done.
    track_extractors_by_track_number.insert({ tnum, extractor });
    track_extractor_list.push_back(extractor);

    mxinfo(fmt::format(FY("Extracting track {0} with the CodecID '{1}' to the file '{2}'. Container format: {3}\n"),
                       track_id, codec_id, extractor->get_file_name().string(), extractor->get_container_name()));
  }

  // Signal that all headers have been taken care of.
  for (auto &extractor : track_extractor_list)
    extractor->headers_done();
}

static void
close_timestamp_files() {
  for (auto &pair : timestamp_extractors) {
    auto &extractor  = *pair.second;
    auto &timestamps = extractor.m_timestamps;

    std::sort(timestamps.begin(), timestamps.end());
    for (auto timestamp : timestamps)
      extractor.m_file->puts(mtx::string::format_rational(timestamp.m_timestamp, 1000000, 6) + "\n");

    if (!timestamps.empty()) {
      timestamp_t &last_timestamp = timestamps.back();
      extractor.m_file->puts(mtx::string::format_rational(last_timestamp.m_timestamp + last_timestamp.m_duration, 1000000, 6) + "\n");
    }
  }

  timestamp_extractors.clear();
}

static void
create_timestamp_files(libmatroska::KaxTracks &kax_tracks,
                       std::vector<track_spec_t> &tracks) {
  size_t i;
  for (auto &tspec : tracks) {
    if (track_spec_t::tm_timestamps != tspec.target_mode)
      continue;

    int track_number     = -1;
    libmatroska::KaxTrackEntry *track = nullptr;
    for (i = 0; kax_tracks.ListSize() > i; ++i) {
      if (!is_type<libmatroska::KaxTrackEntry>(kax_tracks[i]))
        continue;

      ++track_number;
      if (track_number != tspec.tid)
        continue;

      track = static_cast<libmatroska::KaxTrackEntry *>(kax_tracks[i]);
      break;
    }

    if (!track)
      continue;

    try {
      mm_io_cptr file = mm_write_buffer_io_c::open(tspec.out_name, 128 * 1024);
      timestamp_extractors[kt_get_number(*track)] = std::make_shared<timestamp_extractor_t>(tspec.tid, kt_get_number(*track), file, std::max<int64_t>(kt_get_default_duration(*track), 0));
      file->puts("# timestamp format v2\n");

    } catch(mtx::mm_io::exception &ex) {
      close_timestamp_files();
      mxerror(fmt::format(FY("Could not open the timestamp file '{0}' for writing ({1}).\n"), tspec.out_name, ex));
    }
  }
}

static void
handle_blockgroup_timestamps(libmatroska::KaxBlockGroup &blockgroup,
                             int64_t tc_scale) {
  auto block = find_child<libmatroska::KaxBlock>(blockgroup);
  if (!block)
    return;

  // Do we need this block group?
  auto extractor = timestamp_extractors.find(block->TrackNum());
  if (timestamp_extractors.end() == extractor)
    return;

  // Next find the block duration if there is one.
  auto kduration   = find_child<libmatroska::KaxBlockDuration>(blockgroup);
  int64_t duration = !kduration ? extractor->second->m_default_duration * block->NumberFrames() : kduration->GetValue() * tc_scale;

  // Pass the block to the extractor.
  for (auto idx = 0u, end = block->NumberFrames(); idx < end; ++idx)
    extractor->second->m_timestamps.push_back(timestamp_t(get_global_timestamp(*block) + idx * duration / block->NumberFrames(), duration / block->NumberFrames()));
}

static void
handle_simpleblock_timestamps(libmatroska::KaxSimpleBlock &simpleblock) {
  auto itr = timestamp_extractors.find(simpleblock.TrackNum());
  if (timestamp_extractors.end() == itr)
    return;

  // Pass the block to the extractor.
  auto &extractor = *itr->second;
  for (auto idx = 0u, end = simpleblock.NumberFrames(); idx < end; ++idx)
    extractor.m_timestamps.emplace_back(get_global_timestamp(simpleblock) + idx * extractor.m_default_duration, extractor.m_default_duration);
}

static int64_t
handle_blockgroup(libmatroska::KaxBlockGroup &blockgroup,
                  libmatroska::KaxCluster &cluster,
                  int64_t tc_scale) {
  // Only continue if this block group actually contains a block.
  libmatroska::KaxBlock *block = find_child<libmatroska::KaxBlock>(&blockgroup);
  if (!block || (0 == block->NumberFrames()))
    return -1;

  block->SetParent(cluster);

  handle_blockgroup_timestamps(blockgroup, tc_scale);

  // Do we need this block group?
  auto extractor_itr = track_extractors_by_track_number.find(block->TrackNum());
  if (extractor_itr == track_extractors_by_track_number.end())
    return -1;

  // Next find the block duration if there is one.
  auto &extractor       = *extractor_itr->second;
  auto kduration        = find_child<libmatroska::KaxBlockDuration>(&blockgroup);
  int64_t duration      = !kduration ? -1 : static_cast<int64_t>(kduration->GetValue() * tc_scale);
  int64_t max_timestamp = 0;

  // Now find backward and forward references.
  int64_t bref    = 0;
  int64_t fref    = 0;
  auto kreference = find_child<libmatroska::KaxReferenceBlock>(&blockgroup);
  for (int i = 0; (2 > i) && kreference; i++) {
    if (0 > kreference->GetValue())
      bref = kreference->GetValue();
    else
      fref = kreference->GetValue();
    kreference = FindNextChild(blockgroup, *kreference);
  }

  // Any block additions present?
  auto kadditions = find_child<libmatroska::KaxBlockAdditions>(&blockgroup);

  if (0 > duration)
    duration = extractor.m_default_duration * block->NumberFrames();

  auto kcstate = find_child<libmatroska::KaxCodecState>(&blockgroup);
  if (kcstate) {
    auto ctstate = memory_c::borrow(kcstate->GetBuffer(), kcstate->GetSize());
    extractor.handle_codec_state(ctstate);
  }

  for (int i = 0, num_frames = block->NumberFrames(); i < num_frames; i++) {
    int64_t this_timestamp, this_duration;

    if (0 > duration) {
      this_timestamp = get_global_timestamp(*block);
      this_duration = duration;
    } else {
      this_timestamp = get_global_timestamp(*block) + i * duration /
        block->NumberFrames();
      this_duration = duration / block->NumberFrames();
    }

    auto discard_padding  = timestamp_c::ns(0);
    auto kdiscard_padding = find_child<libmatroska::KaxDiscardPadding>(blockgroup);
    if (kdiscard_padding)
      discard_padding = timestamp_c::ns(kdiscard_padding->GetValue());

    auto &data = block->GetBuffer(i);
    auto frame = memory_c::borrow(data.Buffer(), data.Size());
    auto f     = xtr_frame_t{frame, kadditions, this_timestamp, this_duration, bref, fref, (!bref && !fref), false, discard_padding};
    extractor.decode_and_handle_frame(f);

    max_timestamp = std::max(max_timestamp, this_timestamp);
  }

  return max_timestamp;
}

static int64_t
handle_simpleblock(libmatroska::KaxSimpleBlock &simpleblock,
                   libmatroska::KaxCluster &cluster) {
  if (0 == simpleblock.NumberFrames())
    return -1;

  simpleblock.SetParent(cluster);

  handle_simpleblock_timestamps(simpleblock);

  // Do we need this block group?
  auto extractor_itr = track_extractors_by_track_number.find(simpleblock.TrackNum());
  if (extractor_itr == track_extractors_by_track_number.end())
    return - 1;

  auto &extractor       = *extractor_itr->second;
  int64_t duration      = extractor.m_default_duration * simpleblock.NumberFrames();
  int64_t max_timestamp = 0;

  for (int i = 0, num_frames = simpleblock.NumberFrames(); i < num_frames; i++) {
    int64_t this_timestamp, this_duration;

    if (0 > duration) {
      this_timestamp = get_global_timestamp(simpleblock);
      this_duration  = duration;
    } else {
      this_timestamp = get_global_timestamp(simpleblock) + i * duration / simpleblock.NumberFrames();
      this_duration  = duration / simpleblock.NumberFrames();
    }

    auto &data = simpleblock.GetBuffer(i);
    auto frame = memory_c::borrow(data.Buffer(), data.Size());
    auto f     = xtr_frame_t{frame, nullptr, this_timestamp, this_duration, 0, 0, simpleblock.IsKeyframe(), simpleblock.IsDiscardable(), timestamp_c::ns(0)};
    extractor.decode_and_handle_frame(f);

    max_timestamp = std::max(max_timestamp, this_timestamp);
  }

  return max_timestamp;
}

static void
close_extractors() {
  for (auto &extractor : track_extractor_list)
    extractor->finish_track();

  for (auto &extractor : track_extractor_list)
    if (extractor->m_master)
      extractor->finish_file();

  for (auto &extractor : track_extractor_list)
    if (!extractor->m_master)
      extractor->finish_file();

  track_extractors_by_track_number.clear();
  track_extractor_list.clear();
}

static void
write_all_cuesheets(libmatroska::KaxChapters &chapters,
                    libmatroska::KaxTags &tags,
                    std::vector<track_spec_t> &tspecs) {
  size_t i;

  for (i = 0; i < tspecs.size(); i++) {
    if (tspecs[i].extract_cuesheet) {
      std::string file_name = tspecs[i].out_name;
      int pos               = file_name.rfind('/');
      int pos2              = file_name.rfind('\\');

      if (pos2 > pos)
        pos = pos2;
      if (pos >= 0)
        file_name.erase(0, pos2);

      std::string cue_file_name = tspecs[i].out_name;
      pos                       = cue_file_name.rfind('.');
      pos2                      = cue_file_name.rfind('/');
      int pos3                  = cue_file_name.rfind('\\');

      if ((0 <= pos) && (pos > pos2) && (pos > pos3))
        cue_file_name.erase(pos);
      cue_file_name += ".cue";

      try {
        mm_io_cptr out = mm_write_buffer_io_c::open(cue_file_name, 128 * 1024);
        mxinfo(fmt::format(FY("The cue sheet for track {0} will be written to '{1}'.\n"), tspecs[i].tid, cue_file_name));
        write_cuesheet(file_name, chapters, tags, tspecs[i].tuid, *out);

      } catch(mtx::mm_io::open_x &ex) {
        mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), cue_file_name, ex));
      }
    }
  }
}

void
find_and_verify_track_uids(libmatroska::KaxTracks &tracks,
                           std::vector<track_spec_t> &tspecs) {
  std::map<int64_t, bool> available_track_ids;
  size_t t;
  int64_t track_id = -1;

  for (t = 0; t < tracks.ListSize(); t++) {
    libmatroska::KaxTrackEntry *track_entry = dynamic_cast<libmatroska::KaxTrackEntry *>(tracks[t]);
    if (!track_entry)
      continue;

    ++track_id;
    available_track_ids[track_id] = true;

    for (auto &tspec : tspecs)
      if (tspec.tid == track_id) {
        tspec.tuid = kt_get_uid(*track_entry);
        break;
      }
  }

  for (auto &tspec : tspecs)
    if (!available_track_ids[ tspec.tid ])
      mxerror(fmt::format(FY("No track with the ID {0} was found in the source file.\n"), tspec.tid));
}

bool
extract_tracks(kax_analyzer_c &analyzer,
               options_c::mode_options_c &options) {
  auto &tspecs = options.m_tracks;

  if (tspecs.empty())
    return false;

  // open input file
  auto &in          = analyzer.get_file();
  auto file         = std::make_shared<kax_file_c>(in);
  int64_t file_size = in.get_size();

  // open input file
  auto af_segment_info = ebml_master_cptr{ analyzer.read_all(EBML_INFO(libmatroska::KaxInfo)) };
  auto segment_info    = dynamic_cast<libmatroska::KaxInfo *>(af_segment_info.get());
  auto af_tracks       = ebml_master_cptr{ analyzer.read_all(EBML_INFO(libmatroska::KaxTracks)) };
  auto tracks          = dynamic_cast<libmatroska::KaxTracks *>(af_tracks.get());

  if (!segment_info || !tracks)
    return false;

  find_and_verify_track_uids(*tracks, tspecs);
  create_extractors(*tracks, tspecs);
  create_timestamp_files(*tracks, tspecs);

  try {
    in.setFilePointer(0);
    auto es = std::make_shared<libebml::EbmlStream>(in);

    // Find the libebml::EbmlHead element. Must be the first one.
    libebml::EbmlElement *l0 = es->FindNextID(EBML_INFO(libebml::EbmlHead), 0xFFFFFFFFL);
    if (!l0) {
      show_error(Y("Error: No EBML head found."));
      return false;
    }

    // Don't verify its data for now.
    l0->SkipData(*es, EBML_CONTEXT(l0));
    delete l0;

    while (1) {
      // Next element must be a segment
      l0 = es->FindNextID(EBML_INFO(libmatroska::KaxSegment), 0xFFFFFFFFFFFFFFFFLL);

      if (!l0) {
        show_error(Y("No segment/level 0 element found."));
        return false;
      }

      if (is_type<libmatroska::KaxSegment>(l0))
        break;

      l0->SkipData(*es, EBML_CONTEXT(l0));
      delete l0;
    }

    auto previous_percentage = -1;
    auto tc_scale            = find_child_value<kax_timestamp_scale_c, uint64_t>(segment_info, 1000000);

    file->set_timestamp_scale(tc_scale);
    file->set_segment_end(static_cast<libmatroska::KaxSegment &>(*l0));

    while (true) {
      auto cluster = file->read_next_cluster();
      if (!cluster)
        break;

      auto ctc = static_cast<kax_cluster_timestamp_c *> (cluster->FindFirstElt(EBML_INFO(kax_cluster_timestamp_c), false));
      init_timestamp(*cluster, ctc ? ctc->GetValue() : 0, tc_scale);

      if (0 == verbose) {
        auto current_percentage = in.getFilePointer() * 100 / file_size;

        if (previous_percentage != static_cast<int>(current_percentage)) {
          if (mtx::cli::g_gui_mode)
            mxinfo(fmt::format("#GUI#progress {0}%\n", current_percentage));
          else
            mxinfo(fmt::format(FY("Progress: {0}%{1}"), current_percentage, "\r"));

          previous_percentage = current_percentage;
        }
      }

      size_t i;
      int64_t max_timestamp = -1;

      for (i = 0; cluster->ListSize() > i; ++i) {
        int64_t max_bg_timestamp = -1;
        libebml::EbmlElement *el          = (*cluster)[i];

        if (is_type<libmatroska::KaxBlockGroup>(el))
          max_bg_timestamp = handle_blockgroup(*static_cast<libmatroska::KaxBlockGroup *>(el), *cluster, tc_scale);

        else if (is_type<libmatroska::KaxSimpleBlock>(el))
          max_bg_timestamp = handle_simpleblock(*static_cast<libmatroska::KaxSimpleBlock *>(el), *cluster);

        max_timestamp = std::max(max_timestamp, max_bg_timestamp);
      }

      if (-1 != max_timestamp)
        file->set_last_timestamp(max_timestamp);
    }

    delete l0;

    auto af_chapters = ebml_element_cptr{ analyzer.read_all(EBML_INFO(libmatroska::KaxChapters)) };
    auto chapters    = dynamic_cast<libmatroska::KaxChapters *>(af_chapters.get());

    auto af_tags     = ebml_element_cptr{ analyzer.read_all(EBML_INFO(libmatroska::KaxTags)) };
    auto tags        = dynamic_cast<libmatroska::KaxTags *>(af_tags.get());

    if (chapters && tags)
      write_all_cuesheets(*chapters, *tags, tspecs);

    // Now just close the files and go to sleep. Mummy will sing you a
    // lullaby. Just close your eyes, listen to her sweet voice, singing,
    // singing, fading... fad... ing...
    close_extractors();
    close_timestamp_files();

    if (0 == verbose) {
      if (mtx::cli::g_gui_mode)
        mxinfo(fmt::format("#GUI#progress {0}%\n", 100));
      else
        mxinfo(fmt::format(FY("Progress: {0}%{1}"), 100, "\n"));
    }

    return true;
  } catch (...) {
    show_error(Y("Caught exception"));

    return false;
  }
}
