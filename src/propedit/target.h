/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlMaster.h>

#include "propedit/change.h"

#define INVALID_TRACK_TYPE static_cast<track_type>(0)

using namespace libebml;

class kax_analyzer_c;

class target_c {
protected:
  std::string m_spec;

  ebml_element_cptr m_level1_element_cp, m_track_headers_cp;
  EbmlMaster *m_level1_element, *m_master, *m_sub_master;

  uint64_t m_track_uid;
  track_type m_track_type;

  std::string m_file_name;
  kax_analyzer_c *m_analyzer;

public:
  target_c();
  virtual ~target_c();

  virtual void validate() = 0;

  virtual void dump_info() const = 0;

  virtual void add_change(change_c::change_type_e type, const std::string &spec);

  virtual bool operator ==(target_c const &cmp) const = 0;
  virtual bool operator !=(target_c const &cmp) const;

  virtual void set_level1_element(ebml_element_cptr level1_element, ebml_element_cptr track_headers = ebml_element_cptr{});

  virtual bool has_changes() const = 0;
  virtual bool has_content_been_modified() const;

  virtual void execute() = 0;
  virtual void execute_change(kax_analyzer_c &analyzer);

  virtual std::string const &get_spec() const;
  virtual uint64_t get_track_uid() const;
  virtual EbmlMaster *get_level1_element() const;
  virtual std::tuple<EbmlMaster *, EbmlMaster *> get_masters() const;

  virtual bool write_elements_set_to_default_value() const;
  virtual bool add_mandatory_elements_if_missing() const;

protected:
  virtual void add_or_replace_all_master_elements(EbmlMaster *source);
};
using target_cptr = std::shared_ptr<target_c>;
