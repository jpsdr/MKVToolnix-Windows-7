/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Definitions for tag target types

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_TAG_TARGET_TYPE_H
#define MTX_COMMON_TAG_TARGET_TYPE_H

namespace mtx { namespace tags {

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

}}
#endif // MTX_COMMON_TAGS_TARGET_TYPE_H
