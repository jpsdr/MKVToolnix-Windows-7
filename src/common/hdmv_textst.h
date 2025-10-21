/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for HDMV TextST subtitles

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace mtx::hdmv_textst {

// Blu-ray Disc Read-Only Format: table 9-70 Segment types
enum segment_type_e {
  dialog_style_segment        = 0x81,
  dialog_presentation_segment = 0x82,
};

// Blu-ray Disc Read-Only Format: table 5-22 Character code
enum character_code_e {         // corresponding iconv charset names
  utf8      = 0x01,             // UTF-8
  utf16     = 0x02,             // UTF-16BE
  shift_jis = 0x03,             // SHIFT-JIS
  euc_kr    = 0x04,             // EUC-KR
  gb18030   = 0x05,             // GB18030
  gb2312    = 0x06,             // GB2312
  big5      = 0x07,             // BIG-5
};

::timestamp_c get_timestamp(uint8_t const *buf);
void put_timestamp(uint8_t *buf, ::timestamp_c const &timestamp);

}
