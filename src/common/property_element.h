/**
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlElement.h>
#include <matroska/KaxTracks.h>

#include "common/ebml.h"
#include "common/translation.h"

class property_element_c {
public:
  enum ebml_type_e { EBMLT_SKIP, EBMLT_BOOL, EBMLT_BINARY, EBMLT_FLOAT, EBMLT_INT, EBMLT_UINT, EBMLT_STRING, EBMLT_USTRING, EBMLT_DATE };

  std::string m_name;
  translatable_string_c m_title, m_description;

  libebml::EbmlCallbacks const *m_callbacks, *m_sub_master_callbacks, *m_sub_sub_master_callbacks, *m_sub_sub_sub_master_callbacks;

  unsigned int m_bit_length;
  ebml_type_e m_type;

  property_element_c();
  property_element_c(std::string name, libebml::EbmlCallbacks const &callbacks, translatable_string_c title, translatable_string_c description,
                     libebml::EbmlCallbacks const &sub_master_callbacks, libebml::EbmlCallbacks const *sub_sub_master_callbacks = nullptr, libebml::EbmlCallbacks const *sub_sub_sub_master_callbacks = nullptr);

  bool is_valid() const;

private:
  void derive_type();

private:                        // static
  static std::map<uint32_t, std::vector<property_element_c> > s_properties;
  static std::map<uint32_t, std::vector<property_element_c> > s_composed_properties;
  static std::unordered_map<std::string, std::string> s_aliases;

public:                         // static
  static void init_tables();
  static std::vector<property_element_c> &get_table_for(const libebml::EbmlCallbacks &master_callbacks, const libebml::EbmlCallbacks *sub_master_callbacks = nullptr, bool full_table = false);
  static std::string get_actual_name(std::string const &name);
};
using property_element_cptr = std::shared_ptr<property_element_c>;
