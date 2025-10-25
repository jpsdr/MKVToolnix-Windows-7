/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   version information

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <boost/operators.hpp>

#include <QDateTime>

#include "common/xml/xml.h"

struct version_number_t: boost::totally_ordered<version_number_t> {
  std::vector<unsigned int> parts;
  unsigned int build{};
  bool valid{};

  version_number_t() = default;
  version_number_t(const std::string &s);

  bool operator <(const version_number_t &cmp) const;
  bool operator ==(const version_number_t &cmp) const;
  int compare(const version_number_t &cmp) const;

  std::string to_string() const;
};

struct mtx_release_version_t {
  version_number_t current_version, latest_source, latest_windows_build;
  std::map<std::string, std::string> urls;
  bool valid;

  mtx_release_version_t();
};

enum version_info_flags_e {
  vif_untranslated = 0x0001,
  vif_architecture = 0x0002,

  vif_none         = 0x0000,
  vif_default      = vif_architecture,
  vif_full         = (0xffff & ~vif_untranslated),
};

struct segment_info_data_t {
  std::string muxing_app, writing_app;
  QDateTime writing_date;
};

std::string get_version_info(const std::string &program, version_info_flags_e flags = vif_default);
int compare_current_version_to(const std::string &other_version_str);
version_number_t get_current_version();
mtx_release_version_t parse_latest_release_version(mtx::xml::document_cptr const &doc);

segment_info_data_t get_default_segment_info_data(std::string const &application = "MKVToolNix");
