/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/translation.h"
#include "merge/id_result.h"
#include "merge/output_control.h"

static void
output_container_unsupported_text(std::string const &filename,
                                  translatable_string_c const &info) {
  if (g_identifying) {
    mxinfo(fmt::format(FY("File '{0}': unsupported container: {1}\n"), filename, info));
    mxexit(3);

  } else
    mxerror(fmt::format(FY("The file '{0}' is a non-supported file type ({1}).\n"), filename, info));
}

static void
output_container_unsupported_json(std::string const &filename,
                                  translatable_string_c const &info) {
  auto json = nlohmann::json{
    { "identification_format_version", ID_JSON_FORMAT_VERSION },
    { "file_name",                     filename               },
    { "container", {
        { "recognized", true                  },
        { "supported",  false                 },
        { "type",       info.get_translated() },
      } },
  };

  display_json_output(json);

  mxexit(0);
}

void
id_result_container_unsupported(std::string const &filename,
                                translatable_string_c const &info) {
  if (identification_output_format_e::json == g_identification_output_format)
    output_container_unsupported_json(filename, info);
  else
    output_container_unsupported_text(filename, info);
}
