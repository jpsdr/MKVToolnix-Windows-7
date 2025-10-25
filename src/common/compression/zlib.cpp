/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   zlib compressor

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/compression/zlib.h"

zlib_compressor_c::zlib_compressor_c()
  : compressor_c(COMPRESSION_ZLIB)
{
}

zlib_compressor_c::~zlib_compressor_c() {
}

memory_cptr
zlib_compressor_c::do_decompress(uint8_t const *buffer,
                                 std::size_t size) {
  z_stream d_stream;

  d_stream.zalloc = (alloc_func)0;
  d_stream.zfree  = (free_func)0;
  d_stream.opaque = (voidpf)0;
  int result      = inflateInit2(&d_stream, 15 + 32); // 15: window size; 32: look for zlib/gzip headers automatically

  if (Z_OK != result)
    mxerror(fmt::format(FY("inflateInit() failed. Result: {0}\n"), result));

  d_stream.next_in   = const_cast<Bytef *>(buffer);
  d_stream.avail_in  = size;
  int n              = 0;
  memory_cptr dst    = memory_c::alloc(0);

  do {
    n++;
    dst->resize(n * 4000);
    d_stream.next_out  = reinterpret_cast<Bytef *>(dst->get_buffer() + (n - 1) * 4000);
    d_stream.avail_out = 4000;
    result             = inflate(&d_stream, Z_NO_FLUSH);

    if ((Z_OK != result) && (Z_STREAM_END != result))
      throw mtx::compression_x(fmt::format(FY("Zlib decompression failed. Result: {0}\n"), result));

  } while ((0 == d_stream.avail_out) && (0 != d_stream.avail_in) && (Z_STREAM_END != result));

  dst->resize(d_stream.total_out);
  inflateEnd(&d_stream);

  mxdebug_if(m_debug, fmt::format("zlib_compressor_c: Decompression from {0} to {1}, {2}%\n", size, dst->get_size(), dst->get_size() * 100 / size));

  return dst;
}

memory_cptr
zlib_compressor_c::do_compress(uint8_t const *buffer,
                               std::size_t size) {
  z_stream c_stream;

  c_stream.zalloc = (alloc_func)0;
  c_stream.zfree  = (free_func)0;
  c_stream.opaque = (voidpf)0;
  int result      = deflateInit(&c_stream, 9);

  if (Z_OK != result)
    mxerror(fmt::format(FY("deflateInit() failed. Result: {0}\n"), result));

  c_stream.next_in   = (Bytef *)buffer;
  c_stream.avail_in  = size;
  int n              = 0;
  memory_cptr dst    = memory_c::alloc(0);

  do {
    n++;
    dst->resize(n * 4000);
    c_stream.next_out  = reinterpret_cast<Bytef *>(dst->get_buffer() + (n - 1) * 4000);
    c_stream.avail_out = 4000;
    result             = deflate(&c_stream, Z_FINISH);

    if ((Z_OK != result) && (Z_STREAM_END != result))
      mxerror(fmt::format(FY("Zlib decompression failed. Result: {0}\n"), result));

  } while ((c_stream.avail_out == 0) && (result != Z_STREAM_END));

  dst->resize(c_stream.total_out);
  deflateEnd(&c_stream);

  mxdebug_if(m_debug, fmt::format("zlib_compressor_c: Compression from {0} to {1}, {2}%\n", size, dst->get_size(), dst->get_size() * 100 / size));

  return dst;
}
