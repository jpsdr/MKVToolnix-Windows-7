/** \brief command line parsing

   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/iso639.h"
#include "common/list_utils.h"
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

#define OPT(spec, func, description) add_option(spec, std::bind(&extract_cli_parser_c::func, this), description)

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

  add_information(TSV(YT("The first argument must be the name of source file."),
                      YT("All other arguments either switch to a certain extraction mode, change options for the currently active mode or specify what to extract into which file."),
                      YT("Multiple modes can be used in the same invocation of mkvextract allowing the extraction of multiple things in a single pass."),
                      YT("Most options can only be used in certain modes with a few options applying to all modes.")));

  add_section_header(YT("Global options"));
  OPT("f|parse-fully", set_parse_fully, YT("Parse the whole file instead of relying on the index."));

  add_common_options();

  add_section_header(YT("Track extraction"));
  add_information(YT("The first mode extracts some tracks to external files."));
  OPT("c=charset",      set_charset,  YT("Convert text subtitles to this charset (default: UTF-8)."));
  OPT("cuesheet",       set_cuesheet, YT("Also try to extract the cue sheet from the chapter information and tags for this track."));
  OPT("blockadd=level", set_blockadd, YT("Keep only the BlockAdditions up to this level (default: keep all levels)"));
  OPT("raw",            set_raw,      YT("Extract the data to a raw file."));
  OPT("fullraw",        set_fullraw,  YT("Extract the data to a raw file including the CodecPrivate as a header."));
  add_informational_option("TID:out", YT("Write track with the ID TID to the file 'out'."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" tracks 2:audio.ogg -c ISO8859-1 3:subs.srt"));
  add_separator();

  add_section_header(YT("Tag extraction"));
  add_information(YT("The second mode extracts the tags, converts them to XML and writes them to an output file."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" tags movie_tags.xml"));

  add_section_header(YT("Attachment extraction"));

  add_information(YT("The third mode extracts attachments from the source file."));
  add_informational_option("AID:outname", YT("Write the attachment with the ID 'AID' to 'outname'."));

  add_section_header(YT("Example"));

  add_information(YT("mkvextract \"a movie.mkv\" attachments 4:cover.jpg"));

  add_section_header(YT("Chapter extraction"));
  add_information(YT("The fourth mode extracts the chapters, converts them to XML and writes them to an output file."));
  OPT("s|simple",                 set_simple,          YT("Exports the chapter information in the simple format used in OGM tools (CHAPTER01=... CHAPTER01NAME=...)."));
  OPT("simple-language=language", set_simple_language, YT("Uses the chapter names of the specified language for extraction instead of the first chapter name found."));

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
    mxerror(boost::format(Y("'%1%' is only allowed when extracting tracks.\n"))   % m_current_arg);

  else if ((options_c::em_chapters == mode) && (m_current_mode->m_extraction_mode != mode))
    mxerror(boost::format(Y("'%1%' is only allowed when extracting chapters.\n")) % m_current_arg);
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
  if (!parse_number(m_next_arg, m_extract_blockadd_level) || (-1 > m_extract_blockadd_level))
    mxerror(boost::format(Y("Invalid BlockAddition level in argument '%1%'.\n")) % m_next_arg);
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
extract_cli_parser_c::set_simple() {
  assert_mode(options_c::em_chapters);
  m_current_mode->m_simple_chapter_format = true;
}

void
extract_cli_parser_c::set_simple_language() {
  assert_mode(options_c::em_chapters);

  auto language_idx = map_to_iso639_2_code(m_next_arg);
  if (0 > language_idx)
    mxerror(boost::format(Y("'%1%' is neither a valid ISO639-2 nor a valid ISO639-1 code. See 'mkvmerge --list-languages' for a list of all languages and their respective ISO639-2 codes.\n")) % m_next_arg);

  m_current_mode->m_simple_chapter_language.reset(g_iso639_languages[language_idx].iso639_2_code);
}

boost::optional<options_c::extraction_mode_e>
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

  return boost::none;
}

std::string
extract_cli_parser_c::extraction_mode_to_string(boost::optional<options_c::extraction_mode_e> mode) {
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

  return mode ? s_mode_map[*mode] : std::string{"<invalid>"};
}

void
extract_cli_parser_c::set_cli_mode() {
  auto mode = extraction_mode_from_string(m_current_arg);

  mxdebug_if(m_debug,
             boost::format("set_cli_mode: current mode %1% new mode %2% current arg %3%\n")
             % extraction_mode_to_string(m_current_mode->m_extraction_mode) % extraction_mode_to_string(mode) % m_current_arg);

  if (mode) {
    m_cli_type                        = cli_type_e::single;
    m_current_mode->m_extraction_mode = *mode;

    mxdebug_if(m_debug, boost::format("set_cli_mode: new mode is single\n"));

  } else if (bfs::exists(bfs::path{m_current_arg})) {
    m_cli_type            = cli_type_e::multiple;
    m_options.m_file_name = m_current_arg;

    mxdebug_if(m_debug, boost::format("set_cli_mode: new mode is multiple\n"));

  } else
    mxerror(boost::format(Y("Unknown mode '%1%'.\n")) % m_current_arg);
}

void
extract_cli_parser_c::handle_unknown_arg_single_mode() {
  mxdebug_if(m_debug, boost::format("handle_unknown_arg_single_mode: num unknown %1% current %2%\n") % m_num_unknown_args % m_current_arg);

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
             boost::format("handle_unknown_arg_multiple_mode: current mode %1% new mode %2% current arg %3% num modes %4%\n")
             % extraction_mode_to_string(m_current_mode->m_extraction_mode) % extraction_mode_to_string(new_mode) % m_current_arg % m_options.m_modes.size());

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
             boost::format("handle_unknown_arg: num unknown %1% cli type %2%\n")
             % m_num_unknown_args
             % (  m_cli_type == cli_type_e::unknown ? "unknown"
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
    mxerror(boost::format(Y("Unrecognized command line option '%1%'.\n")) % m_current_arg);

  boost::regex s_track_id_re("^(\\d+)(:(.+))?$", boost::regex::perl);

  boost::smatch matches;
  if (!boost::regex_search(m_current_arg, matches, s_track_id_re)) {
    if (options_c::em_attachments == m_current_mode->m_extraction_mode)
      mxerror(boost::format(Y("Invalid attachment ID/file name specification in argument '%1%'.\n")) % m_current_arg);
    else
      mxerror(boost::format(Y("Invalid track ID/file name specification in argument '%1%'.\n")) % m_current_arg);
  }

  track_spec_t track;

  parse_number(matches[1].str(), track.tid);

  if (m_used_tids[m_current_mode->m_extraction_mode][track.tid])
    mxerror(boost::format(Y("The ID '%1%' has already been used for another destination file.\n")) % track.tid);
  m_used_tids[m_current_mode->m_extraction_mode][track.tid] = true;

  std::string output_file_name;
  if (matches[3].matched)
    output_file_name = matches[3].str();

  if (output_file_name.empty()) {
    if (options_c::em_attachments == m_current_mode->m_extraction_mode)
      mxinfo(Y("No destination file name specified, will use attachment name.\n"));
    else
      mxerror(boost::format(Y("Missing destination file name in argument '%1%'.\n")) % m_current_arg);
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
extract_cli_parser_c::verify_options_multiple_mode() {
  for (auto const &mode_options : m_options.m_modes) {
    if (!mtx::included_in(mode_options.m_extraction_mode, options_c::em_chapters, options_c::em_cuesheet, options_c::em_tags))
      continue;

    auto mode_name = mode_options.m_extraction_mode == options_c::em_chapters ? "chapters"
                   : mode_options.m_extraction_mode == options_c::em_tags     ? "tags"
                   :                                                            "cuesheet";

    if (mode_options.m_output_file_name.empty())
      mxerror(boost::format(Y("Missing destination file name in argument '%1%'.\n")) % mode_name);
  }
}

options_c
extract_cli_parser_c::run() {
  init_parser();

  parse_args();

  m_options.merge_tracks_and_timestamps_targets();

  if (m_debug)
    m_options.dump();

  if (m_cli_type == cli_type_e::multiple)
    verify_options_multiple_mode();

  return m_options;
}
