/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts timestamps from Matroska files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <algorithm>

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

#include "common/command_line.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/formatting.h"
#include "extract/mkvextract.h"
#include "extract/xtr_base.h"

using namespace libmatroska;

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

static std::vector<timestamp_extractor_t> timestamp_extractors;

// ------------------------------------------------------------------------

static void
close_timestamp_files() {
  for (auto &extractor : timestamp_extractors) {
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
                       std::vector<track_spec_t> &tracks,
                       int version) {
  size_t i;
  for (auto &tspec : tracks) {
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
      timestamp_extractors.push_back(timestamp_extractor_t(tspec.tid, kt_get_number(*track), file, std::max(kt_get_default_duration(*track), static_cast<int64_t>(0))));
      file->puts(boost::format("# timestamp format v%1%\n") % version);

    } catch(mtx::mm_io::exception &ex) {
      close_timestamp_files();
      mxerror(boost::format(Y("Could not open the timestamp file '%1%' for writing (%2%).\n")) % tspec.out_name % ex);
    }
  }
}

static
std::vector<timestamp_extractor_t>::iterator
find_extractor_by_track_number(unsigned int track_number) {
  return std::find_if(timestamp_extractors.begin(), timestamp_extractors.end(),
                      [=](timestamp_extractor_t &xtr) { return track_number == xtr.m_tnum; });
}

static void
handle_blockgroup(KaxBlockGroup &blockgroup,
                  KaxCluster &cluster,
                  int64_t tc_scale) {
  // Only continue if this block group actually contains a block.
  KaxBlock *block = FindChild<KaxBlock>(&blockgroup);
  if (!block)
    return;
  block->SetParent(cluster);

  // Do we need this block group?
  std::vector<timestamp_extractor_t>::iterator extractor = find_extractor_by_track_number(block->TrackNum());
  if (timestamp_extractors.end() == extractor)
    return;

  // Next find the block duration if there is one.
  KaxBlockDuration *kduration = FindChild<KaxBlockDuration>(&blockgroup);
  int64_t duration            = !kduration ? extractor->m_default_duration * block->NumberFrames() : kduration->GetValue() * tc_scale;

  // Pass the block to the extractor.
  size_t i;
  for (i = 0; block->NumberFrames() > i; ++i)
    extractor->m_timestamps.push_back(timestamp_t(block->GlobalTimecode() + i * duration / block->NumberFrames(), duration / block->NumberFrames()));
}

static void
handle_simpleblock(KaxSimpleBlock &simpleblock,
                   KaxCluster &cluster) {
  if (0 == simpleblock.NumberFrames())
    return;

  simpleblock.SetParent(cluster);

  std::vector<timestamp_extractor_t>::iterator extractor = find_extractor_by_track_number(simpleblock.TrackNum());
  if (timestamp_extractors.end() == extractor)
    return;

  // Pass the block to the extractor.
  size_t i;
  for (i = 0; simpleblock.NumberFrames() > i; ++i)
    extractor->m_timestamps.push_back(timestamp_t(simpleblock.GlobalTimecode() + i * extractor->m_default_duration, extractor->m_default_duration));
}

