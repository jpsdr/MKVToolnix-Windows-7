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

#include "common/mm_io_p.h"

class mm_multi_file_io_c;

class mm_multi_file_io_private_c : public mm_io_private_c {
public:
  struct file_t {
    boost::filesystem::path file_name;
    uint64_t size{}, global_start{};
    mm_file_io_cptr file;

    file_t(boost::filesystem::path const &p_file_name, uint64_t p_global_start, mm_file_io_cptr const &p_file)
      : file_name{p_file_name}
      , size{static_cast<uint64_t>(p_file->get_size())}
      , global_start{p_global_start}
      , file{p_file}
    {
    }
  };

  std::string display_file_name;
  uint64_t total_size{}, current_pos{}, current_local_pos{};
  unsigned int current_file{};
  std::vector<file_t> files;

  explicit mm_multi_file_io_private_c(std::vector<boost::filesystem::path> const &p_file_names,
                                      std::string const &p_display_file_name)
    : display_file_name{p_display_file_name}
  {
    for (auto &file_name : p_file_names) {
      auto file = std::static_pointer_cast<mm_file_io_c>(mm_file_io_c::open(file_name.string()));
      files.emplace_back(file_name, total_size, file);

      total_size += file->get_size();
    }
  }
};
