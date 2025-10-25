/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for helper functions for unit tests

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wsign-compare"
# include "gtest/gtest.h"
# pragma GCC diagnostic pop
#else
# include "gtest/gtest.h"
#endif

extern bool g_warning_issued;

namespace mtxut {

class mxerror_x: public std::exception {
protected:
  std::string m_message;

public:
  mxerror_x(std::string const &message)
    : m_message{message}
  {
  }

  virtual ~mxerror_x() throw() { }


  virtual const char *what() const throw() {
    return m_message.c_str();
  }

  virtual std::string error() const throw() {
    return m_message;
  }
};

void init_suite(char const *argv0);
void init_case();

}
