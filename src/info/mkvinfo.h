/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definition of global variables and functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "info/options.h"

void console_main(options_c const &options);
void setup(char const *argv0, const std::string &locale = "");
void cleanup();

int ui_run(int argc, char **argv, options_c const &options);
bool ui_graphical_available();
