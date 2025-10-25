/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper routines for various compression libs

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/compression.h"

enum content_encoding_scope_e {
  CONTENT_ENCODING_SCOPE_BLOCK = 1,
  CONTENT_ENCODING_SCOPE_CODECPRIVATE = 2
};

struct kax_content_encoding_t {
  uint32_t order, type, scope;
  uint32_t comp_algo;
  memory_cptr comp_settings;

  std::shared_ptr<compressor_c> compressor;

  kax_content_encoding_t();
};

class content_decoder_c {
protected:
  std::vector<kax_content_encoding_t> encodings;
  bool ok;

public:
  content_decoder_c();
  content_decoder_c(libmatroska::KaxTrackEntry &ktentry);

  bool initialize(libmatroska::KaxTrackEntry &ktentry);
  void reverse(memory_cptr &data, content_encoding_scope_e scope);
  bool is_ok() {
    return ok;
  }
  bool has_encodings() {
    return !encodings.empty();
  }
  std::string descriptive_algorithm_list();
};
