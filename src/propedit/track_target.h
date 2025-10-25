/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "propedit/change.h"
#include "propedit/target.h"

class track_target_c: public target_c {
public:
  enum selection_mode_e {
    sm_undefined,
    sm_by_number,
    sm_by_uid,
    sm_by_position,
    sm_by_type_and_position,
  };

  selection_mode_e m_selection_mode;
  uint64_t m_selection_param;
  track_type m_selection_track_type;

  std::vector<change_cptr> m_changes;

public:
  track_target_c(std::string const &spec);
  virtual ~track_target_c() override;

  virtual void validate() override;
  virtual void look_up_property_elements();

  virtual void add_change(change_c::change_type_e type, const std::string &spec) override;
  virtual void parse_spec(std::string const &spec);

  virtual void set_level1_element(ebml_element_cptr level1_element_cp, ebml_element_cptr track_headers_cp) override;
  virtual void dump_info() const override;

  virtual bool operator ==(target_c const &cmp) const override;

  virtual bool has_changes() const override;
  virtual bool has_add_or_set_change() const;

  virtual void execute() override;

  virtual void merge_changes(track_target_c &other);

protected:
  virtual bool non_track_target() const;
  virtual bool sub_master_is_track() const;
  virtual bool requires_sub_master() const;
};
