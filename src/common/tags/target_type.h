/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for tag target types

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx::tags {

enum target_type_e {
// see http://www.matroska.org/technical/specs/tagging/index.html#targettypes
  Collection = 70,

  Edition    = 60,
  Issue      = 60,
  Volume     = 60,
  Opus       = 60,
  Season     = 60,
  Sequel     = 60,

  Album      = 50,
  Opera      = 50,
  Concert    = 50,
  Movie      = 50,
  Episode    = 50,

  Part       = 40,
  Session    = 40,

  Track      = 30,
  Song       = 30,
  Chapter    = 30,

  Subtrack   = 20,
  Movement   = 20,
  Scene      = 20,

  Shot       = 10,

  Unknown    =  0,
};

}
