/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bluray/mpls.h"
#include "common/mm_multi_file_io.h"

class mm_mpls_multi_file_io_private_c;
class mm_mpls_multi_file_io_c: public mm_file_io_c {
protected:
  MTX_DECLARE_PRIVATE(mm_mpls_multi_file_io_private_c)

  explicit mm_mpls_multi_file_io_c(mm_mpls_multi_file_io_private_c &p);

public:
  mm_mpls_multi_file_io_c(std::vector<boost::filesystem::path> const &file_names, std::string const &display_file_name, mtx::bluray::mpls::parser_cptr const &mpls_parser);
  virtual ~mm_mpls_multi_file_io_c();

  virtual std::string get_file_name() const override;
  std::vector<boost::filesystem::path> const &get_file_names() const;
  mtx::bluray::mpls::parser_c const &get_mpls_parser() const;

  std::vector<mtx::bluray::mpls::chapter_t> const &get_chapters() const;
  virtual void create_verbose_identification_info(mtx::id::info_c &info);

  static mm_io_cptr open_multi(std::string const &display_file_name);
  static mm_io_cptr open_multi(mm_io_c &in);
};
