/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   hacks :)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/base64.h"
#include "common/hacks.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"

namespace mtx::hacks {

namespace {
std::vector<bool> s_engaged_hacks(MAX_IDX + 1, false);
}

std::vector<hack_t>
get_list() {
  using svec = std::vector<std::string>;

  std::vector<hack_t> hacks;

  hacks.emplace_back("space_after_chapters",               svec{ Y("Leave additional space (EbmlVoid) in the destination file after the chapters.") });
  hacks.emplace_back("no_chapters_in_meta_seek",           svec{ Y("Do not add an entry for the chapters in the meta seek element.") });
  hacks.emplace_back("no_meta_seek",                       svec{ Y("Do not write meta seek elements at all.") });
  hacks.emplace_back("lacing_xiph",                        svec{ Y("Force Xiph style lacing.") });
  hacks.emplace_back("lacing_ebml",                        svec{ Y("Force EBML style lacing.") });
  hacks.emplace_back("native_mpeg4",                       svec{ Y("Analyze MPEG4 bitstreams, put each frame into one Matroska block, use proper timestamping (I P B B = 0 120 40 80), use V_MPEG4/ISO/... CodecIDs.") });
  hacks.emplace_back("no_variable_data",                   svec{ Y("Use fixed values for the elements that change with each file otherwise (multiplexing date, segment UID, track UIDs etc.)."),
                                                                 Y("Two files multiplexed with the same settings and this switch activated will be identical.") });
  hacks.emplace_back("force_passthrough_packetizer",       svec{ Y("Forces the Matroska reader to use the generic passthrough packetizer even for known and supported track types.") });
  hacks.emplace_back("write_headers_twice",                svec{ Y("Causes mkvmerge to write a second set of identical track headers near the end of the file (after all the clusters).") });
  hacks.emplace_back("allow_avc_in_vfw_mode",              svec{ Y("Allows storing AVC/H.264 video in Video-for-Windows compatibility mode, e.g. when it is read from an AVI.") });
  hacks.emplace_back("keep_bitstream_ar_info",             svec{ Y("This option does nothing and is only kept for backwards compatibility.") });
  hacks.emplace_back("no_simpleblocks",                    svec{ Y("Disable the use of SimpleBlocks instead of BlockGroups.") });
  hacks.emplace_back("use_codec_state_only",               svec{ Y("Store changes in CodecPrivate data in CodecState elements instead of the frames."),
                                                                 Y("This is used for e.g. MPEG-1/-2 video tracks for storing the sequence headers.") });
  hacks.emplace_back("enable_timestamp_warning",           svec{ Y("Enables warnings for certain conditions where timestamps aren't monotonous in situations where they should be which could indicate either a problem with "
                                                                   "the file or a programming error.") });
  hacks.emplace_back("remove_bitstream_ar_info",           svec{ Y("Normally mkvmerge keeps aspect ratio information in MPEG4 video bitstreams and puts the information into the container."),
                                                                 Y("This option causes mkvmerge to remove the aspect ratio information from the bitstream.") });
  hacks.emplace_back("vobsub_subpic_stop_cmds",            svec{ Y("Causes mkvmerge to add 'stop display' commands to VobSub subtitle packets that do not contain a duration field.") });
  hacks.emplace_back("no_cue_duration",                    svec{ Y("Causes mkvmerge not to write 'CueDuration' elements in the cues.") });
  hacks.emplace_back("no_cue_relative_position",           svec{ Y("Causes mkvmerge not to write 'CueRelativePosition' elements in the cues.") });
  hacks.emplace_back("no_delay_for_garbage_in_avi",        svec{ Y("Garbage at the start of audio tracks in AVI files is normally used for delaying that track."),
                                                                 Y("mkvmerge normally calculates the delay implied by its presence and offsets all of the track's timestamps by it."),
                                                                 Y("This option prevents that behavior.") });
  hacks.emplace_back("keep_last_chapter_in_mpls",          svec{ Y("Blu-ray discs often contain a chapter entry very close to the end of the movie."),
                                                                 Y("mkvmerge normally removes that last entry if it's timestamp is within five seconds of the total duration."),
                                                                 Y("Enabling this option causes mkvmerge to keep that last entry.") });
  hacks.emplace_back("keep_track_statistics_tags",         svec{ Y("Don't remove track statistics tags when reading Matroska files, no matter if new ones are created or not.") });
  hacks.emplace_back("all_i_slices_are_key_frames",        svec{ Y("Some AVC/H.264 tracks contain I slices but lack real key frames."),
                                                                 Y("This option forces mkvmerge to treat all of those I slices as key frames.") });
  hacks.emplace_back("append_and_split_flac",              svec{ Y("Enable appending and splitting FLAC tracks."),
                                                                 Y("The resulting tracks will be broken: the official FLAC tools will not be able to decode them and seeking will not work as expected.") });
  hacks.emplace_back("dont_normalize_parameter_sets",      svec{ Y("Normally the HEVC/H.265 code in mkvmerge and mkvextract normalizes parameter sets by prefixing all key frames with all currently active parameter sets and removes duplicates that might already be present."),
                                                                 Y("If this hack is enabled, the code will leave the parameter sets as they are.") });
  hacks.emplace_back("keep_whitespaces_in_text_subtitles", svec{ Y("Normally spaces & tabs are removed from the beginning & the end of each line in text subtitles."),
                                                                 Y("If this hack is enabled, they won't be removed.") });
  hacks.emplace_back("always_write_block_add_ids",         svec{ Y("If enabled, the BlockAddID element will be written even if it's set to its default value of 1.") });
  hacks.emplace_back("keep_dolby_vision_layers_separate",  svec{ Y("Prevents mkvmerge from looking for Dolby Vision Enhancement Layers stored in a separate track & combining them with the track containing the Dolby Vision Base Layer.") });
  hacks.emplace_back("keep_bsid_9_10_in_ac3_codec_id",     svec{ Y("Causes mkvmerge to use the codec IDs A_AC3/BSID9 & A_AC3/BSID10 instead of A_AC3 for AC-3 tracks with BSIDs of 9 or 10 like older versions of mkvmerge did.") });
  hacks.emplace_back("cow",                                svec{ Y("No help available.") });

  return hacks;
}

bool
is_engaged(unsigned int id) {
  return (s_engaged_hacks.size() > id) && s_engaged_hacks[id];
}

void
engage(unsigned int id) {
  if (s_engaged_hacks.size() > id)
    s_engaged_hacks[id] = true;
}

void
engage(const std::string &hacks) {
  auto engage_args   = mtx::string::split(hacks, ",");
  auto list_of_hacks = get_list();

  if (std::find(engage_args.begin(), engage_args.end(), "list") != engage_args.end()) {
    mxinfo(Y("Valid hacks are:\n"));

    for (auto const &hack : list_of_hacks)
      mxinfo(fmt::format("  • {0} — {1}\n", hack.name, mtx::string::join(hack.description, " ")));

    mxexit();
  }

  if (std::find(engage_args.begin(), engage_args.end(), "cow") != engage_args.end()) {
    auto const initial    = "ICAgICAgICAgIChfXykKICAgICAgICAgICgqKikgIE9oIGhvbmV5LCB0aGF0J3Mgc28gc3dlZXQhCiAgIC8tLS0tLS0tXC8gICBPZiBjb3Vyc2UgSSdsbCBtYXJyeSB5b3UhCiAgLyB8ICAgICB8fAogKiAgfHwtLS0tfHwKICAgIF5eICAgIF5eCg=="s;
    auto const correction = mtx::base64::decode(initial);
    mxinfo(correction->to_string());
    mxexit();
  }

  auto hacks_begin = list_of_hacks.begin();
  auto hacks_end   = list_of_hacks.end();

  for (auto const &name : engage_args) {
    auto itr = std::find_if(list_of_hacks.begin(), list_of_hacks.end(), [&name](auto const &hack) { return hack.name == name; });

    if (itr != hacks_end)
      s_engaged_hacks[std::distance(hacks_begin, itr)] = true;
    else
      mxerror(fmt::format(FY("'{0}' is not a valid hack.\n"), name));
  }
}

void
init() {
  std::vector<std::string> env_vars = { "MKVTOOLNIX_ENGAGE", "MTX_ENGAGE", balg::to_upper_copy(get_program_name()) + "_ENGAGE" };

  for (auto &name : env_vars) {
    auto value = getenv(name.c_str());
    if (value)
      engage(value);
  }
}

}
