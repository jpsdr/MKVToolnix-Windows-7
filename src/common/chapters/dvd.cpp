/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  helper functions for chapters on DVDs

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(HAVE_DVDREAD)

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>

#include "common/at_scope_exit.h"
#include "common/chapters/chapters.h"
#include "common/chapters/dvd.h"
#include "common/regex.h"
#include "common/strings/parsing.h"
#include "common/timestamp.h"

namespace mtx::chapters {

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

  auto error = fmt::format(Y("Could not open '{0}' for reading.\n"), file_name);

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
    auto overall_timestamp = timestamp_c::ns(0);
    auto ttn               = vmg->tt_srpt->title[title].vts_ttn;
    auto vts_ptt_srpt      = vts->vts_ptt_srpt;

    for (auto chapter = 0; chapter < vmg->tt_srpt->title[title].nr_of_ptts - 1; chapter++) {
      auto pgc_id        = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgcn;
      auto pgn           = vts_ptt_srpt->title[ttn - 1].ptt[chapter].pgn;
      auto cur_pgc       = vts->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
      auto start_cell    = cur_pgc->program_map[pgn - 1] - 1;
      pgc_id             = vts_ptt_srpt->title[ttn - 1].ptt[chapter + 1].pgcn;
      pgn                = vts_ptt_srpt->title[ttn - 1].ptt[chapter + 1].pgn;
      cur_pgc            = vts->vts_pgcit->pgci_srp[pgc_id - 1].pgc;
      auto end_cell      = cur_pgc->program_map[pgn - 1] - 2;
      auto cur_timestamp = timestamp_c::ns(0);

      for (auto cur_cell = start_cell; cur_cell <= end_cell; cur_cell++) {
        auto dt        = &cur_pgc->cell_playback[cur_cell].playback_time;
        auto fps       = ((dt->frame_u & 0xc0) >> 6) == 1 ? 25.00 : 29.97;
        auto hour      = ((dt->hour    & 0xf0) >> 4) * 10ull + (dt->hour    & 0x0f);
        auto minute    = ((dt->minute  & 0xf0) >> 4) * 10ull + (dt->minute  & 0x0f);
        auto second    = ((dt->second  & 0xf0) >> 4) * 10ull + (dt->second  & 0x0f);
        auto frames    = ((dt->frame_u & 0x30) >> 4) * 10ull + (dt->frame_u & 0x0f);

        cur_timestamp += timestamp_c::ms((hour * 60 * 60 * 1000) + (minute * 60 * 1000) + (second * 1000) + (frames * 1000.0 / fps));
      }

      timestamps.emplace_back(overall_timestamp);

      overall_timestamp += cur_timestamp;
    }

    timestamps.emplace_back(overall_timestamp);
  }

  return titles_and_timestamps;
}

std::shared_ptr<libmatroska::KaxChapters>
maybe_parse_dvd(std::string const &file_name,
                mtx::bcp47::language_c const &language) {
  auto title             = 1u;
  auto cleaned_file_name = file_name;
  mtx::regex::jp::VecNum matches;

  if (mtx::regex::match(cleaned_file_name, matches, mtx::regex::jp::Regex{"(.+):([0-9]+)$"})) {
    cleaned_file_name = matches[0][1];

    if (!mtx::string::parse_number(matches[0][2], title) || (title < 1))
      throw parser_x{fmt::format(Y("'{0}' is not a valid DVD title number."), matches[0][2])};
  }

  auto dvd_dir = bfs::path{cleaned_file_name};

  if (mtx::regex::match(cleaned_file_name, mtx::regex::jp::Regex{"\\.(bup|ifo|vob)$", "i"}))
    dvd_dir = dvd_dir.parent_path();

  else if (   !bfs::exists(dvd_dir)
           || !bfs::is_directory(dvd_dir)
           || (   !bfs::exists(dvd_dir / "VIDEO_TS.IFO")
               && !bfs::exists(dvd_dir / "VIDEO_TS" / "VIDEO_TS.IFO")))
    return {};

  auto titles_and_timestamps = parse_dvd(dvd_dir.string());

  if (title > titles_and_timestamps.size())
    throw parser_x{fmt::format(Y("The title number '{0}' is higher than the number of titles on the DVD ({1})."), title, titles_and_timestamps.size())};

  return create_editions_and_chapters({ titles_and_timestamps[title - 1] }, language, {});
}

}

#endif  // HAVE_DVDREAD
