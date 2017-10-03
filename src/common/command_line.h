/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Definitions for command line helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace cli {

extern std::string g_usage_text, g_version_info;
extern bool g_gui_mode;

void display_usage(int exit_code = 0);
std::vector<std::string> args_in_utf8(int argc, char **argv);
bool handle_common_args(std::vector<std::string> &args, const std::string &redirect_output_short);

}}
