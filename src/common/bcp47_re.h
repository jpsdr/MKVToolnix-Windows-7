/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   BCP 47 language tags

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QRegularExpression>

namespace mtx::bcp47 {

extern std::optional<QRegularExpression> s_bcp47_re, s_bcp47_grandfathered_re;

}
