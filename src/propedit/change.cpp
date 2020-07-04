/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlBinary.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <matroska/KaxSegment.h>

#include "common/common_pch.h"
#include "common/date_time.h"
#include "common/ebml.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/output.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "propedit/change.h"
#include "propedit/propedit.h"

change_c::change_c(change_c::change_type_e type,
                   const std::string &name,
                   const std::string &value)
  : m_type(type)
  , m_name(name)
  , m_value(value)
  , m_ui_value(0)
  , m_si_value(0)
  , m_b_value(false)
  , m_x_value(128)
  , m_fp_value(0)
  , m_master(nullptr)
  , m_sub_sub_master{}
  , m_sub_sub_sub_master{}
{
}

void
change_c::validate() {
  if (m_property.m_name.empty())
    mxerror(fmt::format(Y("The name '{0}' is not a valid property name for the current edit specification in '{1}'.\n"), m_name, get_spec()));

  if (change_c::ct_delete == m_type)
    validate_deletion_of_mandatory();
  else
    parse_value();
}

std::string
change_c::get_spec() {
  return change_c::ct_delete == m_type ? fmt::format("--delete {0}",                                                 m_name          )
       :                                 fmt::format("--{0} {1}={2}", change_c::ct_add == m_type ? "add" : "set", m_name, m_value);
}

void
change_c::dump_info()
  const
{
  mxinfo(fmt::format("    change:\n"
                     "      type:  {0}\n"
                     "      name:  {1}\n"
                     "      value: {2}\n",
                     static_cast<int>(m_type),
                     m_name,
                     m_value));
}

bool
change_c::look_up_property(std::vector<property_element_c> &table) {
  for (auto &property : table)
    if (property.m_name == m_name) {
      m_property = property;
      return true;
    }

  return false;
}

void
change_c::parse_value() {
  switch (m_property.m_type) {
    case property_element_c::EBMLT_STRING:  parse_ascii_string();          break;
    case property_element_c::EBMLT_USTRING: parse_unicode_string();        break;
    case property_element_c::EBMLT_UINT:    parse_unsigned_integer();      break;
    case property_element_c::EBMLT_INT:     parse_signed_integer();        break;
    case property_element_c::EBMLT_BOOL:    parse_boolean();               break;
    case property_element_c::EBMLT_BINARY:  parse_binary();                break;
    case property_element_c::EBMLT_FLOAT:   parse_floating_point_number(); break;
    case property_element_c::EBMLT_DATE:    parse_date_time();             break;
    default:                                assert(false);
  }
}

