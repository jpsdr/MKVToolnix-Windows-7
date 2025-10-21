/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions for a "variable sized integer" helper class

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlId.h>

#include "common/mm_io.h"

class vint_c {
public:
  int64_t m_value;
  int m_coded_size;
  bool m_is_set;

public:
  enum read_mode_e {
    rm_normal,
    rm_ebml_id,
  };

  vint_c();
  vint_c(int64_t value, int coded_size);
  vint_c(libebml::EbmlId const &id);
  bool is_unknown();
  bool is_valid();

  libebml::EbmlId to_ebml_id() const;

public:                         // static functions
  static vint_c read(mm_io_c &in, read_mode_e read_mode = rm_normal);
  static vint_c read(mm_io_cptr const &in, read_mode_e read_mode = rm_normal);

  static vint_c read_ebml_id(mm_io_c &in);
  static vint_c read_ebml_id(mm_io_cptr const &in);
};
