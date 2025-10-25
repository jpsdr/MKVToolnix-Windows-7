/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/qt.h"
#include "common/strings/parsing.h"
#include "propedit/track_target.h"

track_target_c::track_target_c(std::string const &spec)
  : target_c()
  , m_selection_mode{track_target_c::sm_undefined}
  , m_selection_param{}
  , m_selection_track_type{static_cast<track_type>(0)}
{
  m_spec = spec;
}

track_target_c::~track_target_c() {
}

bool
track_target_c::operator ==(target_c const &cmp)
  const {
  auto other_track = dynamic_cast<track_target_c const *>(&cmp);

  return other_track
      && (m_selection_mode       == other_track->m_selection_mode)
      && (m_selection_param      == other_track->m_selection_param)
      && (m_selection_track_type == other_track->m_selection_track_type);
}

void
track_target_c::validate() {
  if (static_cast<track_type>(0) == m_track_type)
    return;

  for (auto &change : m_changes)
    change->validate();
}

void
track_target_c::look_up_property_elements() {
  auto &property_table = property_element_c::get_table_for(EBML_INFO(libmatroska::KaxTracks),
                                                             track_audio == m_track_type ? &EBML_INFO(libmatroska::KaxTrackAudio)
                                                           : track_video == m_track_type ? &EBML_INFO(libmatroska::KaxTrackVideo)
                                                           :                               nullptr,
                                                           false);

  for (auto &change : m_changes)
    change->look_up_property(property_table);
}

void
track_target_c::add_change(change_c::change_type_e type,
                           const std::string &spec) {
  for (auto const &change : change_c::parse_spec(type, spec))
    m_changes.push_back(change);
}

void
track_target_c::dump_info()
  const {
  mxinfo(fmt::format("  track_target:\n"
                     "    selection_mode:       {0}\n"
                     "    selection_param:      {1}\n"
                     "    selection_track_type: {2}\n"
                     "    track_uid:            {3}\n"
                     "    file_name:            {4}\n",
                     static_cast<unsigned int>(m_selection_mode),
                     m_selection_param,
                     static_cast<unsigned int>(m_selection_track_type),
                     m_track_uid,
                     m_file_name));

  for (auto &change : m_changes)
    change->dump_info();
}

bool
track_target_c::has_changes()
  const {
  return !m_changes.empty();
}

bool
track_target_c::has_add_or_set_change()
  const {
  for (auto &change : m_changes)
    if (change_c::ct_delete != change->m_type)
      return true;

  return false;
}

void
track_target_c::execute() {
  for (auto &change : m_changes)
    change->execute(m_master, m_sub_master);

  fix_mandatory_elements(m_master);
}

