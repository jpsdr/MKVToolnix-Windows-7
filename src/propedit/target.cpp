/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/kax_analyzer.h"
#include "common/output.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "propedit/target.h"

target_c::target_c()
  : m_level1_element{}
  , m_master{}
  , m_sub_master{}
  , m_track_uid{}
  , m_track_type{static_cast<track_type>(0)}
  , m_analyzer{}
{
}

target_c::~target_c() {
}

bool
target_c::operator !=(target_c const &cmp)
  const {
  return !(*this == cmp);
}

void
target_c::add_change(change_c::change_type_e,
                     std::string const &) {
}

void
target_c::set_level1_element(ebml_element_cptr level1_element_cp,
                             ebml_element_cptr track_headers_cp) {
  m_level1_element_cp = level1_element_cp;
  m_level1_element    = static_cast<libebml::EbmlMaster *>(m_level1_element_cp.get());

  m_track_headers_cp  = track_headers_cp;
  m_master            = m_level1_element;
}

std::tuple<libebml::EbmlMaster *, libebml::EbmlMaster *>
target_c::get_masters()
  const {
  return std::make_tuple(m_master, m_sub_master);
}

void
target_c::add_or_replace_all_master_elements(libebml::EbmlMaster *source) {
  size_t idx;
  for (idx = 0; m_level1_element->ListSize() > idx; ++idx)
    delete (*m_level1_element)[idx];
  m_level1_element->RemoveAll();

  if (source) {
    for (idx = 0; source->ListSize() > idx; ++idx)
      m_level1_element->PushElement(*(*source)[idx]);
    source->RemoveAll();
  }
}

std::string const &
target_c::get_spec()
  const {
  return m_spec;
}

uint64_t
target_c::get_track_uid()
  const {
  return m_track_uid;
}

libebml::EbmlMaster *
target_c::get_level1_element()
  const {
  return m_level1_element;
}

bool
target_c::has_content_been_modified()
  const {
  return true;
}

void
target_c::execute_change(kax_analyzer_c &analyzer) {
  m_analyzer = &analyzer;
  execute();
}

bool
target_c::write_elements_set_to_default_value()
  const {
  return true;
}

bool
target_c::add_mandatory_elements_if_missing()
  const {
  return true;
}
