/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVersion.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>

#include "common/command_line.h"
#include "common/ebml.h"
#include "common/kax_file.h"
#include "common/mm_io_x.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/formatting.h"
#include "extract/mkvextract.h"
#include "extract/xtr_base.h"

using namespace libmatroska;

// ------------------------------------------------------------------------

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
create_extractors(KaxTracks &kax_tracks,
                  std::vector<track_spec_t> &tracks) {
  size_t i;
  int64_t track_id = -1;

  for (i = 0; i < kax_tracks.ListSize(); i++) {
    if (!Is<KaxTrackEntry>(kax_tracks[i]))
      continue;

    ++track_id;

    KaxTrackEntry &track = *static_cast<KaxTrackEntry *>(kax_tracks[i]);
    int64_t tnum         = kt_get_number(track);

    // Is the track number present and valid?
    if (0 == tnum)
      continue;

    // Is there more than one track with the same track number?
    if (track_extractors_by_track_number.find(tnum) != track_extractors_by_track_number.end()) {
      mxwarn(boost::format(Y("More than one track with the track number %1% found.\n")) % tnum);
      continue;
    }

    // Does the user want this track to be extracted?
    auto tspec_itr = brng::find_if(tracks, [track_id](auto const &t) { return (track_spec_t::tm_timestamps != t.target_mode) && (track_id == t.tid); });
    if (tspec_itr == tracks.end())
      continue;

    // Let's find the codec ID and create an extractor for it.
    std::string codec_id = kt_get_codec_id(track);
    if (codec_id.empty())
      mxerror(boost::format(Y("The track ID %1% does not have a valid CodecID.\n")) % track_id);

    auto extractor = xtr_base_c::create_extractor(codec_id, track_id, *tspec_itr);
    if (!extractor)
      mxerror(boost::format(Y("Extraction of track ID %1% with the CodecID '%2%' is not supported.\n")) % track_id % codec_id);

    extractor->m_track_num = tnum;

    // Has there another file been requested with the same name?
    auto extractor_itr = brng::find_if(track_extractor_list, [&tspec_itr](auto &x) { return x->m_file_name == tspec_itr->out_name; });
    xtr_base_c *master = extractor_itr != track_extractor_list.end() ? extractor_itr->get() : nullptr;

    // Let the extractor create the file.
    extractor->create_file(master, track);

    // We're done.
    track_extractors_by_track_number.insert({ tnum, extractor });
    track_extractor_list.push_back(extractor);

    mxinfo(boost::format(Y("Extracting track %1% with the CodecID '%2%' to the file '%3%'. Container format: %4%\n"))
           % track_id % codec_id % extractor->get_file_name().string() % extractor->get_container_name());
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
      extractor.m_file->puts(to_string(timestamp.m_timestamp, 1000000, 6) + "\n");

    if (!timestamps.empty()) {
      timestamp_t &last_timestamp = timestamps.back();
      extractor.m_file->puts(to_string(last_timestamp.m_timestamp + last_timestamp.m_duration, 1000000, 6) + "\n");
    }
  }

  timestamp_extractors.clear();
}

static void
create_timestamp_files(KaxTracks &kax_tracks,
                       std::vector<track_spec_t> &tracks) {
  size_t i;
  for (auto &tspec : tracks) {
    if (track_spec_t::tm_timestamps != tspec.target_mode)
      continue;

    int track_number     = -1;
    KaxTrackEntry *track = nullptr;
    for (i = 0; kax_tracks.ListSize() > i; ++i) {
      if (!Is<KaxTrackEntry>(kax_tracks[i]))
        continue;

      ++track_number;
      if (track_number != tspec.tid)
        continue;

      track = static_cast<KaxTrackEntry *>(kax_tracks[i]);
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
      mxerror(boost::format(Y("Could not open the timestamp file '%1%' for writing (%2%).\n")) % tspec.out_name % ex);
    }
  }
}

