/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   segment info parser/writer functions

   Written by Moritz Bunkus <moritz@bunkus.org>
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxInfo.h>

using namespace libebml;
using namespace libmatroska;

using kax_info_cptr = std::shared_ptr<KaxInfo>;
