/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definition of functions for converting between Vorbis comments and Matroska tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/base64.h"
#include "common/debugging.h"
#include "common/iso639.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/tags/target_type.h"
#include "common/tags/vorbis.h"

#include <matroska/KaxTags.h>
#include <matroska/KaxTag.h>

namespace mtx::tags {

namespace {
debugging_option_c s_debug{"vorbis_comments|vorbis_tags"};

std::unordered_map<std::string, std::pair<std::string, int>> s_vorbis_to_matroska;
std::unordered_map<int, std::unordered_map<std::string, std::string>> s_matroska_to_vorbis;

void
init_vorbis_to_matroska() {
  if (!s_vorbis_to_matroska.empty())
    return;

  // Official "minimal list of standard field names" from the Vorbis
  // comment spec at https://www.xiph.org/vorbis/doc/v-comment.html
  s_vorbis_to_matroska["ALBUM"s]                 = { "ALBUM"s,              Track };
  s_vorbis_to_matroska["COPYRIGHT"s]             = { "COPYRIGHT"s,          Track };
  s_vorbis_to_matroska["COPYRIGHT"s]             = { "COPYRIGHT"s,          Track };
  s_vorbis_to_matroska["DATE"s]                  = { "DATE_RECORDED"s,      Track };
  s_vorbis_to_matroska["DESCRIPTION"s]           = { "DESCRIPTION"s,        Track };
  s_vorbis_to_matroska["GENRE"s]                 = { "GENRE"s,              Track };
  s_vorbis_to_matroska["ISRC"s]                  = { "ISRC"s,               Track };
  s_vorbis_to_matroska["LICENSE"s]               = { "LICENSE"s,            Track };
  s_vorbis_to_matroska["LOCATION"s]              = { "RECORDING_LOCATION"s, Track };
  s_vorbis_to_matroska["ORGANIZATION"s]          = { "LABEL"s,              Track };
  s_vorbis_to_matroska["PERFORMER"s]             = { "LEAD_PERFORMER"s,     Track };
  s_vorbis_to_matroska["TRACKNUMBER"s]           = { "PART_NUMBER"s,        Track };
  // CONTACT ?
  // TITLE handled elsewhere
  // VERSION ?

  // Other commonly found field names
  s_vorbis_to_matroska["ACTOR"s]                 = { "ACTOR"s,              Track };
  s_vorbis_to_matroska["ALBUMARTIST"s]           = { "ARTIST"s,             Album };
  s_vorbis_to_matroska["ARTIST"s]                = { "ARTIST"s,             Track };
  s_vorbis_to_matroska["COMMENT"s]               = { "COMMENT"s,            Track };
  s_vorbis_to_matroska["COMPOSER"s]              = { "COMPOSER"s,           Track };
  s_vorbis_to_matroska["DIRECTOR"s]              = { "DIRECTOR"s,           Track };
  s_vorbis_to_matroska["ENCODED_BY"s]            = { "ENCODED_BY"s,         Track };
  s_vorbis_to_matroska["ENCODED_USING"s]         = { "ENCODED_USING"s,      Track };
  s_vorbis_to_matroska["ENCODER"s]               = { "ENCODER"s,            Track };
  s_vorbis_to_matroska["ENCODER_OPTIONS"s]       = { "ENCODER_OPTIONS"s,    Track };
  s_vorbis_to_matroska["PRODUCER"s]              = { "PRODUCER"s,           Track };
  s_vorbis_to_matroska["REPLAYGAIN_ALBUM_GAIN"s] = { "REPLAYGAIN_GAIN"s,    Album };
  s_vorbis_to_matroska["REPLAYGAIN_ALBUM_PEAK"s] = { "REPLAYGAIN_PEAK"s,    Album };
  s_vorbis_to_matroska["REPLAYGAIN_TRACK_GAIN"s] = { "REPLAYGAIN_GAIN"s,    Track };
  s_vorbis_to_matroska["REPLAYGAIN_TRACK_PEAK"s] = { "REPLAYGAIN_PEAK"s,    Track };
  // COVERART
  // COVERARTMIME
}

void
init_matroska_to_vorbis() {
  if (!s_matroska_to_vorbis.empty())
    return;

  init_vorbis_to_matroska();

  for (auto const &[vorbis, matroska] : s_vorbis_to_matroska)
    s_matroska_to_vorbis[matroska.second][matroska.first] = vorbis;
}

std::string
parse_language(std::string key_language) {
  auto index = map_to_iso639_2_code(key_language, true);
  if (-1 != index)
    return g_iso639_languages[index].iso639_2_code;

  boost::smatch matches;
  if (   boost::regex_search(key_language, matches, boost::regex{".*\\[(.+?)\\]", boost::regex::perl})
      && ((index = map_to_iso639_2_code(boost::to_lower_copy(matches[1].str()), true)) != -1))
    return g_iso639_languages[index].iso639_2_code;

  if (   boost::regex_search(key_language, matches, boost::regex{".*\\((.+?)\\)", boost::regex::perl})
      && ((index = map_to_iso639_2_code(boost::to_lower_copy(matches[1].str()), true)) != -1))
    return g_iso639_languages[index].iso639_2_code;

  return {};
}

std::shared_ptr<attachment_t>
parse_metadata_block_picture(std::string const &value) {
  // 3|image/jpeg||600x600x24|<36510 bytes of image data>
  auto parts = split(value, "|", 5);

  mxdebug_if(s_debug, fmt::format("parse_metadata_block_picture: num parts {}\n", parts.size()));

  if (parts.size() != 5)
    return {};

  try {
    auto picture       = std::make_shared<attachment_t>();
    picture->mime_type = parts[1];
    auto extension     = picture->mime_type == "image/jpeg"     ? "jpg"
                       : picture->mime_type == "image/png"      ? "png"
                       : picture->mime_type == "image/x-ms-bmp" ? "bmp"
                       :                                         ""s;
    picture->name      = fmt::format("cover.{}", extension);
    picture->data      = mtx::base64::decode(parts[4]);

    mxdebug_if(s_debug, fmt::format("parse_metadata_block_picture: MIME type: {} name: {} data size: {}\n", picture->mime_type, picture->name, picture->data ? picture->data->get_size() : 0));

    if (!picture->mime_type.empty() && !extension.empty() && picture->data && picture->data->get_size())
      return picture;

  } catch (mtx::base64::exception const &ex) {
    mxdebug_if(s_debug, fmt::format("parse_metadata_block_picture: Base64 exception: {}\n", ex.what()));
  }

  return {};
}

} // anonymous namespace

converted_vorbis_comments_t
from_vorbis_comments(std::vector<std::string> const &vorbis_comments) {
  init_vorbis_to_matroska();

  converted_vorbis_comments_t converted;
  converted.m_tags = std::make_shared<libmatroska::KaxTags>();

  for (auto const &line : vorbis_comments) {
    mxdebug_if(s_debug, fmt::format("from_vorbis_comments: parsing {}\n", elide_string(line)));

    auto key_value = split(line, "=", 2);
    if (key_value.size() != 2)
      continue;

    auto key_name_language                  = split(key_value[0], "-", 2);
    auto vorbis_key                         = balg::to_upper_copy(key_name_language[0]);
    auto const &[matroska_key, target_type] = s_vorbis_to_matroska[vorbis_key];
    auto &value                             = key_value[1];

    if (vorbis_key == "TITLE") {
      converted.m_title = key_value[1];
      continue;
    }

    if (vorbis_key == "LANGUAGE") {
      auto new_language = parse_language(value);
      if (!new_language.empty())
        converted.m_language = new_language;
      continue;
    }

    if (vorbis_key == "METADATA_BLOCK_PICTURE") {
      auto picture = parse_metadata_block_picture(value);
      if (picture)
        converted.m_pictures.push_back(picture);
      continue;
    }

    if (matroska_key.empty())
      continue;


  }

  return converted;
}

}
