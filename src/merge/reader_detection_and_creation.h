/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   output handling

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#ifndef MTX_MERGE_READER_DETECTION_AND_TYPE_H
#define MTX_MERGE_READER_DETECTION_AND_TYPE_H

#include "common/common_pch.h"

struct filelist_t;

void get_file_type(filelist_t &file);
void create_readers();

#endif // MTX_MERGE_READER_DETECTION_AND_TYPE_H
