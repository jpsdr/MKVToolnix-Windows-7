/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/cli_parser.h"
#include "propedit/attachment_target.h"
#include "propedit/options.h"

class propedit_cli_parser_c: public mtx::cli::parser_c {
protected:
  options_cptr m_options;
  target_cptr m_target;
  attachment_target_c::options_t m_attachment;

public:
  propedit_cli_parser_c(const std::vector<std::string> &args);

  options_cptr run();

protected:
  void init_parser();
  void validate();

  void add_target();
  void add_change();
  void add_tags();
  void add_chapters();
  void set_chapter_charset();
  void set_parse_mode();
  void set_file_name();
  void disable_language_ietf();
  void enable_legacy_font_mime_types();
  void set_language_ietf_normalization_mode();

  void set_attachment_name();
  void set_attachment_description();
  void set_attachment_mime_type();
  void set_attachment_uid();
  void add_attachment();
  void delete_attachment();
  void replace_attachment();

  void list_property_names();
  void list_property_names_for_table(const std::vector<property_element_c> &table, const std::string &title, const std::string &edit_spec);

  void handle_track_statistics_tags();

  std::map<property_element_c::ebml_type_e, const char *> &get_ebml_type_abbrev_map();
};
