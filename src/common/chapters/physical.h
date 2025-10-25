/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions of chapter physical equivalent values

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx::chapters {

// see http://www.matroska.org/technical/specs/index.html#physical
constexpr auto PHYSEQUIV_SET       = 70;
constexpr auto PHYSEQUIV_PACKAGE   = 70;

constexpr auto PHYSEQUIV_CD        = 60;
constexpr auto PHYSEQUIV_12INCH    = 60;
constexpr auto PHYSEQUIV_10INCH    = 60;
constexpr auto PHYSEQUIV_7INCH     = 60;
constexpr auto PHYSEQUIV_TAPE      = 60;
constexpr auto PHYSEQUIV_MINIDISC  = 60;
constexpr auto PHYSEQUIV_DAT       = 60;
constexpr auto PHYSEQUIV_DVD       = 60;
constexpr auto PHYSEQUIV_VHS       = 60;
constexpr auto PHYSEQUIV_LASERDISC = 60;

constexpr auto PHYSEQUIV_SIDE      = 50;

constexpr auto PHYSEQUIV_LAYER     = 40;

constexpr auto PHYSEQUIV_SESSION   = 30;

constexpr auto PHYSEQUIV_TRACK     = 20;

constexpr auto PHYSEQUIV_INDEX     = 10;

}
