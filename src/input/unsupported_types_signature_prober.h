/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Simple signature-based prober for unsupported file types.

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mm_io.h"

class unsupported_types_signature_prober_c {
public:
  static void probe_file(mm_io_c &io);
};
