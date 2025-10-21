/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  helper functions for chapters on DVDs

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#if defined(HAVE_DVDREAD)

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>

#include <QRegularExpression>

#include "common/at_scope_exit.h"
#include "common/chapters/chapters.h"
#include "common/chapters/dvd.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/parsing.h"
#include "common/timestamp.h"

namespace mtx::chapters {

namespace {

timestamp_c
frames_to_timestamp_ns(unsigned int num_frames,
                       unsigned int fps) {
  auto factor = fps == 30 ? 1001 : 1000;
  return timestamp_c::ns(1000000ull * factor * num_frames / (fps ? fps : 1));
}

} // anonymous namespace


std::vector<std::vector<timestamp_c>>
parse_dvd(std::string const &file_name) {
  dvd_reader_t *dvd{};
  ifo_handle_t *vmg{};

  at_scope_exit_c global_cleanup{[dvd, vmg]() {
    if (vmg)
      ifoClose(vmg);
    if (dvd)
      DVDClose(dvd);
  }};

  auto error = fmt::format(FY("Could not open '{0}' for reading.\n"), file_name);

  dvd = DVDOpen(file_name.c_str());
  if (!dvd)
    throw parser_x{error};

  vmg = ifoOpen(dvd, 0);
  if (!vmg)
    throw parser_x{error};

  std::vector<std::vector<timestamp_c>> titles_and_timestamps;
  titles_and_timestamps.reserve(vmg->tt_srpt->nr_of_srpts);

  for (auto title = 0; title < vmg->tt_srpt->nr_of_srpts; ++title) {
    auto vts = ifoOpen(dvd, vmg->tt_srpt->title[title].title_set_nr);

    at_scope_exit_c title_cleanup{[vts]() {
      if (vts)
        ifoClose(vts);
    }};

    if (!vts)
      throw parser_x{error};

    titles_and_timestamps.emplace_back();

    auto &timestamps       = titles_and_timestamps.back();
    auto ttn               = vmg->tt_srpt->title[title].vts_ttn;
    auto vts_ptt_srpt      = vts->vts_ptt_srpt;
    auto overall_frames    = 0u;
    auto fps               = 0u; // This should be consistent as DVDs are either NTSC or PAL

    for (auto chapter = 0; chapter < vmg->tt_srpt->title[title].nr_of_ptts - 1; chapter++) {
      auto pgc_id        = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn;
      auto pgn           = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgn;
      auto cur_pgc       = vts->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
      auto start_cell    = cur_pgc->program_map[pgn - 1] - 1;
      pgc_id             = vts_ptt_srpt->title[ttn - 1].ptt[chapter + 1].pgcn;
      pgn                = vts_ptt_srpt->title[ttn - 1].ptt[chapter + 1].pgn;
      cur_pgc            = vts->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
      auto end_cell      = cur_pgc->program_map[pgn - 1] - 2;
      auto cur_frames    = 0u;

      for (auto cur_cell = start_cell; cur_cell <= end_cell; cur_cell++) {
        auto dt        = &cur_pgc->cell_playback[cur_cell].playback_time;
        auto hour      = ((dt->hour    & 0xf0) >> 4) * 10 + (dt->hour    & 0x0f);
        auto minute    = ((dt->minute  & 0xf0) >> 4) * 10 + (dt->minute  & 0x0f);
        auto second    = ((dt->second  & 0xf0) >> 4) * 10 + (dt->second  & 0x0f);
        fps            = ((dt->frame_u & 0xc0) >> 6) == 1 ? 25 : 30; // by definition
        cur_frames    += ((hour * 60 * 60) + minute * 60 + second) * fps;
        cur_frames    += ((dt->frame_u & 0x30) >> 4) * 10 + (dt->frame_u & 0x0f);
      }

      timestamps.emplace_back(frames_to_timestamp_ns(overall_frames, fps));

      overall_frames += cur_frames;
    }

    timestamps.emplace_back(frames_to_timestamp_ns(overall_frames, fps));
  }

  return titles_and_timestamps;
}

std::shared_ptr<libmatroska::KaxChapters>
maybe_parse_dvd(std::string const &file_name,
                mtx::bcp47::language_c const &language) {
  auto title             = 1u;
  auto cleaned_file_name = file_name;
  auto matches           = QRegularExpression{"(.+):([0-9]+)$"}.match(Q(cleaned_file_name));

  if (matches.hasMatch()) {
    cleaned_file_name = to_utf8(matches.captured(1));

    if (!mtx::string::parse_number(to_utf8(matches.captured(2)), title) || (title < 1))
      throw parser_x{fmt::format(FY("'{0}' is not a valid DVD title number."), to_utf8(matches.captured(2)))};
  }

  auto dvd_dir = mtx::fs::to_path(cleaned_file_name);

  if (Q(cleaned_file_name).contains(QRegularExpression{"\\.(bup|ifo|vob)$", QRegularExpression::CaseInsensitiveOption}))
    dvd_dir = dvd_dir.parent_path();

  else if (   !boost::filesystem::is_directory(dvd_dir)
           || (   !boost::filesystem::is_regular_file(dvd_dir / "VIDEO_TS.IFO")
               && !boost::filesystem::is_regular_file(dvd_dir / "VIDEO_TS" / "VIDEO_TS.IFO")))
    return {};

  auto titles_and_timestamps = parse_dvd(dvd_dir.string());

  if (title > titles_and_timestamps.size())
    throw parser_x{fmt::format(FY("The title number '{0}' is higher than the number of titles on the DVD ({1})."), title, titles_and_timestamps.size())};

  return create_editions_and_chapters({ titles_and_timestamps[title - 1] }, language, {});
}

}

#endif  // HAVE_DVDREAD
