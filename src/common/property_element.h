/**
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlElement.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackVideo.h>

#include "common/ebml.h"
#include "common/translation.h"

#define NO_CONTAINER EbmlId(static_cast<uint32_t>(0), 0)

class property_element_c {
public:
  enum ebml_type_e { EBMLT_SKIP, EBMLT_BOOL, EBMLT_BINARY, EBMLT_FLOAT, EBMLT_INT, EBMLT_UINT, EBMLT_STRING, EBMLT_USTRING, EBMLT_DATE };

  std::string m_name;
  translatable_string_c m_title, m_description;

  EbmlCallbacks const *m_callbacks, *m_sub_master_callbacks, *m_sub_sub_master_callbacks, *m_sub_sub_sub_master_callbacks;

  unsigned int m_bit_length;
  ebml_type_e m_type;

  property_element_c();
  property_element_c(std::string name, EbmlCallbacks const &callbacks, translatable_string_c title, translatable_string_c description,
                     EbmlCallbacks const &sub_master_callbacks, EbmlCallbacks const *sub_sub_master_callbacks = nullptr, EbmlCallbacks const *sub_sub_sub_master_callbacks = nullptr);

  bool is_valid() const;

private:
  void derive_type();

private:                        // static
  static std::map<uint32_t, std::vector<property_element_c> > s_properties;
  static std::map<uint32_t, std::vector<property_element_c> > s_composed_properties;

public:                         // static
  static void init_tables();
  static std::vector<property_element_c> &get_table_for(const EbmlCallbacks &master_callbacks, const EbmlCallbacks *sub_master_callbacks = nullptr, bool full_table = false);
};
using property_element_cptr = std::shared_ptr<property_element_c>;
