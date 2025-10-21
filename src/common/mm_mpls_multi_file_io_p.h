/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_file_io_p.h"

class mm_mpls_multi_file_io_c;

class mm_mpls_multi_file_io_private_c : public mm_file_io_private_c {
public:
  std::vector<boost::filesystem::path> files;
  std::string display_file_name;
  mtx::bluray::mpls::parser_cptr mpls_parser;
  uint64_t total_size{};

  explicit mm_mpls_multi_file_io_private_c(std::vector<boost::filesystem::path> const &p_file_names,
                                           std::string const &p_display_file_name,
                                           mtx::bluray::mpls::parser_cptr const &p_mpls_parser)
    : mm_file_io_private_c{p_file_names[0].string(), libebml::MODE_READ}
    , files{p_file_names}
    , display_file_name{p_display_file_name}
    , mpls_parser{p_mpls_parser}
    , total_size{std::accumulate(p_file_names.begin(), p_file_names.end(), 0ull, [](uint64_t accu, boost::filesystem::path const &file) { return accu + boost::filesystem::file_size(file); })}
  {
  }
};
