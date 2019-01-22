/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  definitions and helper functions for Blu-ray index file

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace bluray { namespace index {

struct first_playback_t {
  unsigned int object_type{};
  unsigned int hdmv_title_playback_type{}, mobj_id_ref{};
  unsigned int bd_j_title_playback_type{};
  std::string bdjo_file_name;

  void dump() const;
};

struct top_menu_t {
  unsigned int object_type{};
  unsigned int mobj_id_ref{};
  std::string bdjo_file_name;

  void dump() const;
};

struct title_t {
  unsigned int object_type{}, access_type{};
  unsigned int hdmv_title_playback_type{}, mobj_id_ref{};
  unsigned int bd_j_title_playback_type{};
  std::string bdjo_file_name;

  void dump(unsigned int idx) const;
};

struct index_t {
  first_playback_t first_playback;
  top_menu_t top_menu;
  std::vector<title_t> titles;

  void dump() const;
};

class parser_private_c;
class parser_c {
protected:
  MTX_DECLARE_PRIVATE(parser_private_c);

  std::unique_ptr<parser_private_c> const p_ptr;

  explicit parser_c(parser_private_c &p);

public:
  parser_c(std::string file_name);
  virtual ~parser_c();

  virtual bool parse();
  virtual bool is_ok() const;

  virtual index_t const &get_index() const;

  virtual void dump() const;

protected:
  virtual void parse_header();
  virtual void parse_index();
  virtual void parse_first_playback();
  virtual void parse_top_menu();
  virtual void parse_title();
};

}}}                             // namespace mtx::bluray::clpi
