/** \brief command line parsing

   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/ebml.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "extract/extract_cli_parser.h"
#include "extract/options.h"

extract_cli_parser_c::extract_cli_parser_c(const std::vector<std::string> &args)
  : mtx::cli::parser_c{args}
  , m_cli_type{cli_type_e::unknown}
  , m_current_mode{m_options.m_modes.begin()}
  , m_num_unknown_args{}
  , m_debug{"extract_cli_parser"}
{
  set_default_values();
}

void
extract_cli_parser_c::set_default_values() {
  m_charset                = "UTF-8";
  m_extract_cuesheet       = false;
  m_extract_blockadd_level = -1;
  m_target_mode            = track_spec_t::tm_normal;
}

void
extract_cli_parser_c::init_parser() {
  add_information(YT("mkvextract <source-filename> <mode1> [options] <extraction-spec1> [<mode2> [options] <extraction-spec2>…]"));

  add_section_header(YT("Modes"));
  add_information(YT("tracks [options] [TID1:out1 [TID2:out2 ...]]"));
  add_information(YT("tags [options] out.xml"));
  add_information(YT("attachments [options] [AID1:out1 [AID2:out2 ...]]"));
  add_information(YT("chapters [options] out.xml"));
  add_information(YT("cuesheet [options] out.cue"));
  add_information(YT("timestamps_v2 [TID1:out1 [TID2:out2 ...]]"));
  add_information(YT("cues [options] [TID1:out1 [TID2:out2 ...]]"));

  add_section_header(YT("Other options"));
  add_information(YT("mkvextract <-h|--help|-V|--version>"));

  add_separator();

  add_information(std::vector<translatable_string_c>{
      YT("The first argument must be the name of source file."),
      YT("All other arguments either switch to a certain extraction mode, change options for the currently active mode or specify what to extract into which file."),
      YT("Multiple modes can be used in the same invocation of mkvextract allowing the extraction of multiple things in a single pass."),
      YT("Most options can only be used in certain modes with a few options applying to all modes.") });

  add_section_header(YT("Global options"));
  add_option("f|parse-fully", std::bind(&extract_cli_parser_c::set_parse_fully, this), YT("Parse the whole file instead of relying on the index."));

  add_common_options();

  add_section_header(YT("Track extraction"));
  add_information(YT("The first mode extracts some tracks to external files."));
  add_option("c=charset",      std::bind(&extract_cli_parser_c::set_charset,  this), YT("Convert text subtitles to this charset (default: UTF-8)."));
  add_option("cuesheet",       std::bind(&extract_cli_parser_c::set_cuesheet, this), YT("Also try to extract the cue sheet from the chapter information and tags for this track."));
  add_option("blockadd=level", std::bind(&extract_cli_parser_c::set_blockadd, this), YT("Keep only the BlockAdditions up to this level (default: keep all levels)"));
  add_option("raw",            std::bind(&extract_cli_parser_c::set_raw,      this), YT("Extract the data to a raw file."));
  add_option("fullraw",        std::bind(&extract_cli_parser_c::set_fullraw,  this), YT("Extract the data to a raw file including the CodecPrivate as a header."));
  add_informational_option("TID:out", YT("Write track with the ID TID to the file 'out'."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" tracks 2:audio.ogg -c ISO8859-1 3:subs.srt"));
  add_separator();

  add_section_header(YT("Tag extraction"));
  add_information(YT("The second mode extracts the tags, converts them to XML and writes them to an output file."));
  add_option("T|no-track-tags", std::bind(&extract_cli_parser_c::set_no_track_tags, this), YT("Do not export track tags."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" tags movie_tags.xml"));

  add_section_header(YT("Attachment extraction"));

  add_information(YT("The third mode extracts attachments from the source file."));
  add_informational_option("AID:outname", YT("Write the attachment with the ID 'AID' to 'outname'."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" attachments 4:cover.jpg"));

  add_section_header(YT("Chapter extraction"));
  add_information(YT("The fourth mode extracts the chapters, converts them to XML and writes them to an output file."));
  add_option("s|simple",                 std::bind(&extract_cli_parser_c::set_simple,          this), YT("Exports the chapter information in the simple format used in OGM tools (CHAPTER01=... CHAPTER01NAME=...)."));
  add_option("simple-language=language", std::bind(&extract_cli_parser_c::set_simple_language, this), YT("Uses the chapter names of the specified language for extraction instead of the first chapter name found."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" chapters movie_chapters.xml"));

  add_section_header(YT("Cue sheet extraction"));

  add_information(YT("The fifth mode tries to extract chapter information and tags and outputs them as a cue sheet. This is the reverse of using a cue sheet with "
                     "mkvmerge's '--chapters' option."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"audiofile.mka\" cuesheet audiofile.cue"));

  add_section_header(YT("Timestamp extraction"));

  add_information(YT("The sixth mode finds the timestamps of all blocks for a track and outputs a timestamp v2 file with these timestamps."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" timestamps_v2 1:timestamps_track1.txt"));

  add_section_header(YT("Cue extraction"));
  add_information(YT("This mode extracts cue information for some tracks to external text files."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" cues 0:cues_track0.txt"));

  add_section_header(YT("Extracting multiple items at the same time"));
  add_information(YT("Multiple modes can be used in the same invocation of mkvextract allowing the extraction of multiple things in a single pass."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" tracks 0:video.h264 1:audio.aac timestamps_v2 0:timestamps.video.txt chapters chapters.xml tags tags.xml"));

  add_separator();

  add_hook(mtx::cli::parser_c::ht_unknown_option, std::bind(&extract_cli_parser_c::handle_unknown_arg, this));
}

#undef OPT

void
extract_cli_parser_c::assert_mode(options_c::extraction_mode_e mode) {
  if      ((options_c::em_tracks   == mode) && (m_current_mode->m_extraction_mode != mode))
    mxerror(fmt::format(FY("'{0}' is only allowed when extracting tracks.\n"),   m_current_arg));

  else if ((options_c::em_chapters == mode) && (m_current_mode->m_extraction_mode != mode))
    mxerror(fmt::format(FY("'{0}' is only allowed when extracting chapters.\n"), m_current_arg));
}

void
extract_cli_parser_c::set_parse_fully() {
  m_options.m_parse_mode = kax_analyzer_c::parse_mode_full;
}

void
extract_cli_parser_c::set_charset() {
  assert_mode(options_c::em_tracks);
  m_charset = m_next_arg;
}

void
extract_cli_parser_c::set_cuesheet() {
  assert_mode(options_c::em_tracks);
  m_extract_cuesheet = true;
}

void
extract_cli_parser_c::set_blockadd() {
  assert_mode(options_c::em_tracks);
  if (!mtx::string::parse_number(m_next_arg, m_extract_blockadd_level) || (-1 > m_extract_blockadd_level))
    mxerror(fmt::format(FY("Invalid BlockAddition level in argument '{0}'.\n"), m_next_arg));
}

void
extract_cli_parser_c::set_raw() {
  assert_mode(options_c::em_tracks);
  m_target_mode = track_spec_t::tm_raw;
}

void
extract_cli_parser_c::set_fullraw() {
  assert_mode(options_c::em_tracks);
  m_target_mode = track_spec_t::tm_full_raw;
}

void
extract_cli_parser_c::set_no_track_tags() {
  assert_mode(options_c::em_tags);
  m_current_mode->m_no_track_tags = true;
}

void
extract_cli_parser_c::set_simple() {
  assert_mode(options_c::em_chapters);
  m_current_mode->m_simple_chapter_format = true;
}

void
extract_cli_parser_c::set_simple_language() {
  assert_mode(options_c::em_chapters);

  m_current_mode->m_simple_chapter_language = mtx::bcp47::language_c::parse(m_next_arg);
  if (!m_current_mode->m_simple_chapter_language.has_valid_iso639_code())
    mxerror(fmt::format(FY("'{0}' is neither a valid ISO 639-2 nor a valid ISO 639-1 code. See 'mkvmerge --list-languages' for a list of all languages and their respective ISO 639-2 codes.\n"), m_next_arg));
}

std::optional<options_c::extraction_mode_e>
extract_cli_parser_c::extraction_mode_from_string(std::string const &mode_string) {
  static std::unordered_map<std::string, options_c::extraction_mode_e> s_mode_map{
    { "tracks",        options_c::em_tracks        },
    { "tags",          options_c::em_tags          },
    { "attachments",   options_c::em_attachments   },
    { "chapters",      options_c::em_chapters      },
    { "cuesheet",      options_c::em_cuesheet      },
    { "timecodes_v2",  options_c::em_timestamps_v2 },
    { "timestamps_v2", options_c::em_timestamps_v2 },
    { "cues",          options_c::em_cues          },
  };

  auto itr = s_mode_map.find(mode_string);
  if (itr != s_mode_map.end())
    return itr->second;

  return std::nullopt;
}

std::string
extract_cli_parser_c::extraction_mode_to_string(std::optional<options_c::extraction_mode_e> mode) {
  static std::unordered_map<options_c::extraction_mode_e, std::string, mtx::hash<options_c::extraction_mode_e>> s_mode_map{
    { options_c::em_unknown,       "unknown"       },
    { options_c::em_tracks,        "tracks"        },
    { options_c::em_tags,          "tags"          },
    { options_c::em_attachments,   "attachments"   },
    { options_c::em_chapters,      "chapters"      },
    { options_c::em_cuesheet,      "cuesheet"      },
    { options_c::em_timestamps_v2, "timestamps_v2" },
    { options_c::em_cues,          "cues"          },
  };

  return mode ? s_mode_map[*mode] : "<invalid>"s;
}

void
extract_cli_parser_c::set_cli_mode() {
  auto mode = extraction_mode_from_string(m_current_arg);

  mxdebug_if(m_debug,
             fmt::format("set_cli_mode: current mode {0} new mode {1} current arg {2}\n",
                         extraction_mode_to_string(m_current_mode->m_extraction_mode), extraction_mode_to_string(mode), m_current_arg));

  if (mode) {
    m_cli_type                        = cli_type_e::single;
    m_current_mode->m_extraction_mode = *mode;

    mxdebug_if(m_debug, fmt::format("set_cli_mode: new mode is single\n"));

  } else if (boost::filesystem::is_regular_file(mtx::fs::to_path(m_current_arg))) {
    m_cli_type            = cli_type_e::multiple;
    m_options.m_file_name = m_current_arg;

    mxdebug_if(m_debug, fmt::format("set_cli_mode: new mode is multiple\n"));

  } else
    mxerror(fmt::format(FY("Unknown mode '{0}'.\n"), m_current_arg));
}

void
extract_cli_parser_c::handle_unknown_arg_single_mode() {
  mxdebug_if(m_debug, fmt::format("handle_unknown_arg_single_mode: num unknown {0} current {1}\n", m_num_unknown_args, m_current_arg));

  if (2 == m_num_unknown_args)
    m_options.m_file_name = m_current_arg;

  else if (2 < m_num_unknown_args)
    add_extraction_spec();
}

void
extract_cli_parser_c::handle_unknown_arg_multiple_mode() {
  // First unknown argument is the file name, and that's already been
  // handled.
  if (1 == m_num_unknown_args)
    return;

  auto new_mode = extraction_mode_from_string(m_current_arg);

  mxdebug_if(m_debug,
             fmt::format("handle_unknown_arg_multiple_mode: current mode {0} new mode {1} current arg {2} num modes {3}\n",
                         extraction_mode_to_string(m_current_mode->m_extraction_mode), extraction_mode_to_string(new_mode), m_current_arg, m_options.m_modes.size()));

  if (!new_mode) {
    if (mtx::included_in(m_current_mode->m_extraction_mode, options_c::em_chapters, options_c::em_cuesheet, options_c::em_tags))
      m_current_mode->m_output_file_name = m_current_arg;
    else
      add_extraction_spec();

    return;
  }

  if (m_current_mode->m_extraction_mode == options_c::em_unknown) {
    m_current_mode->m_extraction_mode = *new_mode;
    return;
  }

  m_current_mode = m_options.get_options_for_mode(*new_mode);
}

void
extract_cli_parser_c::handle_unknown_arg() {
  ++m_num_unknown_args;

  mxdebug_if(m_debug,
             fmt::format("handle_unknown_arg: num unknown {0} cli type {1}\n",
                         m_num_unknown_args,
                           m_cli_type == cli_type_e::unknown ? "unknown"
                         : m_cli_type == cli_type_e::single  ? "single"
                         :                                     "multiple"));

  if (cli_type_e::unknown == m_cli_type)
    set_cli_mode();

  if (cli_type_e::single == m_cli_type)
    handle_unknown_arg_single_mode();

  else
    handle_unknown_arg_multiple_mode();
}

void
extract_cli_parser_c::add_extraction_spec() {
  if (   (options_c::em_tracks        != m_current_mode->m_extraction_mode)
      && (options_c::em_cues          != m_current_mode->m_extraction_mode)
      && (options_c::em_timestamps_v2 != m_current_mode->m_extraction_mode)
      && (options_c::em_attachments   != m_current_mode->m_extraction_mode))
    mxerror(fmt::format(FY("Unrecognized command line option '{0}'.\n"), m_current_arg));

  QRegularExpression s_track_id_re{"^(\\d+)(?::(.+))?$"};

  auto matches = s_track_id_re.match(Q(m_current_arg));

  if (!matches.hasMatch()) {
    if (options_c::em_attachments == m_current_mode->m_extraction_mode)
      mxerror(fmt::format(FY("Invalid attachment ID/file name specification in argument '{0}'.\n"), m_current_arg));
    else
      mxerror(fmt::format(FY("Invalid track ID/file name specification in argument '{0}'.\n"), m_current_arg));
  }

  track_spec_t track;

  mtx::string::parse_number(to_utf8(matches.captured(1)), track.tid);

  if (m_used_tids[m_current_mode->m_extraction_mode][track.tid])
    mxerror(fmt::format(FY("The ID '{0}' has already been used for another destination file.\n"), track.tid));
  m_used_tids[m_current_mode->m_extraction_mode][track.tid] = true;

  std::string output_file_name;
  if (matches.capturedLength(2))
    output_file_name = to_utf8(matches.captured(2));

  if (output_file_name.empty()) {
    if (options_c::em_attachments == m_current_mode->m_extraction_mode)
      mxinfo(Y("No destination file name specified, will use attachment name.\n"));
    else
      mxerror(fmt::format(FY("Missing destination file name in argument '{0}'.\n"), m_current_arg));
  }

  track.out_name               = output_file_name;
  track.sub_charset            = m_charset;
  track.extract_cuesheet       = m_extract_cuesheet;
  track.extract_blockadd_level = m_extract_blockadd_level;
  track.target_mode            = m_target_mode;
  m_current_mode->m_tracks.push_back(track);

  set_default_values();
}

void
extract_cli_parser_c::check_for_identical_source_and_destination_file_names() {
  if (m_options.m_file_name.empty())
    return;

  auto source_file_name = boost::filesystem::absolute(mtx::fs::to_path(m_options.m_file_name));

  for (auto const &mode_option : m_options.m_modes) {
    if (!mode_option.m_output_file_name.empty() && (boost::filesystem::absolute(mtx::fs::to_path(mode_option.m_output_file_name)) == source_file_name))
      mxerror(fmt::format(FY("The name of one of the destination files is the same as the name of the source file ({0}).\n"), source_file_name.string()));

    for (auto const &track_spec : mode_option.m_tracks)
      if (!track_spec.out_name.empty() && (boost::filesystem::absolute(mtx::fs::to_path(track_spec.out_name)) == source_file_name))
        mxerror(fmt::format(FY("The name of one of the destination files is the same as the name of the source file ({0}).\n"), source_file_name.string()));
  }
}

options_c
extract_cli_parser_c::run() {
  init_parser();

  parse_args();

  m_options.merge_tracks_and_timestamps_targets();

  check_for_identical_source_and_destination_file_names();

  if (m_debug)
    m_options.dump();

  return m_options;
}
