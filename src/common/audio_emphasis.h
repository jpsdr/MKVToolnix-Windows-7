/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the audio emphasis helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/translation.h"

class audio_emphasis_c {
public:
  static std::vector<std::string> s_modes;
  static std::vector<translatable_string_c> s_translations;

  enum mode_e {
    invalid           = -1,
    unspecified       = -1,
    none              =  0,
    cd_audio          =  1,
    ccit_j_17         =  3,
    fm_50             =  4,
    fm_75             =  5,
    phono_riaa        = 10,
    phono_iec_n78     = 11,
    phono_teldec      = 12,
    phono_emi         = 13,
    phono_columbia_lp = 14,
    phono_london      = 15,
    phono_nartb       = 16,
  };

  static void init();
  static void init_translations();
  static std::string const symbol(unsigned int mode);
  static std::string const translate(unsigned int mode);
  static std::string const displayable_modes_list();
  static mode_e parse_mode(const std::string &str);
  static bool valid_index(int index);
  static void list();
  static int max_index();
};