void
track_target_c::set_level1_element(ebml_element_cptr level1_element_cp,
                                   ebml_element_cptr track_headers_cp) {
  m_level1_element_cp = level1_element_cp;
  m_level1_element    = static_cast<libebml::EbmlMaster *>(m_level1_element_cp.get());

  m_track_headers_cp  = track_headers_cp;

  if (non_track_target()) {
    m_master = m_level1_element;
    return;
  }

  std::map<uint8_t, unsigned int> num_tracks_by_type;
  unsigned int num_tracks_total = 0;

  if (!track_headers_cp)
    track_headers_cp = level1_element_cp;

  libebml::EbmlMaster *track_headers = static_cast<libebml::EbmlMaster *>(track_headers_cp.get());

  size_t i;
  for (i = 0; track_headers->ListSize() > i; ++i) {
    if (!is_type<libmatroska::KaxTrackEntry>((*track_headers)[i]))
      continue;

    libmatroska::KaxTrackEntry *track = dynamic_cast<libmatroska::KaxTrackEntry *>((*track_headers)[i]);
    assert(track);

    libmatroska::KaxTrackType *kax_track_type     = dynamic_cast<libmatroska::KaxTrackType *>(find_child<libmatroska::KaxTrackType>(track));
    track_type this_track_type       = !kax_track_type ? track_video : static_cast<track_type>(uint8_t(*kax_track_type));

    libmatroska::KaxTrackUID *kax_track_uid       = dynamic_cast<libmatroska::KaxTrackUID *>(find_child<libmatroska::KaxTrackUID>(track));
    uint64_t track_uid               = !kax_track_uid ? 0 : uint64_t(*kax_track_uid);

    libmatroska::KaxTrackNumber *kax_track_number = dynamic_cast<libmatroska::KaxTrackNumber *>(find_child<libmatroska::KaxTrackNumber>(track));

    ++num_tracks_total;
    ++num_tracks_by_type[this_track_type];

    bool track_matches = sm_by_uid      == m_selection_mode ? m_selection_param == track_uid
                       : sm_by_position == m_selection_mode ? m_selection_param == num_tracks_total
                       : sm_by_number   == m_selection_mode ? kax_track_number  && (m_selection_param == uint64_t(*kax_track_number))
                       :                                      (this_track_type == m_selection_track_type) && (m_selection_param == num_tracks_by_type[this_track_type]);

    if (!track_matches)
      continue;

    m_track_uid  = track_uid;
    m_track_type = this_track_type;
    m_master     = track;
    m_sub_master = track_video == m_track_type ? static_cast<libebml::EbmlMaster *>(&get_child<libmatroska::KaxTrackVideo>(track))
                 : track_audio == m_track_type ? static_cast<libebml::EbmlMaster *>(&get_child<libmatroska::KaxTrackAudio>(track))
                 :                               nullptr;

    look_up_property_elements();

    if (track_video == m_track_type)
      for (auto const &change_ptr : m_changes) {
        auto &change = *change_ptr;
        auto &prop   = change.m_property;

        if (!prop.m_sub_sub_master_callbacks)
          continue;

        change.m_sub_sub_master = prop.m_sub_sub_master_callbacks == &EBML_INFO(libmatroska::KaxVideoColour)     ? &get_child_empty_if_new<libmatroska::KaxVideoColour>(m_sub_master)
                                : prop.m_sub_sub_master_callbacks == &EBML_INFO(libmatroska::KaxVideoProjection) ? &get_child_empty_if_new<libmatroska::KaxVideoProjection>(m_sub_master)
                                :                                                                                  static_cast<libebml::EbmlMaster*>(nullptr);

        if (!change.m_sub_sub_master)
          continue;

        if (!prop.m_sub_sub_sub_master_callbacks)
          continue;

        change.m_sub_sub_sub_master = prop.m_sub_sub_sub_master_callbacks == &EBML_INFO(libmatroska::KaxVideoColourMasterMeta) ? &get_child_empty_if_new<libmatroska::KaxVideoColourMasterMeta>(change.m_sub_sub_master)
                                    :                                                                                            nullptr;
      }

    if (sub_master_is_track()) {
      m_master     = m_level1_element;
      m_sub_master = track;
    }

    return;
  }

  mxerror(fmt::format(FY("No track corresponding to the edit specification '{0}' was found. {1}\n"), m_spec, Y("The file has not been modified.")));
}

void
track_target_c::parse_spec(std::string const &spec) {
  static QRegularExpression track_re{"^([absv=@]?)(\\d+)"};

  auto matches = track_re.match(Q(spec));
  if (!matches.hasMatch())
    throw false;

  std::string prefix = to_utf8(matches.captured(1));
  mtx::string::parse_number(to_utf8(matches.captured(2)), m_selection_param);

  m_selection_mode = prefix.empty() ? sm_by_position
                   : prefix == "="  ? sm_by_uid
                   : prefix == "@"  ? sm_by_number
                   :                  sm_by_type_and_position;

  if (sm_by_type_and_position == m_selection_mode) {
    m_selection_track_type = 'a' == prefix[0] ? track_audio
                           : 'v' == prefix[0] ? track_video
                           : 's' == prefix[0] ? track_subtitle
                           : 'b' == prefix[0] ? track_buttons
                           :                    static_cast<track_type>(0);

    if (static_cast<track_type>(0) == m_selection_track_type)
      throw false;
  }
}

bool
track_target_c::non_track_target()
  const {
  return false;
}

bool
track_target_c::sub_master_is_track()
  const {
  return false;
}

bool
track_target_c::requires_sub_master()
  const {
  return (   (track_video == m_track_type)
          || (track_audio == m_track_type))
      && has_add_or_set_change();
}

void
track_target_c::merge_changes(track_target_c &other) {
  m_changes.insert(m_changes.begin(), other.m_changes.begin(), other.m_changes.end());
}
