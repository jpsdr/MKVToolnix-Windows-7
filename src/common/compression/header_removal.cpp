/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   header removal compressor

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/ac3.h"
#include "common/dirac.h"
#include "common/dts.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/compression/header_removal.h"

header_removal_compressor_c::header_removal_compressor_c()
  : compressor_c(COMPRESSION_HEADER_REMOVAL)
{
}

memory_cptr
header_removal_compressor_c::do_decompress(uint8_t const *buffer,
                                           std::size_t size) {
  if (!m_bytes || (0 == m_bytes->get_size()))
    return memory_c::clone(buffer, size);

  memory_cptr new_buffer = memory_c::alloc(size + m_bytes->get_size());

  memcpy(new_buffer->get_buffer(),                       m_bytes->get_buffer(), m_bytes->get_size());
  memcpy(new_buffer->get_buffer() + m_bytes->get_size(), buffer,                size);

  return new_buffer;
}

memory_cptr
header_removal_compressor_c::do_compress(uint8_t const *buffer,
                                         std::size_t size) {
  if (!m_bytes || (0 == m_bytes->get_size()))
    return memory_c::clone(buffer, size);

  size_t to_remove_size = m_bytes->get_size();
  if (size < to_remove_size)
    throw mtx::compression_x(fmt::format(FY("Header removal compression not possible because the buffer contained {0} bytes "
                                            "which is less than the size of the headers that should be removed, {1}."), size, to_remove_size));

  auto bytes_ptr = m_bytes->get_buffer();

  if (memcmp(buffer, bytes_ptr, to_remove_size)) {
    std::string b_buffer, b_bytes;
    size_t i;

    for (i = 0; to_remove_size > i; ++i) {
      b_buffer += fmt::format(" {0:02x}", static_cast<unsigned int>(buffer[i]));
      b_bytes  += fmt::format(" {0:02x}", static_cast<unsigned int>(bytes_ptr[i]));
    }
    throw mtx::compression_x(fmt::format(FY("Header removal compression not possible because the buffer did not start with the bytes that should be removed. "
                                            "Wanted bytes:{0}; found:{1}."), b_bytes, b_buffer));
  }

  return memory_c::clone(buffer + size, size - to_remove_size);
}

void
header_removal_compressor_c::set_track_headers(libmatroska::KaxContentEncoding &c_encoding) {
  compressor_c::set_track_headers(c_encoding);

  // Set compression parameters.
  get_child<libmatroska::KaxContentCompSettings>(get_child<libmatroska::KaxContentCompression>(c_encoding)).CopyBuffer(m_bytes->get_buffer(), m_bytes->get_size());
}

// ------------------------------------------------------------

analyze_header_removal_compressor_c::analyze_header_removal_compressor_c()
  : compressor_c(COMPRESSION_ANALYZE_HEADER_REMOVAL)
  , m_packet_counter(0)
{
}

analyze_header_removal_compressor_c::~analyze_header_removal_compressor_c() {
  if (!m_bytes)
    mxinfo("Analysis failed: no packet encountered\n");

  else if (m_bytes->get_size() == 0)
    mxinfo("Analysis complete but no similarities found.\n");

  else {
    mxinfo(fmt::format("Analysis complete. {0} identical byte(s) at the start of each of the {1} packet(s). Hex dump of the content:\n", m_bytes->get_size(), m_packet_counter));
    debugging_c::hexdump(m_bytes->get_buffer(), m_bytes->get_size());
  }
}

memory_cptr
analyze_header_removal_compressor_c::do_decompress(uint8_t const *,
                                                   std::size_t) {
  mxerror("analyze_header_removal_compressor_c::do_decompress(): not supported\n");
  return {};
}

memory_cptr
analyze_header_removal_compressor_c::do_compress(uint8_t const *buffer,
                                                 std::size_t size) {
  ++m_packet_counter;

  if (!m_bytes) {
    m_bytes = memory_c::clone(buffer, size);
    return memory_c::clone(buffer, size);
  }

  auto saved         = m_bytes->get_buffer();
  size_t i, new_size = 0;

  for (i = 0; i < std::min(size, m_bytes->get_size()); ++i, ++new_size)
    if (buffer[i] != saved[i])
      break;

  m_bytes->set_size(new_size);

  return memory_c::clone(buffer, size);
}

void
analyze_header_removal_compressor_c::set_track_headers(libmatroska::KaxContentEncoding &) {
}

// ------------------------------------------------------------

mpeg4_p2_compressor_c::mpeg4_p2_compressor_c() {
  memory_cptr bytes = memory_c::alloc(3);
  put_uint24_be(bytes->get_buffer(), 0x000001);
  set_bytes(bytes);
}

mpeg4_p10_compressor_c::mpeg4_p10_compressor_c() {
  memory_cptr bytes      = memory_c::alloc(1);
  bytes->get_buffer()[0] = 0;
  set_bytes(bytes);
}

dirac_compressor_c::dirac_compressor_c() {
  memory_cptr bytes = memory_c::alloc(4);
  put_uint32_be(bytes->get_buffer(), mtx::dirac::SYNC_WORD);
  set_bytes(bytes);
}

dts_compressor_c::dts_compressor_c() {
  memory_cptr bytes = memory_c::alloc(4);
  put_uint32_be(bytes->get_buffer(), static_cast<uint32_t>(mtx::dts::sync_word_e::core));
  set_bytes(bytes);
}

ac3_compressor_c::ac3_compressor_c() {
  memory_cptr bytes = memory_c::alloc(2);
  put_uint16_be(bytes->get_buffer(), mtx::ac3::SYNC_WORD);
  set_bytes(bytes);
}

mp3_compressor_c::mp3_compressor_c() {
  memory_cptr bytes = memory_c::alloc(1);
  bytes->get_buffer()[0] = 0xff;
  set_bytes(bytes);
}
