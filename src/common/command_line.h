/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Definitions for command line helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx::cli {

extern std::string g_usage_text;
extern bool g_gui_mode, g_abort_on_warnings;

void display_usage(int exit_code = 0);
std::vector<std::string> args_in_utf8(int argc, char **argv);
bool handle_common_args(std::vector<std::string> &args, const std::string &redirect_output_short);

}
