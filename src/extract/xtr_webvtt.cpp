/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/ebml.h"
#include "common/webvtt.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "extract/xtr_webvtt.h"

xtr_webvtt_c::xtr_webvtt_c(const std::string &codec_id,
                     int64_t tid,
                     track_spec_t &tspec)
  : xtr_base_c{codec_id, tid, tspec}
{
}

void
xtr_webvtt_c::create_file(xtr_base_c *master,
                          libmatroska::KaxTrackEntry &track) {
  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (!priv)
    mxerror(fmt::format(FY("Track {0} with the CodecID '{1}' is missing the \"codec private\" element and cannot be extracted.\n"), m_tid, m_codec_id));

  xtr_base_c::create_file(master, track);

  m_out->write_bom("UTF-8");

  auto global = mtx::string::chomp(mtx::string::normalize_line_endings(decode_codec_private(priv)->to_string())) + "\n";
  m_out->write(global);
}

void
xtr_webvtt_c::handle_frame(xtr_frame_t &f) {
  ++m_num_entries;

  if (-1 == f.duration) {
    mxwarn(fmt::format(FY("Track {0}: Subtitle entry number {1} is missing its duration. Assuming a duration of 1s.\n"), m_tid, m_num_entries));
    f.duration = 1000000000;
  }

  std::string label, settings_list, local_blocks;

  if (f.additions) {
    auto &block_more    = get_child<libmatroska::KaxBlockMore>(f.additions);
    auto block_addition = find_child<libmatroska::KaxBlockAdditional>(block_more);

    if (block_addition) {
      auto content = std::string{reinterpret_cast<char const *>(block_addition->GetBuffer()), static_cast<std::string::size_type>(block_addition->GetSize())};
      auto lines   = mtx::string::split(mtx::string::chomp(mtx::string::normalize_line_endings(content)), "\n", 3);

      if ((lines.size() > 0) && !lines[0].empty())
        settings_list = " "s + mtx::string::strip_copy(lines[0]);

      if ((lines.size() > 1) && !lines[1].empty())
        label = mtx::string::strip_copy(lines[1]) + "\n";

      if ((lines.size() > 2) && !lines[2].empty())
        local_blocks = mtx::string::chomp(lines[2]) + "\n\n";
    }
  }

  auto content = mtx::string::chomp(mtx::string::normalize_line_endings(f.frame->to_string())) + "\n";
  content      = mtx::webvtt::parser_c::adjust_embedded_timestamps(content, timestamp_c::ns(f.timestamp));
  content      = fmt::format("\n{0}{1}{2} --> {3}{4}\n{5}",
                             local_blocks, label,
                             mtx::string::format_timestamp(f.timestamp, 3), mtx::string::format_timestamp(f.timestamp + f.duration, 3),
                             settings_list, content);

  m_out->write(content);
}
