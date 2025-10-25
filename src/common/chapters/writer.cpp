/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   XML chapter writer functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/strings/utf8.h"
#include "common/timestamp.h"

namespace mtx::chapters {

using chapter_entry_c         = std::pair<timestamp_c, std::string>;
using chapter_entry_storage_c = std::vector<chapter_entry_c>;

static void
handle_atom(libmatroska::KaxChapterAtom const &atom,
            chapter_entry_storage_c &chapter_entries,
            mtx::bcp47::language_c const &language_to_extract) {
  if (find_child_value<libmatroska::KaxChapterFlagHidden>(atom) != 0)
    return;

  if (find_child_value<libmatroska::KaxChapterFlagEnabled>(atom, 1llu) != 1)
    return;

  auto start = timestamp_c::ns(find_child_value<libmatroska::KaxChapterTimeStart>(atom));
  auto name  = std::string{};

  for (auto const &child : atom) {
    auto display = dynamic_cast<libmatroska::KaxChapterDisplay const *>(child);
    if (!display)
      continue;

    auto language = mtx::bcp47::language_c::parse(find_child_value<libmatroska::KaxChapterLanguage>(display, "eng"s));
    if (language_to_extract.is_valid() && (language != language_to_extract))
      continue;

    name = to_utf8(find_child_value<libmatroska::KaxChapterString>(display));
    break;
  }

  if (name.empty())
    name = "–";

  chapter_entries.emplace_back(start, name);
}

std::size_t
write_simple(libmatroska::KaxChapters &chapters,
             mm_io_c &out,
             mtx::bcp47::language_c const &language_to_extract) {
  auto chapter_entries = chapter_entry_storage_c{};

  for (auto const &chapters_child : chapters) {
    auto edition = dynamic_cast<libmatroska::KaxEditionEntry const *>(chapters_child);
    if (!edition)
      continue;

    for (auto const &edition_child : *edition) {
      auto atom = dynamic_cast<libmatroska::KaxChapterAtom const *>(edition_child);
      if (atom)
        handle_atom(*atom, chapter_entries, language_to_extract);
    }
  }

  std::sort(chapter_entries.begin(), chapter_entries.end());

  auto chapter_num = 0u;

  for (auto const &entry : chapter_entries) {
    ++chapter_num;

    out.puts(g_cc_stdio->native(fmt::format("CHAPTER{0:02}={1:02}:{2:02}:{3:02}.{4:03}\n", chapter_num, entry.first.to_h(), entry.first.to_m() % 60, entry.first.to_s() % 60, entry.first.to_ms() % 1000)));
    out.puts(g_cc_stdio->native(fmt::format("CHAPTER{0:02}NAME={1}\n",                     chapter_num, entry.second)));
  }

  return chapter_num;
}

}
