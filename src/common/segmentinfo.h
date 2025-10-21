/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   segment info parser/writer functions

   Written by Moritz Bunkus <mo@bunkus.online>
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxSemantic.h>

using kax_info_cptr = std::shared_ptr<libmatroska::KaxInfo>;
