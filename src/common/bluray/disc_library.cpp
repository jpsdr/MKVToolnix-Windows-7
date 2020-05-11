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

#include "common/bluray/util.h"
#include "common/bluray/disc_library.h"
#include "common/debugging.h"
#include "common/image.h"
#include "common/strings/parsing.h"
#include "common/xml/xml.h"

namespace mtx::bluray::disc_library {

namespace {
debugging_option_c debug{"disc_library|bdmt"};

std::optional<info_t>
parse_bdmt_xml(bfs::path const &file_name) {
  try {
    auto doc = mtx::xml::load_file(file_name.string());

    if (!doc) {
      mxdebug_if(debug, fmt::format("{}: could not load the XML file\n", file_name));
      return {};
    }

    auto root_node = doc->document_element();
    if (!root_node || (std::string{root_node.name()} != "disclib")) {
      mxdebug_if(debug, fmt::format("{}: no root node found or wrong name: {}\n", file_name, root_node ? root_node.name() : "<none>"));
      return {};
    }

    info_t info;

    info.m_title = root_node.child("di:discinfo").child("di:title").child("di:name").child_value();

    std::regex size_re{"([0-9]+)x([0-9]+)"};

    for (auto node = root_node.child("di:discinfo").child("di:description").child("di:thumbnail"); node; node = node.next_sibling("di:thumbnail")) {
      thumbnail_t thumbnail;

      thumbnail.m_file_name = bfs::path{node.attribute("href").value()};
      auto size             = std::string{node.attribute("size").value()};
      auto ok               = false;

      if (thumbnail.m_file_name.is_relative())
        thumbnail.m_file_name = (bfs::absolute(file_name).parent_path() / thumbnail.m_file_name).lexically_normal();

      std::smatch matches;

      if (   std::regex_match(size, matches, size_re)
          && parse_number(matches[1].str(), thumbnail.m_width)
          && parse_number(matches[2].str(), thumbnail.m_height))
        ok = true;

      if (!ok) {
        auto dimensions = mtx::image::get_size(thumbnail.m_file_name);
        if (dimensions) {
          thumbnail.m_width  = dimensions->first;
          thumbnail.m_height = dimensions->second;
          ok                 = true;
        }
      }

      if (ok) {
        info.m_thumbnails.push_back(thumbnail);
        mxdebug_if(debug, fmt::format("{}: file name: {} width: {} height: {}\n", file_name, thumbnail.m_file_name.string(), thumbnail.m_width, thumbnail.m_height));
      }
    }

    mxdebug_if(debug, fmt::format("{}: file parsed, title: {}; number of thumbnails: {}\n", file_name, info.m_title, info.m_thumbnails.size()));

    return info;

  } catch (mtx::exception const &ex) {
    mxdebug_if(debug, fmt::format("{}: exception: {}\n", file_name, ex));
  }

  return {};
}

}

// ------------------------------------------------------------

void
info_t::dump()
  const {
  mxinfo(fmt::format("    title: {}\n", m_title));
  for (auto const &thumbnail : m_thumbnails)
    mxinfo(fmt::format("    thumbnail: {}x{} @ {}\n", thumbnail.m_width, thumbnail.m_height, thumbnail.m_file_name.string()));
}

// ------------------------------------------------------------

void
disc_library_t::dump()
  const {
  std::vector<std::string> languages;
  for (auto const &elt : m_infos_by_language)
    languages.emplace_back(elt.first);

  std::sort(languages.begin(), languages.end());

  mxinfo(fmt::format("Disc library dump:\n"));

  for (auto const &language : languages) {
    mxinfo(fmt::format("  {}:\n", language));
    m_infos_by_language.at(language).dump();
  }
}

// ------------------------------------------------------------

std::optional<disc_library_t>
locate_and_parse(bfs::path const &location) {
  auto base_dir = mtx::bluray::find_base_dir(location);
  if (base_dir.empty())
    return {};

  auto disc_library_dir = base_dir / "META" / "DL";
  if (!bfs::exists(disc_library_dir) || !bfs::is_directory(disc_library_dir))
    return {};

  mxdebug_if(debug, fmt::format("found DL directory at {}\n", disc_library_dir));

  std::regex bdmt_re{"bdmt_([a-z]{3})\\.xml"};
  std::smatch matches;

  disc_library_t lib;

  for (bfs::directory_iterator dir_itr{disc_library_dir}, end_itr; dir_itr != end_itr; ++dir_itr) {
    auto other_file_name = dir_itr->path().filename().string();
    if (!std::regex_match(other_file_name, matches, bdmt_re))
      continue;

    auto language = matches[1].str();

    mxdebug_if(debug, fmt::format("found BDMT file for language {}\n", matches[1].str()));

    auto info = parse_bdmt_xml(*dir_itr);
    if (info)
      lib.m_infos_by_language[language] = *info;
  }

  if (lib.m_infos_by_language.empty())
    return {};

  return lib;
}

std::optional<info_t>
locate_and_parse_for_language(bfs::path const &location,
                              std::string const &language) {
  auto base_dir = mtx::bluray::find_base_dir(location);
  if (base_dir.empty())
    return {};

  auto disc_library_file = base_dir / "META" / "DL" / fmt::format("bdmt_{}.xml", language);
  if (!bfs::exists(disc_library_file))
    return {};

  mxdebug_if(debug, fmt::format("found DL file for language {} at {}\n", language, disc_library_file));

  return parse_bdmt_xml(disc_library_file);
}

}
