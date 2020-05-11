/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  Blu-ray disc library meta data handling

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bluray/track_chapter_names.h"
#include "common/bluray/util.h"
#include "common/debugging.h"
#include "common/xml/xml.h"

namespace mtx::bluray::track_chapter_names {

namespace {
debugging_option_c debug{"track_chapter_names|tnmt"};

std::vector<std::string>
parse_tnmt_xml(bfs::path const &file_name) {
  try {
    auto doc = mtx::xml::load_file(file_name.string());

    if (!doc) {
      mxdebug_if(debug, fmt::format("{}: could not load the XML file\n", file_name));
      return {};
    }

    auto root_node = doc->document_element();
    if (!root_node || (std::string{root_node.name()} != "chapters")) {
      mxdebug_if(debug, fmt::format("{}: no root node found or wrong name: {}\n", file_name, root_node ? root_node.name() : "<none>"));
      return {};
    }

    std::vector<std::string> names;

    for (auto node = root_node.child("name"); node; node = node.next_sibling("name"))
      names.push_back(node.child_value());

    mxdebug_if(debug, fmt::format("{}: file parsed, number of chapter names: {}\n", file_name, names.size()));

    return names;

  } catch (mtx::exception const &ex) {
    mxdebug_if(debug, fmt::format("{}: exception: {}\n", file_name, ex));
  }

  return {};
}

}

// ------------------------------------------------------------

// ------------------------------------------------------------

all_chapter_names_t
locate_and_parse_for_title(bfs::path const &location,
                           std::string const &title_number) {
  auto base_dir = mtx::bluray::find_base_dir(location);
  if (base_dir.empty())
    return {};

  auto track_chapter_names_dir = base_dir / "META" / "TN";
  if (!bfs::exists(track_chapter_names_dir) || !bfs::is_directory(track_chapter_names_dir))
    return {};

  mxdebug_if(debug, fmt::format("found TN directory at {}\n", track_chapter_names_dir));

  std::regex tnmt_re{fmt::format("tnmt_([a-z]{{3}})_{}\\.xml", title_number)};

  std::vector<chapter_names_t> chapter_names;

  for (bfs::directory_iterator dir_itr{track_chapter_names_dir}, end_itr; dir_itr != end_itr; ++dir_itr) {
    std::smatch matches;
    auto entry_name = dir_itr->path().filename().string();
    if (!std::regex_match(entry_name, matches, tnmt_re))
      continue;

    auto language = matches[1].str();

    mxdebug_if(debug, fmt::format("found TNMT file for language {}\n", language));

    auto names = parse_tnmt_xml(*dir_itr);
    if (!names.empty())
      chapter_names.emplace_back(language, names);
  }

  std::sort(chapter_names.begin(), chapter_names.end());

  return chapter_names;
}

void
dump(all_chapter_names_t const &list) {
  for (auto const &names : list) {
    mxinfo(fmt::format("{} ({} entries):\n", names.first, names.second.size()));
    for (auto const &name : names.second)
      mxinfo(fmt::format("  {}\n", name));
  }
}

}
