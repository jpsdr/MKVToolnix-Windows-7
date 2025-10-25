/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   exception definitions for AAC data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/error.h"

namespace mtx::aac {

class unsupported_feature_x: public mtx::exception {
private:
  std::string const m_error;

public:
  unsupported_feature_x(std::string const &feature)
    : m_error{fmt::format("unsupported feature \"{0}\"", feature)}
  {
  }

  unsupported_feature_x(char const *feature)
    : m_error{fmt::format("unsupported feature \"{0}\"", feature)}
  {
  }

  virtual char const *what() const throw() override {
    return m_error.c_str();
  }

  virtual std::string error() const throw() override {
    return m_error;
  }
};


} // namespace mtx::aac
