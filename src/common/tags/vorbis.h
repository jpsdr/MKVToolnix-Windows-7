/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definition of functions for converting between Vorbis comments and Matroska tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/attachment.h"
#include "common/bcp47.h"

namespace libmatroska {
class KaxTags;
}

namespace mtx::tags {

struct vorbis_comments_t {
  enum class type_e {
    Unknown,
    Vorbis,
    VP8,
    VP9,
    Opus,
    Other,
  };

  type_e m_type{type_e::Unknown};
  std::string m_vendor;
  std::vector<std::pair<std::string, std::string>> m_comments;

  bool valid() const {
    return type_e::Unknown != m_type;
  }
};

struct converted_vorbis_comments_t {
  std::string m_title;
  mtx::bcp47::language_c m_language;
  std::shared_ptr<libmatroska::KaxTags> m_track_tags, m_album_tags;
  std::vector<std::shared_ptr<attachment_t>> m_pictures;
};

converted_vorbis_comments_t from_vorbis_comments(vorbis_comments_t const &vorbis_comments);

vorbis_comments_t parse_vorbis_comments_from_packet(memory_c const &packet, std::optional<unsigned int> offset = std::nullopt);
memory_cptr assemble_vorbis_comments_into_packet(vorbis_comments_t const &comments);

}
