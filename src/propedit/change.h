/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bitvalue.h"
#include "common/property_element.h"

class change_c;
using change_cptr = std::shared_ptr<change_c>;

class change_c {
public:
  enum change_type_e {
    ct_add,
    ct_set,
    ct_delete,
  };

  change_type_e m_type;
  std::string m_name, m_value;

  property_element_c m_property;

  std::string m_s_value;
  uint64_t m_ui_value;
  int64_t m_si_value;
  bool m_b_value;
  mtx::bits::value_c m_x_value;
  double m_fp_value;

  libebml::EbmlMaster *m_master, *m_sub_sub_master, *m_sub_sub_sub_master;

public:
  change_c(change_type_e type, const std::string &name, const std::string &value);

  void validate();
  void dump_info() const;

  bool look_up_property(std::vector<property_element_c> &table);

  std::string get_spec();

  void execute(libebml::EbmlMaster *master, libebml::EbmlMaster *sub_master);

public:
  static std::vector<change_cptr> parse_spec(change_type_e type, std::string const &spec);
  static std::vector<change_cptr> make_change_for_language(change_type_e type, std::string const &name, std::string const &value);

protected:
  void parse_ascii_string();
  void parse_binary();
  void parse_boolean();
  void parse_date_time();
  void parse_floating_point_number();
  void parse_signed_integer();
  void parse_unicode_string();
  void parse_unsigned_integer();
  void parse_value();

  void execute_add_or_set();
  void execute_delete();
  void do_add_element();
  void set_element_at(int idx);

  void validate_deletion_of_mandatory();

  const libebml::EbmlSemantic *get_semantic();

  void record_track_uid_changes(std::size_t idx);
};
