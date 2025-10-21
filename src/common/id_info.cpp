/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/id_info.h"
#include "common/strings/formatting.h"

namespace mtx::id {

info_c &
info_c::add(std::string const &key,
            memory_cptr const &value) {
  if (value && value->get_size())
    set(key, mtx::string::to_hex(value->get_buffer(), value->get_size(), true));

  return *this;
}

info_c &
info_c::add(std::string const &key,
            memory_c const *value) {
  if (value && value->get_size())
    set(key, mtx::string::to_hex(value->get_buffer(), value->get_size(), true));

  return *this;
}

} // namespace mtx::id
