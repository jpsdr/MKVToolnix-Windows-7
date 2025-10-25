/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/output.h"
#include "propedit/segment_info_target.h"

segment_info_target_c::segment_info_target_c()
  : target_c()
{
}

segment_info_target_c::~segment_info_target_c() {
}

bool
segment_info_target_c::operator ==(target_c const &cmp)
  const {
  return dynamic_cast<segment_info_target_c const *>(&cmp);
}

void
segment_info_target_c::validate() {
  look_up_property_elements();

  for (auto &change : m_changes)
    change->validate();
}

void
segment_info_target_c::look_up_property_elements() {
  auto &property_table = property_element_c::get_table_for(EBML_INFO(libmatroska::KaxInfo), nullptr, false);

  for (auto &change : m_changes)
    change->look_up_property(property_table);
}

void
segment_info_target_c::add_change(change_c::change_type_e type,
                                  const std::string &spec) {
  for (auto const &change : change_c::parse_spec(type, spec))
    m_changes.push_back(change);
}

void
segment_info_target_c::dump_info()
  const {
  mxinfo(fmt::format("  segment_info_target:\n"));

  for (auto &change : m_changes)
    change->dump_info();
}

bool
segment_info_target_c::has_changes()
  const {
  return !m_changes.empty();
}

void
segment_info_target_c::execute() {
  for (auto &change : m_changes)
    change->execute(m_master, m_sub_master);
}
