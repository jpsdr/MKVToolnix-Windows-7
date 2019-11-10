/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definition of functions for converting between Vorbis comments and Matroska tags

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/attachment.h"

namespace libmatroska {
class KaxTags;
}

namespace mtx::tags {

struct vorbis_comments_t {
  enum type_e {
    Unknown,
    Vorbis,
    VP_8_9,
    Opus,
  };

  type_e m_type{type_e::Unknown};
  std::string m_vendor;
  std::vector<std::pair<std::string, std::string>> m_comments;

  bool valid() const {
    return type_e::Unknown != m_type;
  }
};

struct converted_vorbis_comments_t {
  std::string m_title, m_language;
  std::shared_ptr<libmatroska::KaxTags> m_tags;
  std::vector<std::shared_ptr<attachment_t>> m_pictures;
};

converted_vorbis_comments_t from_vorbis_comments(vorbis_comments_t const &vorbis_comments);

vorbis_comments_t parse_vorbis_comments_from_packet(memory_cptr const &packet);

}
