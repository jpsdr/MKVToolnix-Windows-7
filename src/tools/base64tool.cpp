/*
   base64util - Utility for encoding and decoding Base64 files.

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   command line parameter parsing, looping, output handling

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <string>

#include "common/base64.h"
#include "common/command_line.h"
#include "common/common_pch.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"
#include "common/version.h"

void
set_usage() {
  mtx::cli::g_usage_text = Y("base64util <encode|decode> <input> <output> [maxlen]\n"
                             "\n"
                             "  encode - Read from <input>, encode to Base64 and write to <output>.\n"
                             "           Max line length can be specified and is 72 if left out.\n"
                             "  decode - Read from <input>, decode to uint8_t and write to <output>.\n");
}

int
main(int argc,
     char *argv[]) {
  int maxlen;
  uint64_t size;
  char mode;
  std::string s, line;

  mtx_common_init("base64tool", argv[0]);

  set_usage();

  if (argc < 4)
    mtx::cli::display_usage(0);

  mode = 0;
  if (!strcmp(argv[1], "encode"))
    mode = 'e';
  else if (!strcmp(argv[1], "decode"))
    mode = 'd';
  else
    mxerror(fmt::format(FY("Invalid mode '{0}'.\n"), argv[1]));

  maxlen = 72;
  if ((argc == 5) && (mode == 'e')) {
    if (!mtx::string::parse_number(argv[4], maxlen) || (maxlen < 4))
      mxerror(Y("Max line length must be >= 4.\n\n"));
  } else if ((argc > 5) || ((argc > 4) && (mode == 'd')))
    mtx::cli::display_usage(2);

  maxlen = ((maxlen + 3) / 4) * 4;

  mm_io_cptr in, intext;
  try {
    in = mm_io_cptr(new mm_file_io_c(argv[2]));
    if (mode != 'e')
      intext = std::make_shared<mm_text_io_c>(in);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for reading: {1}.\n"), argv[2], ex));
  }

  mm_io_cptr out;
  try {
    out = mm_write_buffer_io_c::open(argv[3], 128 * 1024);
  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), argv[3], ex));
  }

  in->save_pos();
  in->setFilePointer(0, libebml::seek_end);
  size = in->getFilePointer();
  in->restore_pos();

  if (mode == 'e') {
    auto af_buffer = memory_c::alloc(size);
    auto buffer    = af_buffer->get_buffer();
    size = in->read(buffer, size);

    s = mtx::base64::encode(buffer, size, true, maxlen);

    out->write(s.c_str(), s.length());

  } else {

    while (intext->getline2(line)) {
      mtx::string::strip(line);
      s += line;
    }

    try {
      auto decoded = mtx::base64::decode(s);
      out->write(decoded);
    } catch(...) {
      mxerror(Y("The Base64 encoded data could not be decoded.\n"));
    }
  }

  mxinfo(Y("Done.\n"));

  mxexit();
}
