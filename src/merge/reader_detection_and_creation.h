/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   output handling

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#pragma once

#include "common/common_pch.h"

struct filelist_t;

std::unique_ptr<generic_reader_c> probe_file_format(filelist_t &file);
void read_file_headers();
