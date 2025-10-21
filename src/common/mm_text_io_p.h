/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_proxy_io_p.h"

class mm_text_io_c;

class mm_text_io_private_c : public mm_proxy_io_private_c {
public:
  byte_order_mark_e byte_order_mark{byte_order_mark_e::none};
  unsigned int bom_len{};
  bool uses_carriage_returns{}, uses_newlines{}, eol_style_detected{};

  explicit mm_text_io_private_c(mm_io_cptr const &in);
};
