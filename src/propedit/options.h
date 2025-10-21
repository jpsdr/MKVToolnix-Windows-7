/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlMaster.h>

#include "common/kax_analyzer.h"
#include "propedit/attachment_target.h"
#include "propedit/tag_target.h"

class options_c {
public:
  std::string m_file_name, m_chapter_charset;
  std::vector<target_cptr> m_targets;
  bool m_show_progress;
  kax_analyzer_c::parse_mode_e m_parse_mode;

public:
  options_c();

  void validate();
  void options_parsed();

  target_cptr add_track_or_segmentinfo_target(std::string const &spec);
  void add_tags(const std::string &spec);
  void add_chapters(const std::string &spec);
  void add_attachment_command(attachment_target_c::command_e command, std::string const &spec, attachment_target_c::options_t const &options);
  void add_delete_track_statistics_tags(tag_target_c::tag_operation_mode_e operation_mode);
  void set_file_name(const std::string &file_name);
  void set_parse_mode(const std::string &parse_mode);
  void dump_info() const;
  bool has_changes() const;

  void find_elements(kax_analyzer_c *analyzer);

  void execute(kax_analyzer_c &analzyer);

protected:
  void remove_empty_targets();
  void merge_targets();
  void prune_empty_masters();
};
using options_cptr = std::shared_ptr<options_c>;
