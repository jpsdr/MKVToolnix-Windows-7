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

#include <matroska/KaxTracks.h>

#include "common/compression.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/strings/formatting.h"

static const char *compression_methods[] = {
  "unspecified", "zlib", "header_removal", "mpeg4_p2", "mpeg4_p10", "dirac", "dts", "ac3", "mp3", "analyze_header_removal", "none"
};

static const int compression_method_map[] = {
  0,                            // unspecified
  0,                            // zlib
  3,                            // header removal
  3,                            // mpeg4_p2 is header removal
  3,                            // mpeg4_p10 is header removal
  3,                            // dirac is header removal
  3,                            // dts is header removal
  3,                            // ac3 is header removal
  3,                            // mp3 is header removal
  999999999,                    // analyze_header_removal
  0                             // none
};

compressor_c::~compressor_c() {
  if (0 == items)
    return;

  mxdebug_if(m_debug,
             fmt::format("compression: Overall stats: raw size: {0}, compressed size: {1}, items: {2}, ratio: {3:.2f}%, avg bytes per item: {4}\n",
                         raw_size, compressed_size, items, compressed_size * 100.0 / raw_size, compressed_size / items));
}

void
compressor_c::set_track_headers(libmatroska::KaxContentEncoding &c_encoding) {
  // Set compression method.
  get_child<libmatroska::KaxContentCompAlgo>(get_child<libmatroska::KaxContentCompression>(c_encoding)).SetValue(compression_method_map[method]);
}

compressor_ptr
compressor_c::create(compression_method_e method) {
  if ((COMPRESSION_UNSPECIFIED >= method) || (COMPRESSION_NUM < method))
    return compressor_ptr();

  return create(compression_methods[method]);
}

compressor_ptr
compressor_c::create(const char *method) {
  if (!strcasecmp(method, compression_methods[COMPRESSION_ZLIB]))
    return compressor_ptr(new zlib_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_MPEG4_P2]))
    return compressor_ptr(new mpeg4_p2_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_MPEG4_P10]))
    return compressor_ptr(new mpeg4_p10_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_DIRAC]))
    return compressor_ptr(new dirac_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_DTS]))
    return compressor_ptr(new dts_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_AC3]))
    return compressor_ptr(new ac3_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_MP3]))
    return compressor_ptr(new mp3_compressor_c());

  if (!strcasecmp(method, compression_methods[COMPRESSION_ANALYZE_HEADER_REMOVAL]))
    return compressor_ptr(new analyze_header_removal_compressor_c());

  if (!strcasecmp(method, "none"))
    return std::make_shared<compressor_c>(COMPRESSION_NONE);

  return compressor_ptr();
}

compressor_ptr
compressor_c::create_from_file_name(std::string const &file_name) {
  auto pos = file_name.rfind(".");
  auto ext = balg::to_lower_copy(pos == std::string::npos ? file_name : file_name.substr(pos + 1));

  if (ext == "gz")
    return compressor_ptr(new zlib_compressor_c());

  return std::make_shared<compressor_c>(COMPRESSION_NONE);
}
