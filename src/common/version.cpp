/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   version information

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>
#include <QTimeZone>

#include <ebml/EbmlVersion.h>
#include <matroska/KaxVersion.h>

#include "common/debugging.h"
#include "common/hacks.h"
#include "common/mkvtoolnix_version.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/version.h"

constexpr auto VERSIONNAME = "It's My Life";

version_number_t::version_number_t(const std::string &s)
  : valid{}
{
  if (debugging_c::requested("version_check"))
    mxinfo(fmt::format("version check: Parsing {0}\n", s));

  // Match the following:
  // 4.4.0
  // 4.4.0.5 build 123
  // 4.4.0-build20101201-123
  // mkvmerge v4.4.0
  // * Optional prefix "mkvmerge v"
  // * An arbitrary number of digits separated by dots
  // * Optional build number that can have two forms:
  //   - " build nnn"
  //   - "-buildYYYYMMDD-nnn" (date is ignored)
  static QRegularExpression s_version_number_re{
    "^(?:mkv[a-z]+[ \\t]+v)?"  // Optional prefix mkv... v
    "((?:\\d+\\.)*)(\\d+)"     // An arbitrary number of digitss separated by dots; $1 & $2
    "(?:"                      // Optional build number including its prefix
      "(?:[ \\t]*build[ \\t]*" //   Build number prefix: either " build " or...
       "|-build\\d{8}-)"       //   ... "-buildYYYYMMDD-"
      "(\\d+)"                 //   The build number itself; $3
    ")?$"
  };

  auto matches = s_version_number_re.match(Q(s));
  if (!matches.hasMatch())
    return;

  valid          = true;
  auto str_parts = mtx::string::split(to_utf8(matches.captured(1)) + to_utf8(matches.captured(2)), ".");

  for (auto const &str_part : str_parts) {
    parts.push_back(0);
    if (!mtx::string::parse_number(str_part, parts.back())) {
      valid = false;
      break;
    }
  }

  if (matches.capturedLength(3) && !mtx::string::parse_number(to_utf8(matches.captured(3)), build))
    valid = false;

  if (parts.empty())
    valid = false;

  if (debugging_c::requested("version_check"))
    mxinfo(fmt::format("version check: parse OK; result: {0}\n", to_string()));
}

int
version_number_t::compare(const version_number_t &cmp)
  const
{
  for (int idx = 0, num_parts = std::max(parts.size(), cmp.parts.size()); idx < num_parts; ++idx) {
    auto this_num = static_cast<unsigned int>(idx) < parts.size()     ? parts[idx]     : 0;
    auto cmp_num  = static_cast<unsigned int>(idx) < cmp.parts.size() ? cmp.parts[idx] : 0;

    if (this_num < cmp_num)
      return -1;

    if (this_num > cmp_num)
      return 1;
  }

  return build < cmp.build ? -1
       : build > cmp.build ?  1
       :                      0;
}

bool
version_number_t::operator <(const version_number_t &cmp)
  const
{
  return compare(cmp) == -1;
}

bool
version_number_t::operator ==(const version_number_t &cmp)
  const
{
  return compare(cmp) == 0;
}

std::string
version_number_t::to_string()
  const
{
  if (!valid)
    return "<invalid>";

  std::string v;

  for (auto const &part : parts) {
    if (!v.empty())
      v += ".";
    v += fmt::to_string(part);
  }

  if (0 != build)
    v += " build " + fmt::to_string(build);

  return v;
}

mtx_release_version_t::mtx_release_version_t()
  : current_version(get_current_version())
  , valid(false)
{
}

std::string
get_version_info(const std::string &program,
                 version_info_flags_e flags) {
  std::vector<std::string> info;

  if (!program.empty())
    info.push_back(program);
  info.push_back(fmt::format("v{0} ('{1}')", MKVTOOLNIX_VERSION, VERSIONNAME));

  if (flags & vif_architecture)
    info.push_back(fmt::format("{0}-bit", __SIZEOF_POINTER__ * 8));

  return mtx::string::join(info, " ");
}

int
compare_current_version_to(const std::string &other_version_str) {
  return version_number_t(MKVTOOLNIX_VERSION).compare(version_number_t(other_version_str));
}

version_number_t
get_current_version() {
  return version_number_t(MKVTOOLNIX_VERSION);
}

mtx_release_version_t
parse_latest_release_version(mtx::xml::document_cptr const &doc) {
  if (!doc)
    return {};

  mtx_release_version_t release;

  release.latest_source             = version_number_t{doc->select_node("/mkvtoolnix-releases/latest-source/version").node().child_value()};
  release.latest_windows_build      = version_number_t{fmt::format("{0} build {1}",
                                                                   doc->select_node("/mkvtoolnix-releases/latest-windows-pre/version").node().child_value(),
                                                                   doc->select_node("/mkvtoolnix-releases/latest-windows-pre/build").node().child_value())};
  release.valid                     = release.latest_source.valid;
  release.urls["general"]           = doc->select_node("/mkvtoolnix-releases/latest-source/url").node().child_value();
  release.urls["source_code"]       = doc->select_node("/mkvtoolnix-releases/latest-source/source-code-url").node().child_value();
  release.urls["windows_pre_build"] = doc->select_node("/mkvtoolnix-releases/latest-windows-pre/url").node().child_value();

  for (auto arch : std::vector<std::string>{ "x86", "amd64" })
    for (auto package : std::vector<std::string>{ "installer", "portable" })
      release.urls["windows_"s + arch + "_" + package] = doc->select_node(("/mkvtoolnix-releases/latest-windows-binary/"s + package + "-url/" + arch).c_str()).node().child_value();

  if (debugging_c::requested("version_check")) {
    std::stringstream urls;
    std::for_each(release.urls.begin(), release.urls.end(), [&urls](auto const &kv) { urls << " " << kv.first << ":" << kv.second; });
    mxdebug(fmt::format("update check: current {0} latest source {1} latest winpre {2} URLs{3}\n",
                        release.current_version.to_string(), release.latest_source.to_string(), release.latest_windows_build.to_string(), urls.str()));
  }

  return release;
}

segment_info_data_t
get_default_segment_info_data(std::string const &application) {
  segment_info_data_t data{};

  if (!mtx::hacks::is_engaged(mtx::hacks::NO_VARIABLE_DATA)) {
    data.muxing_app   = fmt::format("libebml v{0} + libmatroska v{1}", libebml::EbmlCodeVersion, libmatroska::KaxCodeVersion);
    data.writing_app  = get_version_info(application, static_cast<version_info_flags_e>(vif_full | vif_untranslated));
    data.writing_date = QDateTime::currentDateTimeUtc();

  } else {
    data.muxing_app   = "no_variable_data";
    data.writing_app  = "no_variable_data";
    data.writing_date = QDateTime::fromSecsSinceEpoch(0, QTimeZone::utc());
  }

  return data;
}