static void
handle_blockgroup_timestamps(KaxBlockGroup &blockgroup,
                             int64_t tc_scale) {
  auto block = FindChild<KaxBlock>(blockgroup);
  if (!block)
    return;

  // Do we need this block group?
  auto extractor = timestamp_extractors.find(block->TrackNum());
  if (timestamp_extractors.end() == extractor)
    return;

  // Next find the block duration if there is one.
  auto kduration   = FindChild<KaxBlockDuration>(blockgroup);
  int64_t duration = !kduration ? extractor->second->m_default_duration * block->NumberFrames() : kduration->GetValue() * tc_scale;

  // Pass the block to the extractor.
  for (auto idx = 0u, end = block->NumberFrames(); idx < end; ++idx)
    extractor->second->m_timestamps.push_back(timestamp_t(block->GlobalTimecode() + idx * duration / block->NumberFrames(), duration / block->NumberFrames()));
}

static void
handle_simpleblock_timestamps(KaxSimpleBlock &simpleblock) {
  auto itr = timestamp_extractors.find(simpleblock.TrackNum());
  if (timestamp_extractors.end() == itr)
    return;

  // Pass the block to the extractor.
  auto &extractor = *itr->second;
  for (auto idx = 0u, end = simpleblock.NumberFrames(); idx < end; ++idx)
    extractor.m_timestamps.emplace_back(simpleblock.GlobalTimecode() + idx * extractor.m_default_duration, extractor.m_default_duration);
}

static int64_t
handle_blockgroup(KaxBlockGroup &blockgroup,
                  KaxCluster &cluster,
                  int64_t tc_scale) {
  // Only continue if this block group actually contains a block.
  KaxBlock *block = FindChild<KaxBlock>(&blockgroup);
  if (!block || (0 == block->NumberFrames()))
    return -1;

  block->SetParent(cluster);

  handle_blockgroup_timestamps(blockgroup, tc_scale);

  // Do we need this block group?
  auto extractor_itr = track_extractors_by_track_number.find(block->TrackNum());
  if (extractor_itr == track_extractors_by_track_number.end())
    return -1;

  // Next find the block duration if there is one.
  auto &extractor               = *extractor_itr->second;
  KaxBlockDuration *kduration   = FindChild<KaxBlockDuration>(&blockgroup);
  int64_t duration              = !kduration ? -1 : static_cast<int64_t>(kduration->GetValue() * tc_scale);
  int64_t max_timestamp         = 0;

  // Now find backward and forward references.
  int64_t bref    = 0;
  int64_t fref    = 0;
  auto kreference = FindChild<KaxReferenceBlock>(&blockgroup);
  for (int i = 0; (2 > i) && kreference; i++) {
    if (0 > kreference->GetValue())
      bref = kreference->GetValue();
    else
      fref = kreference->GetValue();
    kreference = FindNextChild(blockgroup, *kreference);
  }

  // Any block additions present?
  KaxBlockAdditions *kadditions = FindChild<KaxBlockAdditions>(&blockgroup);

  if (0 > duration)
    duration = extractor.m_default_duration * block->NumberFrames();

  KaxCodecState *kcstate = FindChild<KaxCodecState>(&blockgroup);
  if (kcstate) {
    memory_cptr codec_state(new memory_c(kcstate->GetBuffer(), kcstate->GetSize(), false));
    extractor.handle_codec_state(codec_state);
  }

  for (int i = 0, num_frames = block->NumberFrames(); i < num_frames; i++) {
    int64_t this_timestamp, this_duration;

    if (0 > duration) {
      this_timestamp = block->GlobalTimecode();
      this_duration = duration;
    } else {
      this_timestamp = block->GlobalTimecode() + i * duration /
        block->NumberFrames();
      this_duration = duration / block->NumberFrames();
    }

    auto discard_padding  = timestamp_c::ns(0);
    auto kdiscard_padding = FindChild<KaxDiscardPadding>(blockgroup);
    if (kdiscard_padding)
      discard_padding = timestamp_c::ns(kdiscard_padding->GetValue());

    auto &data = block->GetBuffer(i);
    auto frame = std::make_shared<memory_c>(data.Buffer(), data.Size(), false);
    auto f     = xtr_frame_t{frame, kadditions, this_timestamp, this_duration, bref, fref, false, false, true, discard_padding};
    extractor.decode_and_handle_frame(f);

    max_timestamp = std::max(max_timestamp, this_timestamp);
  }

  return max_timestamp;
}

