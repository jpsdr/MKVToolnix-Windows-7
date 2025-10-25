/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper routines for various compression libs

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/content_decoder.h"
#include "common/ebml.h"
#include "common/list_utils.h"
#include "common/strings/formatting.h"

kax_content_encoding_t::kax_content_encoding_t()
  : order{}
  , type{}
  , scope(CONTENT_ENCODING_SCOPE_BLOCK)
  , comp_algo{}
{
}

content_decoder_c::content_decoder_c()
  : ok{true}
{
}

content_decoder_c::content_decoder_c(libmatroska::KaxTrackEntry &ktentry)
  : ok{true}
{
  initialize(ktentry);
}

bool
content_decoder_c::initialize(libmatroska::KaxTrackEntry &ktentry) {
  encodings.clear();

  auto kcencodings = find_child<libmatroska::KaxContentEncodings>(&ktentry);
  if (!kcencodings)
    return true;

  int tid = kt_get_number(ktentry);

  for (auto kcenc_el : *kcencodings) {
    auto kcenc = dynamic_cast<libmatroska::KaxContentEncoding *>(kcenc_el);
    if (!kcenc)
      continue;

    kax_content_encoding_t enc;

    enc.order    = find_child_value<libmatroska::KaxContentEncodingOrder>(kcenc);
    enc.type     = find_child_value<libmatroska::KaxContentEncodingType >(kcenc);
    enc.scope    = find_child_value<libmatroska::KaxContentEncodingScope>(kcenc, 1u);

    auto ce_comp = find_child<libmatroska::KaxContentCompression>(kcenc);
    if (ce_comp) {
      enc.comp_algo     = find_child_value<libmatroska::KaxContentCompAlgo    >(ce_comp);
      enc.comp_settings = find_child_value<libmatroska::KaxContentCompSettings>(ce_comp);
    }

    if (1 == enc.type) {
      mxwarn(fmt::format(FY("Track number {0} has been encrypted and decryption has not yet been implemented.\n"), tid));
      ok = false;
      break;
    }

    if (0 != enc.type) {
      mxerror(fmt::format(FY("Unknown content encoding type {0} for track {1}.\n"), enc.type, tid));
      ok = false;
      break;
    }

    if (0 == enc.comp_algo) {
      enc.compressor = std::shared_ptr<compressor_c>(new zlib_compressor_c());
      encodings.push_back(enc);

    } else if (mtx::included_in(enc.comp_algo, 1u, 2u)) {
      auto algorithm = 1u == enc.comp_algo ? "bzlib" : "lzo1x";
      mxwarn(fmt::format(FY("Track {0} was compressed with the algorithm '{1}' which is not supported anymore.\n"), tid, algorithm));
      ok = false;
      break;

    } else if (3 == enc.comp_algo) {
      if (enc.comp_settings && enc.comp_settings->get_size()) {
        enc.compressor = std::shared_ptr<compressor_c>(new header_removal_compressor_c);
        std::static_pointer_cast<header_removal_compressor_c>(enc.compressor)->set_bytes(enc.comp_settings);

        encodings.push_back(enc);
      }

    } else {
      mxwarn(fmt::format(FY("Track {0} has been compressed with an unknown/unsupported compression algorithm ({1}).\n"), tid, enc.comp_algo));
      ok = false;
      break;
    }
  }

  std::stable_sort(encodings.begin(), encodings.end(), [](kax_content_encoding_t const &a, kax_content_encoding_t const &b) { return a.order < b.order; });

  return ok;
}

void
content_decoder_c::reverse(memory_cptr &memory,
                           content_encoding_scope_e scope) {
  if (!is_ok() || encodings.empty())
    return;

  for (auto &ce : encodings)
    if (0 != (ce.scope & scope))
      memory = ce.compressor->decompress(memory);
}

std::string
content_decoder_c::descriptive_algorithm_list() {
  std::string list;
  std::vector<std::string> algorithms;

  for (auto const &enc : encodings)
    algorithms.push_back(fmt::to_string(enc.comp_algo));

  return mtx::string::join(algorithms, ",");
}
