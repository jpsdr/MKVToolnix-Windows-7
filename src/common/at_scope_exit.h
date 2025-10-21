/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   run code at scope exit

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx {

class at_scope_exit_c {
private:
  std::function<void()> m_code;
public:
  at_scope_exit_c(const std::function<void()> &code) : m_code(code) {}
  ~at_scope_exit_c() {
    m_code();
  }
};

}
