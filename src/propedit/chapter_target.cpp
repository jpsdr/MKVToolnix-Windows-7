/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/kax_analyzer.h"
#include "propedit/chapter_target.h"

chapter_target_c::chapter_target_c()
  : target_c()
{
}

chapter_target_c::~chapter_target_c() {
}

bool
chapter_target_c::operator ==(target_c const &cmp)
  const {
  return dynamic_cast<chapter_target_c const *>(&cmp);
}

void
chapter_target_c::validate() {
  if (!m_file_name.empty() && !m_new_chapters)
    m_new_chapters = mtx::chapters::parse(m_file_name, 0, -1, 0, {}, m_charset);
}

void
chapter_target_c::dump_info()
  const {
  mxinfo(fmt::format("  chapter target:\n"
                     "    file_name: {0}\n",
                     m_file_name));
}

bool
chapter_target_c::has_changes()
  const {
  return true;
}

void
chapter_target_c::parse_chapter_spec(std::string const &spec,
                                     std::string const &charset) {
  m_file_name = spec;
  m_charset   = charset;
}

void
chapter_target_c::execute() {
  add_or_replace_all_master_elements(m_new_chapters.get());

  if (!m_level1_element->ListSize())
    return;

  fix_mandatory_elements(m_level1_element);
  if (!m_level1_element->CheckMandatory())
    mxerror(fmt::format(FY("Error parsing the chapters in '{0}': some mandatory elements are missing.\n"), m_file_name));

  if (m_analyzer->is_webm())
    mtx::chapters::remove_elements_unsupported_by_webm(*m_level1_element);
}

bool
chapter_target_c::write_elements_set_to_default_value()
  const {
  return !m_analyzer->is_webm();
}

bool
chapter_target_c::add_mandatory_elements_if_missing()
  const {
  return false;
}
