/*
   mkvtoolnix -- wrapper utility for all the other programs in the MKVToolNix package

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)
# include <windows.h>
#endif

#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QTemporaryFile>

#include "common/command_line.h"
#include "common/fs_sys_helpers.h"
#include "common/list_utils.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/unique_numbers.h"

namespace {

void
setup(char **argv) {
  mtx_common_init("mkvtoolnix", argv[0]);
  clear_list_of_unique_numbers(UNIQUE_ALL_IDS);
}

std::string
determine_program(std::vector<std::string> &args) {
  auto program = "mkvtoolnix-gui"s;

  if (args.empty())
    return program;

  QRegularExpression main_name_re{"^(?:mkv)?(merge|info|extract|propedit|toolnix-gui)(?:\\.exe)?$"};
  QRegularExpression tool_name_re{"^(bluray_dump|ebml_validator|hevcc_dump|xyzvc_dump)(?:\\.exe)?$"};

  auto matches = main_name_re.match(Q(args[0]));

  if (matches.hasMatch()) {
    program = "mkv"s + to_utf8(matches.captured(1));
    args.erase(args.begin(), args.begin() + 1);

    return program;
  }

  matches = main_name_re.match(Q(args[0]));

  if (!matches.hasMatch())
    return program;

  program = to_utf8(matches.captured(1));
  args.erase(args.begin(), args.begin() + 1);

  return program;
}

std::unique_ptr<QTemporaryFile>
create_option_file(std::vector<std::string> const &options) {
  auto file = std::make_unique<QTemporaryFile>(QDir::temp().filePath(Q("mkvtoolnix-XXXXXX.json")));

  if (!file->open())
    mxerror(to_utf8(QY("Error creating a temporary file (reason: %1).").arg(file->errorString())));

  auto serialized = mtx::json::dump(nlohmann::json(options));

  file->write(serialized.c_str());
  file->close();

  return file;
}

std::string
find_executable(std::string const &argv0,
                std::string program) {
#if defined(SYS_WINDOWS)
  program += ".exe"s;
#endif

  auto base_path    = mtx::sys::get_current_exe_path(argv0);
  auto program_path = mtx::fs::to_path(program);

  std::vector<boost::filesystem::path> potential_paths{
    base_path,
    base_path / "tools",
    base_path / "..",
    base_path / ".." / "mkvtoolnix-gui",
    base_path / ".." / "tools",
  };

  for (auto const & potential_path: potential_paths) {
    auto exe_path = potential_path / program_path;
    if (boost::filesystem::is_regular_file(exe_path))
      return exe_path.string();
  }

  mxerror(fmt::format("{0}: {1}\n", Y("Executable not found"), program));

  return {};
}

void
maybe_alloc_console([[maybe_unused]] std::string const &program) {
#if defined(SYS_WINDOWS)
  if (mtx::included_in(program, "mkvtoolnix-gui"s, "mkvtoolnix-gui.exe"s))
    return;

  if (AttachConsole(ATTACH_PARENT_PROCESS))
    return;

  AllocConsole();
#endif
}

QStringList
to_qstringlist(std::vector<std::string> const &args) {
  QStringList q_args;

  for (auto const &arg : args)
    q_args << Q(arg);

  return q_args;
}

} // anonymous namespace

int
main(int argc,
     char **argv) {
  if (argc == 0)
    return 3;

  setup(argv);

  auto args        = mtx::cli::args_in_utf8(argc, argv);
  auto program     = determine_program(args);
  auto executable  = find_executable(argv[0], program);
  auto option_file = create_option_file(args);

  maybe_alloc_console(program);

  auto exit_code = QProcess::execute(Q(executable), to_qstringlist(args));

  mxexit(exit_code);
}