static int64_t
handle_simpleblock(KaxSimpleBlock &simpleblock,
                   KaxCluster &cluster) {
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
      this_timestamp = simpleblock.GlobalTimecode();
      this_duration = duration;
    } else {
      this_timestamp = simpleblock.GlobalTimecode() + i * duration / simpleblock.NumberFrames();
      this_duration = duration / simpleblock.NumberFrames();
    }

    auto &data = simpleblock.GetBuffer(i);
    auto frame = std::make_shared<memory_c>(data.Buffer(), data.Size(), false);
    auto f     = xtr_frame_t{frame, nullptr, this_timestamp, this_duration, -1, -1, simpleblock.IsKeyframe(), simpleblock.IsDiscardable(), false, timestamp_c::ns(0)};
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
write_all_cuesheets(KaxChapters &chapters,
                    KaxTags &tags,
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
        mxinfo(boost::format(Y("The cue sheet for track %1% will be written to '%2%'.\n")) % tspecs[i].tid % cue_file_name);
        write_cuesheet(file_name, chapters, tags, tspecs[i].tuid, *out);

      } catch(mtx::mm_io::open_x &ex) {
        mxerror(boost::format(Y("The file '%1%' could not be opened for writing: %2%.\n")) % cue_file_name % ex);
      }
    }
  }
}

