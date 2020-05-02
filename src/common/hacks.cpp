/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   hacks :)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/base64.h"
#include "common/hacks.h"
#include "common/strings/editing.h"

namespace mtx::hacks {

namespace {
std::vector<std::string> const &
get_list() {
  static std::vector<std::string> s_hacks;

  if (s_hacks.empty()) {
    s_hacks = {
      "space_after_chapters",
      "no_chapters_in_meta_seek",
      "no_meta_seek",
      "lacing_xiph",
      "lacing_ebml",
      "native_mpeg4",
      "no_variable_data",
      "force_passthrough_packetizer",
      "write_headers_twice",
      "allow_avc_in_vfw_mode",
      "keep_bitstream_ar_info",
      "no_simpleblocks",
      "use_codec_state_only",
      "enable_timestamp_warning",
      "remove_bitstream_ar_info",
      "vobsub_subpic_stop_cmds",
      "no_cue_duration",
      "no_cue_relative_position",
      "no_delay_for_garbage_in_avi",
      "keep_last_chapter_in_mpls",
      "keep_track_statistics_tags",
      "all_i_slices_are_key_frames",
      "append_and_split_flac",
    };
  }

  return s_hacks;
}

std::vector<bool> s_engaged_hacks(MAX_IDX + 1, false);
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
  std::vector<std::string> engage_args = split(hacks, ",");
  size_t aidx;

  auto &list = get_list();

  for (aidx = 0; engage_args.size() > aidx; aidx++)
    if (engage_args[aidx] == "list") {
      mxinfo(Y("Valid hacks are:\n"));

      for (auto const &hack : list)
        mxinfo(fmt::format("{0}\n", hack));
      mxexit();

    } else if (engage_args[aidx] == "cow") {
      const std::string initial = "ICAgICAgICAgIChfXykKICAgICAgICAgICgqKikg"
        "IE9oIGhvbmV5LCB0aGF0J3Mgc28gc3dlZXQhCiAgIC8tLS0tLS0tXC8gICBPZiB"
        "jb3Vyc2UgSSdsbCBtYXJyeSB5b3UhCiAgLyB8ICAgICB8fAogKiAgfHwtLS0tfH"
        "wKICAgIF5eICAgIF5eCg==";
      auto correction = mtx::base64::decode(initial);
      mxinfo(correction->to_string());
      mxexit();
    }

  for (aidx = 0; engage_args.size() > aidx; aidx++) {
    auto itr = brng::find(list, engage_args[aidx]);

    if (itr != list.end())
      s_engaged_hacks[std::distance(list.begin(), itr)] = true;
    else
      mxerror(fmt::format(Y("'{0}' is not a valid hack.\n"), engage_args[aidx]));
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
