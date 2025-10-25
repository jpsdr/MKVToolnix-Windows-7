/** \brief command line parsing

   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/bcp47.h"
#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "propedit/globals.h"
#include "propedit/propedit_cli_parser.h"

propedit_cli_parser_c::propedit_cli_parser_c(const std::vector<std::string> &args)
  : mtx::cli::parser_c{args}
  , m_options(options_cptr(new options_c))
  , m_target(m_options->add_track_or_segmentinfo_target("segment_info"))
{
}

void
propedit_cli_parser_c::set_parse_mode() {
  try {
    m_options->set_parse_mode(m_next_arg);
  } catch (...) {
    mxerror(fmt::format(FY("Unknown parse mode in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::add_target() {
  try {
    m_target = m_options->add_track_or_segmentinfo_target(m_next_arg);
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::add_change() {
  try {
    change_c::change_type_e type = (m_current_arg == "-a") || (m_current_arg == "--add") ? change_c::ct_add
                                 : (m_current_arg == "-s") || (m_current_arg == "--set") ? change_c::ct_set
                                 :                                                         change_c::ct_delete;
    m_target->add_change(type, m_next_arg);
  } catch (std::runtime_error &error) {
    mxerror(fmt::format(FY("Invalid change spec ({2}) in '{0} {1}'.\n"), m_current_arg, m_next_arg, error.what()));
  }
}

void
propedit_cli_parser_c::add_tags() {
  try {
    m_options->add_tags(m_next_arg);
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::set_chapter_charset() {
  m_options->m_chapter_charset = m_next_arg;
}

void
propedit_cli_parser_c::add_chapters() {
  try {
    m_options->add_chapters(m_next_arg);
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::set_attachment_name() {
  m_attachment.m_name = m_next_arg;
}

void
propedit_cli_parser_c::set_attachment_description() {
  m_attachment.m_description = m_next_arg;
}

void
propedit_cli_parser_c::set_attachment_mime_type() {
  m_attachment.m_mime_type = m_next_arg;
}

void
propedit_cli_parser_c::set_attachment_uid() {
  auto uid = uint64_t{};
  if (!mtx::string::parse_number(m_next_arg, uid))
    mxerror(fmt::format(FY("The value '{0}' is not a number.\n"), m_next_arg));

  m_attachment.m_uid = uid;
}

void
propedit_cli_parser_c::add_attachment() {
  try {
    m_options->add_attachment_command(attachment_target_c::ac_add, m_next_arg, m_attachment);
    m_attachment = attachment_target_c::options_t();
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::delete_attachment() {
  try {
    m_options->add_attachment_command(attachment_target_c::ac_delete, m_next_arg, m_attachment);
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::replace_attachment() {
  try {
    m_options->add_attachment_command(m_current_arg == "--update-attachment" ? attachment_target_c::ac_update : attachment_target_c::ac_replace, m_next_arg, m_attachment);
    m_attachment = attachment_target_c::options_t();
  } catch (...) {
    mxerror(fmt::format(FY("Invalid selector in '{0} {1}'.\n"), m_current_arg, m_next_arg));
  }
}

void
propedit_cli_parser_c::handle_track_statistics_tags() {
  m_options->add_delete_track_statistics_tags(m_current_arg == "--add-track-statistics-tags" ? tag_target_c::tom_add_track_statistics : tag_target_c::tom_delete_track_statistics);
}

std::map<property_element_c::ebml_type_e, const char *> &
propedit_cli_parser_c::get_ebml_type_abbrev_map() {
  static std::map<property_element_c::ebml_type_e, const char *> s_ebml_type_abbrevs;
  if (s_ebml_type_abbrevs.empty()) {
    s_ebml_type_abbrevs[property_element_c::EBMLT_INT]     = "SI";
    s_ebml_type_abbrevs[property_element_c::EBMLT_UINT]    = "UI";
    s_ebml_type_abbrevs[property_element_c::EBMLT_BOOL]    = "B";
    s_ebml_type_abbrevs[property_element_c::EBMLT_STRING]  = "S";
    s_ebml_type_abbrevs[property_element_c::EBMLT_USTRING] = "US";
    s_ebml_type_abbrevs[property_element_c::EBMLT_BINARY]  = "X";
    s_ebml_type_abbrevs[property_element_c::EBMLT_FLOAT]   = "FP";
    s_ebml_type_abbrevs[property_element_c::EBMLT_DATE]    = "DT";
  }

  return s_ebml_type_abbrevs;
}

void
propedit_cli_parser_c::list_property_names() {
  mxinfo(Y("All known property names and their meaning\n"));

  list_property_names_for_table(property_element_c::get_table_for(EBML_INFO(libmatroska::KaxInfo),   nullptr, true), Y("Segment information"), "info");
  list_property_names_for_table(property_element_c::get_table_for(EBML_INFO(libmatroska::KaxTracks), nullptr, true), Y("Track headers"),       "track:...");

  mxinfo("\n");
  mxinfo(Y("Element types:\n"));
  mxinfo(Y("  SI: signed integer\n"));
  mxinfo(Y("  UI: unsigned integer\n"));
  mxinfo(Y("  B:  boolean (0 or 1)\n"));
  mxinfo(Y("  S:  string\n"));
  mxinfo(Y("  US: Unicode string\n"));
  mxinfo(Y("  X:  binary in hex\n"));
  mxinfo(Y("  FP: floating point number\n"));
  mxinfo(Y("  DT: date & time\n"));

  mxexit();
}

void
propedit_cli_parser_c::list_property_names_for_table(const std::vector<property_element_c> &table,
                                                     const std::string &title,
                                                     const std::string &edit_spec) {
  auto &ebml_type_map = get_ebml_type_abbrev_map();

  auto max_name_len = std::accumulate(table.begin(), table.end(), 0u, [](size_t a, const property_element_c &e) { return std::max(a, e.m_name.length()); });

  static QRegularExpression s_newline_re{"[ \\t]*\\n[ \\t]*"};
  std::string indent_string = std::string(max_name_len, ' ') + " |    | ";

  mxinfo("\n");
  mxinfo(fmt::format(FY("Elements in the category '{0}' ('--edit {1}'):\n"), title, edit_spec));

  for (auto &property : table) {
    if (property.m_title.get_untranslated().empty())
      continue;

    auto name        = fmt::format("{0:<{1}} | {2:<2} |", property.m_name, max_name_len, ebml_type_map[property.m_type]);
    auto description = fmt::format("{0}: {1}", property.m_title.get_translated(), to_utf8(Q(property.m_description.get_translated()).replace(s_newline_re, " ")));
    mxinfo(mtx::string::format_paragraph(description, max_name_len + 8, name, indent_string));
  }
}

void
propedit_cli_parser_c::set_file_name() {
  m_options->set_file_name(m_current_arg);
}

void
propedit_cli_parser_c::disable_language_ietf() {
  mtx::bcp47::language_c::disable();
}

void
propedit_cli_parser_c::set_language_ietf_normalization_mode() {
  if (m_next_arg.empty())
    mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), "--normalize-language-ietf"));

  if (!mtx::included_in(m_next_arg, "canonical"s, "extlang"s, "no"s, "off"s, "none"s))
    mxerror(fmt::format(FY("'{0}' is not a valid language normalization mode.\n"), m_next_arg));

  auto mode = m_next_arg == "canonical"s ? mtx::bcp47::normalization_mode_e::canonical
            : m_next_arg == "extlang"s   ? mtx::bcp47::normalization_mode_e::extlang
            :                              mtx::bcp47::normalization_mode_e::none;

  mtx::bcp47::language_c::set_normalization_mode(mode);
}

void
propedit_cli_parser_c::enable_legacy_font_mime_types() {
  g_use_legacy_font_mime_types = true;
}

void
propedit_cli_parser_c::init_parser() {
  add_information(YT("mkvpropedit [options] <file> <actions>"));

  add_section_header(YT("Options"));
  add_option("l|list-property-names",         std::bind(&propedit_cli_parser_c::list_property_names,           this), YT("List all valid property names and exit"));
  add_option("p|parse-mode=<mode>",           std::bind(&propedit_cli_parser_c::set_parse_mode,                this), YT("Sets the Matroska parser mode to 'fast' (default) or 'full'"));
  add_option("enable-legacy-font-mime-types", std::bind(&propedit_cli_parser_c::enable_legacy_font_mime_types, this), YT("Use legacy font MIME types when adding new attachments or replacing existing ones"));

  add_section_header(YT("Actions for handling properties"));
  add_option("e|edit=<selector>",  std::bind(&propedit_cli_parser_c::add_target, this), YT("Sets the Matroska file section that all following add/set/delete actions operate on (see below and man page for syntax)"));
  add_option("a|add=<name=value>", std::bind(&propedit_cli_parser_c::add_change, this), YT("Adds a property with the value even if such a property already exists"));
  add_option("s|set=<name=value>", std::bind(&propedit_cli_parser_c::add_change, this), YT("Sets a property to the value if it exists and add it otherwise"));
  add_option("d|delete=<name>",    std::bind(&propedit_cli_parser_c::add_change, this), YT("Delete all occurrences of a property"));

  add_section_header(YT("Actions for handling tags and chapters"));
  add_option("t|tags=<selector:filename>",   std::bind(&propedit_cli_parser_c::add_tags,                     this), YT("Add or replace tags in the file with the ones from 'filename' or remove them if 'filename' is empty (see below and man page for syntax)"));
  add_option("c|chapters=<filename>",        std::bind(&propedit_cli_parser_c::add_chapters,                 this), YT("Add or replace chapters in the file with the ones from 'filename' or remove them if 'filename' is empty"));
  add_option("chapter-charset=<charset>",    std::bind(&propedit_cli_parser_c::set_chapter_charset,          this), YT("Set the charset to use when reading chapter files using the simple chapter format"));
  add_option("add-track-statistics-tags",    std::bind(&propedit_cli_parser_c::handle_track_statistics_tags, this), YT("Calculate statistics for all tracks and add new/update existing tags for them"));
  add_option("delete-track-statistics-tags", std::bind(&propedit_cli_parser_c::handle_track_statistics_tags, this), YT("Delete all existing track statistics tags"));

  add_section_header(YT("Actions for handling attachments"));
  add_option("add-attachment=<filename>",                         std::bind(&propedit_cli_parser_c::add_attachment,             this), YT("Add the file 'filename' as a new attachment"));
  add_option("replace-attachment=<attachment-selector:filename>", std::bind(&propedit_cli_parser_c::replace_attachment,         this), YT("Replace an attachment with the file 'filename'"));
  add_option("update-attachment=<attachment-selector>",           std::bind(&propedit_cli_parser_c::replace_attachment,         this), YT("Update an attachment's properties"));
  add_option("delete-attachment=<attachment-selector>",           std::bind(&propedit_cli_parser_c::delete_attachment,          this), YT("Delete one or more attachments"));
  add_option("attachment-name=<name>",                            std::bind(&propedit_cli_parser_c::set_attachment_name,        this), YT("Set the name to use for the following '--add-attachment', '--replace-attachment' or '--update-attachment' option"));
  add_option("attachment-description=<description>",              std::bind(&propedit_cli_parser_c::set_attachment_description, this), YT("Set the description to use for the following '--add-attachment', '--replace-attachment' or '--update-attachment' option"));
  add_option("attachment-mime-type=<mime-type>",                  std::bind(&propedit_cli_parser_c::set_attachment_mime_type,   this), YT("Set the MIME type to use for the following '--add-attachment', '--replace-attachment' or '--update-attachment' option"));
  add_option("attachment-uid=<uid>",                              std::bind(&propedit_cli_parser_c::set_attachment_uid,         this), YT("Set the UID to use for the following '--add-attachment', '--replace-attachment' or '--update-attachment' option"));

  add_section_header(YT("Other options"));
  add_option("disable-language-ietf",          std::bind(&propedit_cli_parser_c::disable_language_ietf,                this), YT("Do not change LanguageIETF track header elements when the 'language' property is changed."));
  add_option("normalize-language-ietf=<mode>", std::bind(&propedit_cli_parser_c::set_language_ietf_normalization_mode, this), YT("Normalize all IETF BCP 47 language tags of changed elements to either their canonical form (mode 'canonical'), their extended language subtags form (mode 'extlang') or not at all (mode 'off')"));
  add_common_options();

  add_separator();
  add_information(YT("The order of the various options is not important."));

  add_section_header(YT("Edit selectors for properties"), 0);
  add_section_header(YT("Segment information"), 1);
  add_information(YT("The strings 'info', 'segment_info' or 'segmentinfo' select the segment information element. This is also the default until the first '--edit' option is found."), 2);

  add_section_header(YT("Track headers"), 1);
  add_information(YT("The string 'track:n' with 'n' being a number selects the nth track. Numbering starts at 1."), 2);
  add_information(YT("The string 'track:' followed by one of the chars 'a', 'b', 's' or 'v' followed by a number 'n' selects the nth audio, button, subtitle or video track "
                     "(e.g. '--edit track:a2'). Numbering starts at 1."), 2);
  add_information(YT("The string 'track:=uid' with 'uid' being a number selects the track whose 'track UID' element equals 'uid'."), 2);
  add_information(YT("The string 'track:@number' with 'number' being a number selects the track whose 'track number' element equals 'number'."), 2);

  add_section_header(YT("Tag selectors"), 0);
  add_information(YT("The string 'all' works on all tags."), 1);
  add_information(YT("The string 'global' works on the global tags."), 1);
  add_information(YT("All other strings work just like the track header selectors (see above)."), 1);

  add_section_header(YT("Attachment selectors"), 0);
  add_information(YT("An <attachment-selector> can have three forms:"), 1);
  add_information(YT("1. A number which will be interpreted as an attachment ID as listed by 'mkvmerge --identify'. These are usually simply numbered starting from 0 (e.g. '2')."), 2);
  add_information(YT("2. A number with the prefix '=' which will be interpreted as the attachment's unique ID (UID) as listed by 'mkvmerge --identify'. These are usually random-looking numbers (e.g. '128975986723')."), 2);
  add_information(YT("3. Either 'name:<value>' or 'mime-type:<value>' in which case the selector applies to all attachments whose name or MIME type respectively equals <value>."), 2);

  add_hook(mtx::cli::parser_c::ht_unknown_option, std::bind(&propedit_cli_parser_c::set_file_name, this));

  set_to_parse_first("--normalize-language-ietf");
}

void
propedit_cli_parser_c::validate() {
  if (m_attachment.m_name || m_attachment.m_description || m_attachment.m_mime_type || m_attachment.m_uid)
    mxerror(Y("One of the options '--attachment-name', '--attachment-description', '--attachment-mime-type' or '--attachment-uid' has been used "
              "without a following '--add-attachment', '--replace-attachment' or '--update-attachment' option.\n"));
}

options_cptr
propedit_cli_parser_c::run() {
  init_parser();

  parse_args();
  validate();

  m_options->options_parsed();
  m_options->validate();

  return m_options;
}
