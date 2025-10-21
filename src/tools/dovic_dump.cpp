/*
   dovic_dump - A tool for dumping the HEVCC element from the middle of a file

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/command_line.h"
#include "common/dovi_meta.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/strings/parsing.h"

static void
setup() {
  mtx::cli::g_usage_text =
    "dovic_dump [options] input_file_name position_in_file size\n"
    "\n"
    "General options:\n"
    "\n"
    "  -h, --help             This help text\n"
    "  -V, --version          Print version information\n";
}

static std::tuple<std::string, uint64_t, uint64_t>
parse_args(std::vector<std::string> &args) {
  std::string file_name;
  int64_t file_pos = 0;
  uint64_t size    = 0;

  for (auto const &arg: args) {
    if (file_name.empty())
      file_name = arg;

    else if (size != 0)
      mxerror("Superfluous arguments given.\n");

    else if (!file_pos) {
      if (!mtx::string::parse_number(arg, file_pos))
        mxerror("The file position is not a valid number.\n");

    } else if (!size) {
      if (!mtx::string::parse_number(arg, size))
        mxerror("The size is not a valid number.\n");
    }
  }

  if (file_name.empty())
    mxerror("No file name given\n");

  if (!file_pos)
    mxerror("No file position given\n");

  if (!size)
    mxerror("No size given\n");

  if (file_pos < 0)
    file_pos = file_pos * -1 - size;

  return std::make_tuple(file_name, static_cast<uint64_t>(file_pos), size);
}

static void
read_and_parse_dovic(std::string const &file_name,
                     uint64_t file_pos,
                     uint64_t size) {
  mm_file_io_c in{file_name};

  in.setFilePointer(file_pos);
  auto data = in.read(size);
  auto r    = mtx::bits::reader_c{data->get_buffer(), data->get_size()};

  try {
    auto dovi_cfg = mtx::dovi::parse_dovi_decoder_configuration_record(r);
    dovi_cfg.dump();

  } catch (...) {
    mxerror("Parsing the DOVI decoder configuration record failed\n");
  }
}

int
main(int argc,
     char **argv) {
  mtx_common_init("dovic_dump", argv[0]);

  setup();

  auto args = mtx::cli::args_in_utf8(argc, argv);
  while (mtx::cli::handle_common_args(args, "-r"))
    ;

  auto file_spec = parse_args(args);

  try {
    read_and_parse_dovic(std::get<0>(file_spec), std::get<1>(file_spec), std::get<2>(file_spec));
  } catch (mtx::mm_io::open_x &) {
    mxerror("File not found\n");
  }

  mxexit();
}