bool
extract_timestamps(kax_analyzer_c &analyzer,
                   options_c::mode_options_c &options) {
  if (options.m_tracks.empty())
    return false;

  // open input file
  auto &in = analyzer.get_file();

  try {
    in.setFilePointer(0);

    int64_t file_size = in.get_size();
    auto es           = std::make_shared<EbmlStream>(in);

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

    bool tracks_found = false;
    int upper_lvl_el  = 0;
    uint64_t tc_scale = TIMESTAMP_SCALE;

    // We've got our segment, so let's find the tracks
    EbmlElement *l1   = es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true, 1);
    EbmlElement *l2   = nullptr;
    EbmlElement *l3   = nullptr;

    while (l1 && (0 >= upper_lvl_el)) {
      if (Is<KaxInfo>(l1)) {
        // General info about this Matroska file
        show_element(l1, 1, Y("Segment information"));

        upper_lvl_el = 0;
        l2           = es->FindNextElement(EBML_CONTEXT(l1), upper_lvl_el, 0xFFFFFFFFL, true, 1);
        while (l2 && (0 >= upper_lvl_el)) {
          if (Is<KaxTimecodeScale>(l2)) {
            KaxTimecodeScale &ktc_scale = *static_cast<KaxTimecodeScale *>(l2);
            ktc_scale.ReadData(es->I_O());
            tc_scale = ktc_scale.GetValue();
            show_element(l2, 2, boost::format(Y("Timestamp scale: %1%")) % tc_scale);
          } else
            l2->SkipData(*es, EBML_CONTEXT(l2));

          if (!in_parent(l1)) {
            delete l2;
            break;
          }

          if (0 > upper_lvl_el) {
            upper_lvl_el++;
            if (0 > upper_lvl_el)
              break;

          }

          l2->SkipData(*es, EBML_CONTEXT(l2));
          delete l2;
          l2 = es->FindNextElement(EBML_CONTEXT(l1), upper_lvl_el, 0xFFFFFFFFL, true);
        }

      } else if ((Is<KaxTracks>(l1)) && !tracks_found) {

        // Yep, we've found our KaxTracks element. Now find all tracks
        // contained in this segment.
        show_element(l1, 1, Y("Segment tracks"));

        tracks_found = true;
        l1->Read(*es, EBML_CLASS_CONTEXT(KaxTracks), upper_lvl_el, l2, true);
        find_and_verify_track_uids(*dynamic_cast<KaxTracks *>(l1), options.m_tracks);
        create_timestamp_files(*dynamic_cast<KaxTracks *>(l1), options.m_tracks, 2);

      } else if (Is<KaxCluster>(l1)) {
        show_element(l1, 1, Y("Cluster"));
        KaxCluster *cluster = (KaxCluster *)l1;
        uint64_t cluster_ts = 0;

        if (0 == verbose) {
          auto current_percentage = in.getFilePointer() * 100 / file_size;

          if (mtx::cli::g_gui_mode)
            mxinfo(boost::format("#GUI#progress %1%%%\n") % current_percentage);
          else
            mxinfo(boost::format(Y("Progress: %1%%%%2%")) % current_percentage % "\r");
        }

        upper_lvl_el = 0;
        l2           = es->FindNextElement(EBML_CONTEXT(l1), upper_lvl_el, 0xFFFFFFFFL, true, 1);
        while (l2 && (0 >= upper_lvl_el)) {

          if (Is<KaxClusterTimecode>(l2)) {
            KaxClusterTimecode &ctc = *static_cast<KaxClusterTimecode *>(l2);
            ctc.ReadData(es->I_O());
            cluster_ts = ctc.GetValue();
            show_element(l2, 2, boost::format(Y("Cluster timestamp: %|1$.3f|s")) % ((float)cluster_ts * (float)tc_scale / 1000000000.0));
            cluster->InitTimecode(cluster_ts, tc_scale);

          } else if (Is<KaxBlockGroup>(l2)) {
            show_element(l2, 2, Y("Block group"));

            l2->Read(*es, EBML_CLASS_CONTEXT(KaxBlockGroup), upper_lvl_el, l3, true);
            handle_blockgroup(*static_cast<KaxBlockGroup *>(l2), *cluster, tc_scale);

          } else if (Is<KaxSimpleBlock>(l2)) {
            show_element(l2, 2, Y("Simple block"));

            l2->Read(*es, EBML_CLASS_CONTEXT(KaxSimpleBlock), upper_lvl_el, l3, true);
            handle_simpleblock(*static_cast<KaxSimpleBlock *>(l2), *cluster);

          } else
            l2->SkipData(*es, EBML_CONTEXT(l2));

          if (!in_parent(l1)) {
            delete l2;
            break;
          }

          if (0 < upper_lvl_el) {
            upper_lvl_el--;
            if (0 < upper_lvl_el)
              break;
            delete l2;
            l2 = l3;
            continue;

          } else if (0 > upper_lvl_el) {
            upper_lvl_el++;
            if (0 > upper_lvl_el)
              break;

          }

          l2->SkipData(*es, EBML_CONTEXT(l2));
          delete l2;
          l2 = es->FindNextElement(EBML_CONTEXT(l1), upper_lvl_el, 0xFFFFFFFFL, true);

        } // while (l2)

      } else
        l1->SkipData(*es, EBML_CONTEXT(l1));

      if (!in_parent(l0)) {
        delete l1;
        break;
      }

      if (0 < upper_lvl_el) {
        upper_lvl_el--;
        if (0 < upper_lvl_el)
          break;
        delete l1;
        l1 = l2;
        continue;

      } else if (0 > upper_lvl_el) {
        upper_lvl_el++;
        if (0 > upper_lvl_el)
          break;

      }

      l1->SkipData(*es, EBML_CONTEXT(l1));
      delete l1;
      l1 = es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true);

    } // while (l1)

    delete l0;

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

    close_timestamp_files();

    return false;
  }
}