void
find_and_verify_track_uids(KaxTracks &tracks,
                           std::vector<track_spec_t> &tspecs) {
  std::map<int64_t, bool> available_track_ids;
  size_t t;
  int64_t track_id = -1;

  for (t = 0; t < tracks.ListSize(); t++) {
    KaxTrackEntry *track_entry = dynamic_cast<KaxTrackEntry *>(tracks[t]);
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
      mxerror(boost::format(Y("No track with the ID %1% was found in the source file.\n")) % tspec.tid);
}

static void
handle_segment_info(EbmlMaster *info,
                    kax_file_c *file,
                    uint64_t &tc_scale) {
  auto ktc_scale = FindChild<KaxTimecodeScale>(info);
  if (!ktc_scale)
    return;

  tc_scale = ktc_scale->GetValue();
  file->set_timestamp_scale(tc_scale);
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
  uint64_t tc_scale = TIMESTAMP_SCALE;
  bool segment_info_found = false, tracks_found = false;

  // open input file
  auto af_master    = ebml_master_cptr{ analyzer.read_all(EBML_INFO(KaxInfo)) };
  auto segment_info = dynamic_cast<KaxInfo *>(af_master.get());
  if (segment_info) {
    segment_info_found = true;
    handle_segment_info(segment_info, file.get(), tc_scale);
  }

  af_master   = ebml_master_cptr{ analyzer.read_all(EBML_INFO(KaxTracks)) };
  auto tracks = dynamic_cast<KaxTracks *>(af_master.get());
  if (tracks) {
    tracks_found = true;
    find_and_verify_track_uids(*tracks, tspecs);
    create_extractors(*tracks, tspecs);
    create_timestamp_files(*tracks, tspecs);
  }

  try {
    in.setFilePointer(0);
    auto es = std::make_shared<EbmlStream>(in);

    // Find the EbmlHead element. Must be the first one.
    EbmlElement *l0 = es->FindNextID(EBML_INFO(EbmlHead), 0xFFFFFFFFL);
    if (!l0) {
      show_error(Y("Error: No EBML head found."));
      return false;
    }

    // Don't verify its data for now.
    l0->SkipData(*es, EBML_CONTEXT(l0));
    delete l0;

    while (1) {
      // Next element must be a segment
      l0 = es->FindNextID(EBML_INFO(KaxSegment), 0xFFFFFFFFFFFFFFFFLL);

      if (!l0) {
        show_error(Y("No segment/level 0 element found."));
        return false;
      }

      if (Is<KaxSegment>(l0)) {
        show_element(l0, 0, Y("Segment"));
        break;
      }

      l0->SkipData(*es, EBML_CONTEXT(l0));
      delete l0;
    }

    EbmlElement *l1   = nullptr;

    KaxChapters all_chapters;
    KaxTags all_tags;

    while ((l1 = file->read_next_level1_element())) {
      if (Is<KaxInfo>(l1) && !segment_info_found) {
        segment_info_found = true;
        handle_segment_info(static_cast<EbmlMaster *>(l1), file.get(), tc_scale);

      } else if (Is<KaxTracks>(l1) && !tracks_found) {
        tracks_found = true;
        tracks       = dynamic_cast<KaxTracks *>(l1);
        find_and_verify_track_uids(*tracks, tspecs);
        create_extractors(*tracks, tspecs);
        create_timestamp_files(*tracks, tspecs);

      } else if (Is<KaxCluster>(l1)) {
        show_element(l1, 1, Y("Cluster"));
        KaxCluster *cluster = static_cast<KaxCluster *>(l1);

        if (0 == verbose) {
          auto current_percentage = in.getFilePointer() * 100 / file_size;

          if (mtx::cli::g_gui_mode)
            mxinfo(boost::format("#GUI#progress %1%%%\n") % current_percentage);
          else
            mxinfo(boost::format(Y("Progress: %1%%%%2%")) % current_percentage % "\r");
        }

        KaxClusterTimecode *ctc = FindChild<KaxClusterTimecode>(l1);
        if (ctc) {
          uint64_t cluster_ts = ctc->GetValue();
          show_element(ctc, 2, boost::format(Y("Cluster timestamp: %|1$.3f|s")) % ((float)cluster_ts * (float)tc_scale / 1000000000.0));
          cluster->InitTimecode(cluster_ts, tc_scale);
        } else
          cluster->InitTimecode(0, tc_scale);

        size_t i;
        int64_t max_timestamp = -1;

        for (i = 0; cluster->ListSize() > i; ++i) {
          int64_t max_bg_timestamp = -1;
          EbmlElement *el          = (*cluster)[i];

          if (Is<KaxBlockGroup>(el)) {
            show_element(el, 2, Y("Block group"));
            max_bg_timestamp = handle_blockgroup(*static_cast<KaxBlockGroup *>(el), *cluster, tc_scale);

          } else if (Is<KaxSimpleBlock>(el)) {
            show_element(el, 2, Y("SimpleBlock"));
            max_bg_timestamp = handle_simpleblock(*static_cast<KaxSimpleBlock *>(el), *cluster);
          }

          max_timestamp = std::max(max_timestamp, max_bg_timestamp);
        }

        if (-1 != max_timestamp)
          file->set_last_timestamp(max_timestamp);

      } else if (Is<KaxChapters>(l1)) {
        KaxChapters &chapters = *static_cast<KaxChapters *>(l1);

        while (chapters.ListSize() > 0) {
          if (Is<KaxEditionEntry>(chapters[0])) {
            KaxEditionEntry &entry = *static_cast<KaxEditionEntry *>(chapters[0]);
            while (entry.ListSize() > 0) {
              if (Is<KaxChapterAtom>(entry[0]))
                all_chapters.PushElement(*entry[0]);
              entry.Remove(0);
            }
          }
          chapters.Remove(0);
        }

      } else if (Is<KaxTags>(l1)) {
        KaxTags &tags = *static_cast<KaxTags *>(l1);

        while (tags.ListSize() > 0) {
          all_tags.PushElement(*tags[0]);
          tags.Remove(0);
        }

      }

      delete l1;

    } // while (l1)

    delete l0;

    write_all_cuesheets(all_chapters, all_tags, tspecs);

    // Now just close the files and go to sleep. Mummy will sing you a
    // lullaby. Just close your eyes, listen to her sweet voice, singing,
    // singing, fading... fad... ing...
    close_extractors();
    close_timestamp_files();

    if (0 == verbose) {
      if (mtx::cli::g_gui_mode)
        mxinfo(boost::format("#GUI#progress %1%%%\n") % 100);
      else
        mxinfo(boost::format(Y("Progress: %1%%%%2%")) % 100 % "\n");
    }

    return true;
  } catch (...) {
    show_error(Y("Caught exception"));

    return false;
  }
}
