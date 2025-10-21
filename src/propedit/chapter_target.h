/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "propedit/target.h"

class chapter_target_c: public target_c {
protected:
  mtx::chapters::kax_cptr m_new_chapters;
  std::string m_charset;

public:
  chapter_target_c();
  virtual ~chapter_target_c() override;

  virtual void validate() override;

  virtual bool operator ==(target_c const &cmp) const override;

  virtual void parse_chapter_spec(std::string const &spec, std::string const &charset);
  virtual void dump_info() const override;

  virtual bool has_changes() const override;
  virtual bool write_elements_set_to_default_value() const override;
  virtual bool add_mandatory_elements_if_missing() const override;

  virtual void execute() override;
};
