/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper functions for creating unique numbers.

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

enum unique_id_category_e {
  UNIQUE_ALL_IDS        = -1,
  UNIQUE_TRACK_IDS      = 0,
  UNIQUE_CHAPTER_IDS    = 1,
  UNIQUE_EDITION_IDS    = 2,
  UNIQUE_ATTACHMENT_IDS = 3
};

void clear_list_of_unique_numbers(unique_id_category_e category);
bool is_unique_number(uint64_t number, unique_id_category_e category);
void add_unique_number(uint64_t number, unique_id_category_e category);
void remove_unique_number(uint64_t number, unique_id_category_e category);
uint64_t create_unique_number(unique_id_category_e category);
void ignore_unique_numbers(unique_id_category_e category);
