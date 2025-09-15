/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

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

#include <QDateTime>
#include <QRegularExpression>
#include <QTimeZone>

#include "common/bcp47.h"
#include "common/date_time.h"
#include "common/ebml.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/output.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "propedit/change.h"
#include "propedit/globals.h"

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
    mxerror(fmt::format(FY("The name '{0}' is not a valid property name for the current edit specification in '{1}'.\n"), m_name, get_spec()));

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
  auto actual_name = property_element_c::get_actual_name(m_name);

  for (auto &property : table)
    if (property.m_name == actual_name) {
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
    if (127 < static_cast<uint8_t>(m_value[i]))
      mxerror(fmt::format(FY("The property value contains non-ASCII characters, but the property is not a Unicode string in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));

  m_s_value = m_value;
}

void
change_c::parse_unicode_string() {
  m_s_value = m_value;
}

void
change_c::parse_unsigned_integer() {
  if (!mtx::string::parse_number(m_value, m_ui_value))
    mxerror(fmt::format(FY("The property value is not a valid unsigned integer in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
}

void
change_c::parse_signed_integer() {
  if (!mtx::string::parse_number(m_value, m_si_value))
    mxerror(fmt::format(FY("The property value is not a valid signed integer in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
}

void
change_c::parse_boolean() {
  try {
    m_b_value = mtx::string::parse_bool(m_value);
  } catch (...) {
    mxerror(fmt::format(FY("The property value is not a valid boolean in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
  }
}

void
change_c::parse_floating_point_number() {
  if (!mtx::string::parse_number(m_value, m_fp_value))
    mxerror(fmt::format(FY("The property value is not a valid floating point number in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
}

void
change_c::parse_binary() {
  try {
    m_x_value = mtx::bits::value_c(m_value, m_property.m_bit_length);
  } catch (...) {
    if (m_property.m_bit_length)
      mxerror(fmt::format(FY("The property value is not a valid binary spec or it is not exactly {2} bits long in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified."), m_property.m_bit_length));
    mxerror(fmt::format(FY("The property value is not a valid binary spec in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
  }
}

void
change_c::parse_date_time() {
  //                      1        2        3                4        5        6           7  8     9        10
  QRegularExpression re{"^(\\d{4})-(\\d{2})-(\\d{2})(?:T|\\s)(\\d{2}):(\\d{2}):(\\d{2})\\s*(Z|([+-])(\\d{2}):(\\d{2}))$", QRegularExpression::CaseInsensitiveOption};
  int64_t year{}, month{}, day{}, hours{}, minutes{}, seconds{};
  int64_t offset_hours{}, offset_minutes{}, offset_mult{1};

  auto matches = re.match(Q(m_value));
  auto valid   = matches.hasMatch();

  if (valid)
    valid = mtx::string::parse_number(to_utf8(matches.captured(1)), year)
         && mtx::string::parse_number(to_utf8(matches.captured(2)), month)
         && mtx::string::parse_number(to_utf8(matches.captured(3)), day)
         && mtx::string::parse_number(to_utf8(matches.captured(4)), hours)
         && mtx::string::parse_number(to_utf8(matches.captured(5)), minutes)
         && mtx::string::parse_number(to_utf8(matches.captured(6)), seconds);

  if (valid && (to_utf8(matches.captured(7)) != "Z")) {
    valid = mtx::string::parse_number(to_utf8(matches.captured(9)),  offset_hours)
         && mtx::string::parse_number(to_utf8(matches.captured(10)), offset_minutes);

    if (to_utf8(matches.captured(8)) == "-")
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
                        fmt::format(FY("The property value is not a valid date & time string in '{0}'."), get_spec()),
                        Y("The recognized format is 'YYYY-mm-ddTHH:MM:SS+zz:zz': the year, month, day, letter 'T', hours, minutes, seconds and the time zone's offset from UTC; example: 2017-03-28T17:28:00-02:00."),
                        Y("The letter 'Z' can be used instead of the time zone's offset from UTC to indicate UTC aka Zulu time."),
                        Y("The file has not been modified.")));

  QDate date(year, month, day);
  QTime time(hours, minutes, seconds);

  m_ui_value              = QDateTime{date, time, QTimeZone::utc()}.toSecsSinceEpoch();

  auto tz_offset_minutes  = (offset_hours * 60 + offset_minutes) * offset_mult;
  m_ui_value             -= tz_offset_minutes * 60;
}

void
change_c::execute(libebml::EbmlMaster *master,
                  libebml::EbmlMaster *sub_master) {
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
    if (m_property.m_callbacks->ClassId() == get_ebml_id(*(*m_master)[idx])) {
      delete (*m_master)[idx];
      m_master->Remove(idx);
      ++num_deleted;
    } else
      ++idx;
  }

  if (1 < verbose)
    mxinfo(fmt::format(FY("Change for '{0}' executed. Number of entries deleted: {1}\n"), get_spec(), num_deleted));
}

void
change_c::record_track_uid_changes(std::size_t idx) {
  if (m_property.m_callbacks->ClassId() != EBML_ID(libmatroska::KaxTrackUID))
    return;

  auto current_uid = static_cast<libebml::EbmlUInteger *>((*m_master)[idx])->GetValue();
  if (current_uid != m_ui_value)
    g_track_uid_changes[current_uid] = m_ui_value;
}

void
change_c::execute_add_or_set() {
  size_t idx;
  unsigned int num_found = 0;
  for (idx = 0; m_master->ListSize() > idx; ++idx) {
    if (m_property.m_callbacks->ClassId() != get_ebml_id(*(*m_master)[idx]))
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
      mxinfo(fmt::format(FY("Change for '{0}' executed. No property of this type found. One entry added.\n"), get_spec()));
    return;
  }

  if (change_c::ct_set == m_type) {
    if (1 < verbose)
      mxinfo(fmt::format(FY("Change for '{0}' executed. Number of entries set: {1}.\n"), get_spec(), num_found));
    return;
  }

  const libebml::EbmlSemantic *semantic = get_semantic();
  if (semantic && semantic->IsUnique())
    mxerror(fmt::format(FY("This property is unique. More instances cannot be added in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));

  do_add_element();

 if (1 < verbose)
    mxinfo(fmt::format(FY("Change for '{0}' executed. One entry added.\n"), get_spec()));
}

void
change_c::do_add_element() {
  m_master->PushElement(m_property.m_callbacks->NewElement());
  set_element_at(m_master->ListSize() - 1);
}

void
change_c::set_element_at(int idx) {
  libebml::EbmlElement *e = (*m_master)[idx];

  switch (m_property.m_type) {
    case property_element_c::EBMLT_STRING:  static_cast<libebml::EbmlString        *>(e)->SetValue(m_s_value);                                 break;
    case property_element_c::EBMLT_USTRING: static_cast<libebml::EbmlUnicodeString *>(e)->SetValueUTF8(m_s_value);                             break;
    case property_element_c::EBMLT_UINT:    static_cast<libebml::EbmlUInteger      *>(e)->SetValue(m_ui_value);                                break;
    case property_element_c::EBMLT_INT:     static_cast<libebml::EbmlSInteger      *>(e)->SetValue(m_si_value);                                break;
    case property_element_c::EBMLT_BOOL:    static_cast<libebml::EbmlUInteger      *>(e)->SetValue(m_b_value ? 1 : 0);                         break;
    case property_element_c::EBMLT_FLOAT:   static_cast<libebml::EbmlFloat         *>(e)->SetValue(m_fp_value);                                break;
    case property_element_c::EBMLT_BINARY:  static_cast<libebml::EbmlBinary        *>(e)->CopyBuffer(m_x_value.data(), m_x_value.byte_size()); break;
    case property_element_c::EBMLT_DATE:    static_cast<libebml::EbmlDate          *>(e)->SetEpochDate(m_ui_value);                            break;
    default:                                assert(false);
  }
}

void
change_c::validate_deletion_of_mandatory() {
  if (m_sub_sub_master)
    return;

  const libebml::EbmlSemantic *semantic = get_semantic();

  if (!semantic || !semantic->IsMandatory())
    return;

  std::unique_ptr<libebml::EbmlElement> elt(&create_ebml_element(*semantic));

  if (!has_default_value(*elt))
    mxerror(fmt::format(FY("This property is mandatory and cannot be deleted in '{0}'. {1}\n"), get_spec(), Y("The file has not been modified.")));
}

const libebml::EbmlSemantic *
change_c::get_semantic() {
  return find_ebml_semantic(EBML_INFO(libmatroska::KaxSegment), m_property.m_callbacks->ClassId());
}

std::vector<change_cptr>
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

  if (mtx::included_in(name, "language", "language-ietf"))
    return make_change_for_language(type, name, value);

  return { std::make_shared<change_c>(type, name, value) };
}

std::vector<change_cptr>
change_c::make_change_for_language(change_c::change_type_e type,
                                   std::string const &name,
                                   std::string const &value) {
  std::vector<change_cptr> changes;

  if (type == ct_delete) {
    if (name == "language")
      changes.emplace_back(std::make_shared<change_c>(ct_delete, "language", value));

    if ((name == "language-ietf") || !mtx::bcp47::language_c::is_disabled())
      changes.emplace_back(std::make_shared<change_c>(ct_delete, "language-ietf", value));

    return changes;
  }

  auto language = mtx::bcp47::language_c::parse(value);
  if (!language.is_valid())
    throw std::runtime_error{fmt::format(FY("invalid language tag '{0}': {1}"), value, language.get_error())};

  if (name == "language")
    changes.push_back(std::make_shared<change_c>(type, "language", language.get_closest_iso639_2_alpha_3_code()));

  if ((name == "language-ietf") || !mtx::bcp47::language_c::is_disabled())
    changes.push_back(std::make_shared<change_c>(type, "language-ietf", language.format()));

  return changes;
}
