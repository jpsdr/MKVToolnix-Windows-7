/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper functions for unique random numbers

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/container.h"
#include "common/hacks.h"
#include "common/random.h"
#include "common/unique_numbers.h"

static std::vector<uint64_t> s_random_unique_numbers[4];
static std::unordered_map<unique_id_category_e, bool, mtx::hash<unique_id_category_e>> s_ignore_unique_numbers;

static void
assert_valid_category(unique_id_category_e category) {
  assert((UNIQUE_TRACK_IDS <= category) && (UNIQUE_ATTACHMENT_IDS >= category));
}

void
clear_list_of_unique_numbers(unique_id_category_e category) {
  assert((UNIQUE_ALL_IDS <= category) && (UNIQUE_ATTACHMENT_IDS >= category));

  if (UNIQUE_ALL_IDS == category) {
    int i;
    for (i = 0; 4 > i; ++i)
      clear_list_of_unique_numbers((unique_id_category_e)i);
  } else
    s_random_unique_numbers[category].clear();
}

bool
is_unique_number(uint64_t number,
                 unique_id_category_e category) {
  assert_valid_category(category);

  if (s_ignore_unique_numbers[category])
    return true;

  if (mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA))
    return true;

  auto &numbers = s_random_unique_numbers[category];
  return std::find_if(numbers.begin(), numbers.end(), [=](uint64_t stored_number) { return number == stored_number; }) == numbers.end();
}

void
add_unique_number(uint64_t number,
                  unique_id_category_e category) {
  assert_valid_category(category);

  if (mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA))
    s_random_unique_numbers[category].push_back(s_random_unique_numbers[category].size() + 1);
  else
    s_random_unique_numbers[category].push_back(number);
}

void
remove_unique_number(uint64_t number,
                     unique_id_category_e category) {
  assert_valid_category(category);

  auto &numbers = s_random_unique_numbers[category];
  numbers.erase(std::remove_if(numbers.begin(), numbers.end(), [=](uint64_t stored_number) { return number == stored_number; }), numbers.end());
}

uint64_t
create_unique_number(unique_id_category_e category) {
  assert_valid_category(category);

  if (mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA)) {
    s_random_unique_numbers[category].push_back(s_random_unique_numbers[category].size() + 1);
    return s_random_unique_numbers[category].size();
  }

  uint64_t random_number;
  do {
    random_number = random_c::generate_64bits();
  } while ((random_number == 0) || !is_unique_number(random_number, category));
  add_unique_number(random_number, category);

  return random_number;
}

void
ignore_unique_numbers(unique_id_category_e category) {
  assert_valid_category(category);
  s_ignore_unique_numbers[category] = true;
}
