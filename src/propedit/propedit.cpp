/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxChapters.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTracks.h>

#include "common/command_line.h"
#include "common/doc_type_version_handler.h"
#include "common/list_utils.h"
#include "common/mm_io_x.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "propedit/propedit_cli_parser.h"

using namespace libmatroska;

std::unique_ptr<mtx::doc_type_version_handler_c> g_doc_type_version_handler;
std::unordered_map<uint64_t, uint64_t> g_track_uid_changes;

static void
display_update_element_result(const EbmlCallbacks &callbacks,
                              kax_analyzer_c::update_element_result_e result) {
  std::string message(fmt::format(Y("Updating the '{0}' element failed. Reason:"), callbacks.DebugName));
  message += " ";

  switch (result) {
    case kax_analyzer_c::uer_error_segment_size_for_element:
      message += Y("The element was written at the end of the file, but the segment size could not be updated. Therefore the element will not be visible. The process will be aborted. The file has been changed!");
      break;

    case kax_analyzer_c::uer_error_segment_size_for_meta_seek:
      message += Y("The meta seek element was written at the end of the file, but the segment size could not be updated. Therefore the element will not be visible. The process will be aborted. The file has been changed!");
      break;

    case kax_analyzer_c::uer_error_meta_seek:
      message += Y("The Matroska file was modified, but the meta seek entry could not be updated. This means that players might have a hard time finding the element. Please use your favorite player to check this file.");
      break;

    case kax_analyzer_c::uer_error_opening_for_reading:
      message += fmt::format("{0} {1}",
                             Y("The file could not be opened for reading."),
                             Y("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
      break;

    case kax_analyzer_c::uer_error_opening_for_writing:
      message += fmt::format("{0} {1}",
                             Y("The file could not be opened for writing."),
                             Y("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
      break;

    case kax_analyzer_c::uer_error_fixing_last_element_unknown_size_failed:
      message += fmt::format("{0} {1} {2} {3} {4}",
                             Y("The Matroska file's last element is set to an unknown size."),
                             Y("Due to the particular structure of the file this situation cannot be fixed automatically."),
                             Y("The file can be fixed by multiplexing it with mkvmerge again."),
                             Y("The process will be aborted."),
                             Y("The file has not been modified."));
      break;

    default:
      message += Y("An unknown error occurred. The file has been modified.");
  }

  mxerror(message + "\n");
}

bool
has_content_been_modified(options_cptr const &options) {
  return mtx::any(options->m_targets, [](target_cptr const &t) { return t->has_content_been_modified(); });
}

static void
write_changes(options_cptr &options,
              kax_analyzer_c *analyzer) {
  std::vector<EbmlId> ids_to_write;
  ids_to_write.push_back(KaxInfo::ClassInfos.GlobalId);
  ids_to_write.push_back(KaxTracks::ClassInfos.GlobalId);
  ids_to_write.push_back(KaxTags::ClassInfos.GlobalId);
  ids_to_write.push_back(KaxChapters::ClassInfos.GlobalId);
  ids_to_write.push_back(KaxAttachments::ClassInfos.GlobalId);

  for (auto &id_to_write : ids_to_write) {
    for (auto &target : options->m_targets) {
      if (!target->get_level1_element())
        continue;

      EbmlMaster &l1_element = *target->get_level1_element();

      if (id_to_write != l1_element.Generic().GlobalId)
        continue;

      auto result = l1_element.ListSize() ? analyzer->update_element(&l1_element, target->write_elements_set_to_default_value(), target->add_mandatory_elements_if_missing())
                  :                         analyzer->remove_elements(EbmlId(l1_element));
      if (kax_analyzer_c::uer_success != result)
        display_update_element_result(l1_element.Generic(), result);

      break;
    }
  }
}

static void
update_ebml_head(mm_io_c &file) {
  auto result = g_doc_type_version_handler->update_ebml_head(file);
  if (mtx::included_in(result, mtx::doc_type_version_handler_c::update_result_e::ok_updated, mtx::doc_type_version_handler_c::update_result_e::ok_no_update_needed))
    return;

  auto details = mtx::doc_type_version_handler_c::update_result_e::err_no_head_found    == result ? Y("No 'EBML head' element was found.")
               : mtx::doc_type_version_handler_c::update_result_e::err_not_enough_space == result ? Y("There's not enough space at the beginning of the file to fit the updated 'EBML head' element in.")
               :                                                                                    Y("A generic read or write failure occurred.");

  mxwarn(fmt::format("{0} {1}\n", Y("Updating the 'document type version' or 'document type read version' header fields failed."), details));
}

static void
run(options_cptr &options) {
  g_doc_type_version_handler.reset(new mtx::doc_type_version_handler_c);

  console_kax_analyzer_cptr analyzer;

  try {
    if (!kax_analyzer_c::probe(options->m_file_name))
      mxerror(fmt::format(Y("The file '{0}' is not a Matroska file or it could not be found.\n"), options->m_file_name));

    analyzer = console_kax_analyzer_cptr(new console_kax_analyzer_c(options->m_file_name));
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(Y("The file '{0}' could not be opened for reading and writing: {1}.\n"), options->m_file_name, ex));
  }

  mxinfo(fmt::format("{0}\n", Y("The file is being analyzed.")));

  analyzer->set_show_progress(options->m_show_progress);

  bool ok = false;
  try {
    ok = analyzer
      ->set_parse_mode(options->m_parse_mode)
      .set_open_mode(MODE_READ)
      .set_throw_on_error(true)
      .set_doc_type_version_handler(g_doc_type_version_handler.get())
      .process();
  } catch (mtx::exception &ex) {
    mxerror(fmt::format(Y("The file '{0}' could not be opened for reading and writing, or a read/write operation on it failed: {1}.\n"), options->m_file_name, ex));
  } catch (...) {
  }

  if (!ok)
    mxerror(Y("This file could not be opened or parsed.\n"));

  options->find_elements(analyzer.get());
  options->validate();

  if (debugging_c::requested("dump_options")) {
    mxinfo("\nDumping options after file and element analysis\n\n");
    options->dump_info();
  }

  options->execute(*analyzer);

  if (has_content_been_modified(options)) {
    mxinfo(Y("The changes are written to the file.\n"));

    try {
      write_changes(options, analyzer.get());

      auto result = analyzer->update_uid_referrals(g_track_uid_changes);
      if (kax_analyzer_c::uer_success != result)
        display_update_element_result(KaxTracks::ClassInfos, result);

      update_ebml_head(analyzer->get_file());
    } catch (mtx::exception &ex) {
      mxerror(fmt::format(Y("The file '{0}' could not be opened for reading and writing, or a read/write operation on it failed: {1}.\n"), options->m_file_name, ex));
    } catch (...) {
    }

    mxinfo(Y("Done.\n"));

  } else
    mxinfo(Y("No changes were made.\n"));

  mxexit();
}

static
void setup(char **argv) {
  mtx_common_init("mkvpropedit", argv[0]);
  clear_list_of_unique_numbers(UNIQUE_ALL_IDS);
}

/** \brief Setup and high level program control

   Calls the functions for setup, handling the command line arguments,
   the actual processing and cleaning up.
*/
int
main(int argc,
     char **argv) {
  setup(argv);

  options_cptr options = propedit_cli_parser_c(mtx::cli::args_in_utf8(argc, argv)).run();

  if (debugging_c::requested("dump_options")) {
    mxinfo("\nDumping options after parsing the command line\n\n");
    options->dump_info();
  }

  run(options);

  mxexit();
}
