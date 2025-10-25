/** MPEG-4 p10 AVCC functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/avc/avcc.h"
#include "common/avc/util.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mpeg.h"

namespace mtx::avc {

avcc_c::avcc_c()
  : m_profile_idc{}
  , m_profile_compat{}
  , m_level_idc{}
  , m_nalu_size_length{}
{
}

avcc_c::avcc_c(unsigned int nalu_size_length,
               std::vector<memory_cptr> sps_list,
               std::vector<memory_cptr> pps_list)
  : m_profile_idc{}
  , m_profile_compat{}
  , m_level_idc{}
  , m_nalu_size_length{nalu_size_length}
  , m_sps_list{std::move(sps_list)}
  , m_pps_list{std::move(pps_list)}
{
}

avcc_c::operator bool()
  const {
  return m_nalu_size_length
      && !m_sps_list.empty()
      && !m_pps_list.empty()
      && (m_sps_info_list.empty() || (m_sps_info_list.size() == m_sps_list.size()))
      && (m_pps_info_list.empty() || (m_pps_info_list.size() == m_pps_list.size()));
}

bool
avcc_c::parse_sps_list(bool ignore_errors) {
  if (m_sps_info_list.size() == m_sps_list.size())
    return true;

  m_sps_info_list.clear();
  for (auto &sps: m_sps_list) {
    sps_info_t sps_info;
    auto sps_as_rbsp = mtx::mpeg::nalu_to_rbsp(sps);

    if (ignore_errors) {
      try {
        parse_sps(sps_as_rbsp, sps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_sps(sps_as_rbsp, sps_info))
      return false;

    m_sps_info_list.push_back(sps_info);
  }

  return true;
}

bool
avcc_c::parse_pps_list(bool ignore_errors) {
  if (m_pps_info_list.size() == m_pps_list.size())
    return true;

  m_pps_info_list.clear();
  for (auto &pps: m_pps_list) {
    pps_info_t pps_info;
    auto pps_as_rbsp = mtx::mpeg::nalu_to_rbsp(pps);

    if (ignore_errors) {
      try {
        parse_pps(pps_as_rbsp, pps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_pps(pps_as_rbsp, pps_info))
      return false;

    m_pps_info_list.push_back(pps_info);
  }

  return true;
}

memory_cptr
avcc_c::pack() {
  parse_sps_list(true);
  if (!*this)
    return memory_cptr{};

  unsigned int total_size = 6 + 1;

  for (auto &mem : m_sps_list)
    total_size += mem->get_size() + 2;
  for (auto &mem : m_pps_list)
    total_size += mem->get_size() + 2;

  if (m_trailer)
    total_size += m_trailer->get_size();

  auto destination = memory_c::alloc(total_size);
  auto buffer      = destination->get_buffer();
  auto &sps        = *m_sps_info_list.begin();
  auto write_list  = [&buffer](std::vector<memory_cptr> const &list, uint8_t num_byte_bits) {
    *buffer = list.size() | num_byte_bits;
    ++buffer;

    for (auto &mem : list) {
      auto size = mem->get_size();
      put_uint16_be(buffer, size);
      memcpy(buffer + 2, mem->get_buffer(), size);
      buffer += 2 + size;
    }
  };

  buffer[0] = 1;
  buffer[1] = m_profile_idc    ? m_profile_idc    : sps.profile_idc;
  buffer[2] = m_profile_compat ? m_profile_compat : sps.profile_compat;
  buffer[3] = m_level_idc      ? m_level_idc      : sps.level_idc;
  buffer[4] = 0xfc | (m_nalu_size_length - 1);
  buffer   += 5;

  write_list(m_sps_list, 0xe0);
  write_list(m_pps_list, 0x00);

  if (m_trailer)
    memcpy(buffer, m_trailer->get_buffer(), m_trailer->get_size());

  return destination;
}

avcc_c
avcc_c::unpack(memory_cptr const &mem) {
  avcc_c avcc;

  if (!mem || (6 > mem->get_size()))
    return avcc;

  try {
    mm_mem_io_c in{*mem};

    auto read_list = [&in](std::vector<memory_cptr> &list, unsigned int num_entries_mask) {
      auto num_entries = in.read_uint8() & num_entries_mask;
      while (num_entries) {
        auto size = in.read_uint16_be();
        list.push_back(in.read(size));
        --num_entries;
      }
    };

    in.skip(1);                 // always 1
    avcc.m_profile_idc      = in.read_uint8();
    avcc.m_profile_compat   = in.read_uint8();
    avcc.m_level_idc        = in.read_uint8();
    avcc.m_nalu_size_length = (in.read_uint8() & 0x03) + 1;

    read_list(avcc.m_sps_list, 0x0f);
    read_list(avcc.m_pps_list, 0xff);

    if (in.getFilePointer() < static_cast<uint64_t>(in.get_size()))
      avcc.m_trailer = in.read(in.get_size() - in.getFilePointer());

    return avcc;

  } catch (mtx::mm_io::exception &) {
    return avcc_c{};
  }
}

}