void
change_c::parse_ascii_string() {
  size_t i;
  for (i = 0; m_value.length() > i; ++i)
    if (127 < static_cast<unsigned char>(m_value[i]))
      mxerror(fmt::format(Y("The property value contains non-ASCII characters, but the property is not a Unicode string in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));

  m_s_value = m_value;
}

void
change_c::parse_unicode_string() {
  m_s_value = m_value;
}

void
change_c::parse_unsigned_integer() {
  if (!mtx::string::parse_number(m_value, m_ui_value))
    mxerror(fmt::format(Y("The property value is not a valid unsigned integer in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
}

void
change_c::parse_signed_integer() {
  if (!mtx::string::parse_number(m_value, m_si_value))
    mxerror(fmt::format(Y("The property value is not a valid signed integer in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
}

void
change_c::parse_boolean() {
  try {
    m_b_value = mtx::string::parse_bool(m_value);
  } catch (...) {
    mxerror(fmt::format(Y("The property value is not a valid boolean in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
  }
}

void
change_c::parse_floating_point_number() {
  if (!mtx::string::parse_number(m_value, m_fp_value))
    mxerror(fmt::format(Y("The property value is not a valid floating point number in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
}

void
change_c::parse_binary() {
  try {
    m_x_value = mtx::bits::value_c(m_value, m_property.m_bit_length);
  } catch (...) {
    if (m_property.m_bit_length)
      mxerror(fmt::format(Y("The property value is not a valid binary spec or it is not exactly {2} bits long in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED, m_property.m_bit_length));
    mxerror(fmt::format(Y("The property value is not a valid binary spec in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
  }
}

void
change_c::parse_date_time() {
  //              1        2        3                4        5        6           7  8     9        10
  std::regex re{"^(\\d{4})-(\\d{2})-(\\d{2})(?:T|\\s)(\\d{2}):(\\d{2}):(\\d{2})\\s*(Z|([+-])(\\d{2}):(\\d{2}))$", std::regex_constants::icase};
  std::smatch matches;
  int64_t year{}, month{}, day{}, hours{}, minutes{}, seconds{};
  int64_t offset_hours{}, offset_minutes{}, offset_mult{1};

  auto valid = std::regex_match(m_value, matches, re);

  if (valid)
    valid = mtx::string::parse_number(matches[1].str(), year)
         && mtx::string::parse_number(matches[2].str(), month)
         && mtx::string::parse_number(matches[3].str(), day)
         && mtx::string::parse_number(matches[4].str(), hours)
         && mtx::string::parse_number(matches[5].str(), minutes)
         && mtx::string::parse_number(matches[6].str(), seconds);

  if (valid && (matches[7].str() != "Z")) {
    valid = mtx::string::parse_number(matches[9].str(),  offset_hours)
         && mtx::string::parse_number(matches[10].str(), offset_minutes);

    if (matches[8].str() == "-")
      offset_mult = -1;
  }

  valid = valid
    && (year           >= 1900)
    && (month          >=   1)
    && (month          <=  12)
    && (day            >=   1)
    && (day            <=  31)
    && (hours          >=   0)
    && (hours          <=  23)
    && (minutes        >=   0)
    && (minutes        <=  59)
    && (seconds        >=   0)
    && (seconds        <=  59)
    && (offset_hours   >=   0)
    && (offset_hours   <=  23)
    && (offset_minutes >=   0)
    && (offset_minutes <=  59);

  if (!valid)
    mxerror(fmt::format("{0} {1} {2} {3}\n",
                        fmt::format(Y("The property value is not a valid date & time string in '{0}'."), get_spec()),
                        Y("The recognized format is 'YYYY-mm-ddTHH:MM:SS+zz:zz': the year, month, day, letter 'T', hours, minutes, seconds and the time zone's offset from UTC; example: 2017-03-28T17:28:00-02:00."),
                        Y("The letter 'Z' can be used instead of the time zone's offset from UTC to indicate UTC aka Zulu time."),
                        FILE_NOT_MODIFIED));

  std::tm time_info{};

  time_info.tm_sec        = seconds;
  time_info.tm_min        = minutes;
  time_info.tm_hour       = hours;
  time_info.tm_mday       = day;
  time_info.tm_mon        = month - 1;
  time_info.tm_year       = year - 1900;
  time_info.tm_isdst      = -1;

  m_ui_value              = mtx::date_time::tm_to_time_t(time_info, mtx::date_time::epoch_timezone_e::UTC);

  auto tz_offset_minutes  = (offset_hours * 60 + offset_minutes) * offset_mult;
  m_ui_value             -= tz_offset_minutes * 60;
}

void
change_c::execute(EbmlMaster *master,
                  EbmlMaster *sub_master) {
  m_master = m_property.m_sub_sub_sub_master_callbacks ? m_sub_sub_sub_master
           : m_property.m_sub_sub_master_callbacks     ? m_sub_sub_master
           : m_property.m_sub_master_callbacks         ? sub_master
           :                                             master;

  if (!m_master)
    return;

  if (change_c::ct_delete == m_type)
    execute_delete();
  else
    execute_add_or_set();
}

void
change_c::execute_delete() {
  size_t idx               = 0;
  unsigned int num_deleted = 0;
  while (m_master->ListSize() > idx) {
    if (m_property.m_callbacks->GlobalId == (*m_master)[idx]->Generic().GlobalId) {
      delete (*m_master)[idx];
      m_master->Remove(idx);
      ++num_deleted;
    } else
      ++idx;
  }

  if (1 < verbose)
    mxinfo(fmt::format(Y("Change for '{0}' executed. Number of entries deleted: {1}\n"), get_spec(), num_deleted));
}

void
change_c::record_track_uid_changes(std::size_t idx) {
  if (m_property.m_callbacks->GlobalId != EBML_ID(libmatroska::KaxTrackUID))
    return;

  auto current_uid = static_cast<EbmlUInteger *>((*m_master)[idx])->GetValue();
  if (current_uid != m_ui_value)
    g_track_uid_changes[current_uid] = m_ui_value;
}

void
change_c::execute_add_or_set() {
  size_t idx;
  unsigned int num_found = 0;
  for (idx = 0; m_master->ListSize() > idx; ++idx) {
    if (m_property.m_callbacks->GlobalId != (*m_master)[idx]->Generic().GlobalId)
      continue;

    if (change_c::ct_set == m_type) {
      record_track_uid_changes(idx);
      set_element_at(idx);
    }

    ++num_found;
  }

  if (0 == num_found) {
    do_add_element();
    if (1 < verbose)
      mxinfo(fmt::format(Y("Change for '{0}' executed. No property of this type found. One entry added.\n"), get_spec()));
    return;
  }

  if (change_c::ct_set == m_type) {
    if (1 < verbose)
      mxinfo(fmt::format(Y("Change for '{0}' executed. Number of entries set: {1}.\n"), get_spec(), num_found));
    return;
  }

  const EbmlSemantic *semantic = get_semantic();
  if (semantic && semantic->Unique)
    mxerror(fmt::format(Y("This property is unique. More instances cannot be added in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));

  do_add_element();

 if (1 < verbose)
    mxinfo(fmt::format(Y("Change for '{0}' executed. One entry added.\n"), get_spec()));
}

void
change_c::do_add_element() {
  m_master->PushElement(m_property.m_callbacks->Create());
  set_element_at(m_master->ListSize() - 1);
}

void
change_c::set_element_at(int idx) {
  EbmlElement *e = (*m_master)[idx];

  switch (m_property.m_type) {
    case property_element_c::EBMLT_STRING:  static_cast<EbmlString        *>(e)->SetValue(m_s_value);                                 break;
    case property_element_c::EBMLT_USTRING: static_cast<EbmlUnicodeString *>(e)->SetValueUTF8(m_s_value);                             break;
    case property_element_c::EBMLT_UINT:    static_cast<EbmlUInteger      *>(e)->SetValue(m_ui_value);                                break;
    case property_element_c::EBMLT_INT:     static_cast<EbmlSInteger      *>(e)->SetValue(m_si_value);                                break;
    case property_element_c::EBMLT_BOOL:    static_cast<EbmlUInteger      *>(e)->SetValue(m_b_value ? 1 : 0);                         break;
    case property_element_c::EBMLT_FLOAT:   static_cast<EbmlFloat         *>(e)->SetValue(m_fp_value);                                break;
    case property_element_c::EBMLT_BINARY:  static_cast<EbmlBinary        *>(e)->CopyBuffer(m_x_value.data(), m_x_value.byte_size()); break;
    case property_element_c::EBMLT_DATE:    static_cast<EbmlDate          *>(e)->SetEpochDate(m_ui_value);                            break;
    default:                                assert(false);
  }
}

void
change_c::validate_deletion_of_mandatory() {
  if (m_sub_sub_master)
    return;

  const EbmlSemantic *semantic = get_semantic();

  if (!semantic || !semantic->Mandatory)
    return;

  std::unique_ptr<EbmlElement> elt(&semantic->Create());

  if (!elt->DefaultISset())
    mxerror(fmt::format(Y("This property is mandatory and cannot be deleted in '{0}'. {1}\n"), get_spec(), FILE_NOT_MODIFIED));
}

const EbmlSemantic *
change_c::get_semantic() {
  return find_ebml_semantic(libmatroska::KaxSegment::ClassInfos, m_property.m_callbacks->GlobalId);
}

change_cptr
change_c::parse_spec(change_c::change_type_e type,
                     const std::string &spec) {
  std::string name, value;
  if (ct_delete == type)
    name = spec;

  else {
    auto parts = mtx::string::split(spec, "=", 2);
    if (2 != parts.size())
      throw std::runtime_error(Y("missing value"));

    name  = parts[0];
    value = parts[1];
  }

  if (name.empty())
    throw std::runtime_error(Y("missing property name"));

  if (   mtx::included_in(type, ct_add, ct_set)
      && (name == "language")) {
    auto idx = mtx::iso639::look_up(value);
    if (!idx)
      throw std::runtime_error{fmt::format(("invalid ISO 639-2 language code '{0}'"), value)};

    value = mtx::iso639::g_languages[*idx].iso639_2_code;
  }

  return std::make_shared<change_c>(type, name, value);
}
