/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/os.h"

#include <functional>

#include <ebml/EbmlElement.h>

#include "common/json.h"
#include "common/locale.h"
#include "common/mm_io.h"

constexpr auto MXMSG_ERROR   =  5;
constexpr auto MXMSG_WARNING = 10;
constexpr auto MXMSG_INFO    = 15;

using mxmsg_handler_t = std::function<void(unsigned int level, std::string const &)>;
void set_mxmsg_handler(unsigned int level, mxmsg_handler_t const &handler);

extern bool g_suppress_info, g_suppress_warnings;
extern std::string g_stdio_charset;
extern charset_converter_cptr g_cc_stdio;
extern std::shared_ptr<mm_io_c> g_mm_stdio;

void redirect_stdio(const mm_io_cptr &new_stdio);
bool stdio_redirected();

void redirect_warnings_and_errors_to_json();
void display_json_output(nlohmann::json json);

void init_common_output(bool no_charset_detection);
void set_cc_stdio(const std::string &charset);

void mxmsg(unsigned int level, std::string message);

void mxinfo(const std::string &info);
void mxinfo(const std::wstring &info);

void mxwarn(const std::string &warning);
void mxerror(const std::string &error);

void mxinfo_fn(const std::string &file_name, const std::string &info);
void mxinfo_tid(const std::string &file_name, int64_t track_id, const std::string &info);

void mxwarn_fn(const std::string &file_name, const std::string &info);
void mxwarn_tid(const std::string &file_name, int64_t track_id, const std::string &warning);

void mxerror_fn(const std::string &file_name, const std::string &error);
void mxerror_tid(const std::string &file_name, int64_t track_id, const std::string &error);
