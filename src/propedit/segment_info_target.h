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

class segment_info_target_c: public target_c {
public:
  std::vector<change_cptr> m_changes;

public:
  segment_info_target_c();
  virtual ~segment_info_target_c() override;

  virtual void validate() override;
  virtual void look_up_property_elements();

  virtual void add_change(change_c::change_type_e type, const std::string &spec) override;
  virtual void dump_info() const override;

  virtual bool operator ==(target_c const &cmp) const override;

  virtual bool has_changes() const override;

  virtual void execute() override;
};
