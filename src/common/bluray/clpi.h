/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  definitions and helper functions for Blu-ray clip info data

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/bcp47.h"
#include "common/timestamp.h"

namespace mtx::bits {
class reader_c;
}

namespace mtx::bluray::clpi {

constexpr auto FILE_MAGIC   = mtx::calc_fourcc('H', 'D', 'M', 'V');
constexpr auto FILE_MAGIC2A = mtx::calc_fourcc('0', '2', '0', '0');
constexpr auto FILE_MAGIC2B = mtx::calc_fourcc('0', '1', '0', '0');
constexpr auto FILE_MAGIC2C = mtx::calc_fourcc('0', '3', '0', '0');

struct program_stream_t {
  uint16_t pid;
  uint8_t coding_type;
  uint8_t format;
  uint8_t rate;
  uint8_t aspect;
  uint8_t oc_flag;
  uint8_t char_code;
  mtx::bcp47::language_c language;

  program_stream_t();

  void dump();
};
using program_stream_cptr = std::shared_ptr<program_stream_t>;

struct program_t {
  uint32_t spn_program_sequence_start;
  uint16_t program_map_pid;
  uint8_t num_streams;
  uint8_t num_groups;
  std::vector<program_stream_cptr> program_streams;

  program_t();

  void dump();
};
using program_cptr = std::shared_ptr<program_t>;

struct ep_map_one_stream_t {
  struct coarse_point_t {
    uint64_t ref_to_fine_id{}, pts{}, spn{};
  };

  struct fine_point_t {
    uint64_t pts{}, spn{};
  };

  struct point_t {
    timestamp_c pts;
    uint64_t spn{};
  };

  uint16_t pid{}, type{};
  uint32_t num_coarse_points{}, num_fine_points{}, address{};

  std::vector<coarse_point_t> coarse_points;
  std::vector<fine_point_t>   fine_points;
  std::vector<point_t>        points;

  void dump(std::size_t idx) const;
};

class parser_c {
protected:
  std::string m_file_name;
  bool m_ok{};
  debugging_option_c m_debug{"clpi|clpi_parser"};

  std::size_t m_sequence_info_start{}, m_program_info_start{}, m_characteristic_point_info_start{};

public:
  std::vector<program_cptr> m_programs;
  std::vector<ep_map_one_stream_t> m_ep_map;

public:
  parser_c(std::string file_name);
  virtual ~parser_c() = default;

  virtual bool parse();
  virtual bool is_ok() {
    return m_ok;
  }

  virtual void dump();

protected:
  virtual void parse_header(mtx::bits::reader_c &bc);
  virtual void parse_program_info(mtx::bits::reader_c &bc);
  virtual void parse_program_stream(mtx::bits::reader_c &bc, program_t &program);
  virtual void parse_characteristic_point_info(mtx::bits::reader_c &bc);
  virtual void parse_ep_map(mtx::bits::reader_c &bc);
  virtual void parse_ep_map_for_one_stream_pid(mtx::bits::reader_c &bc, ep_map_one_stream_t &map);
};
using parser_cptr = std::shared_ptr<parser_c>;

}                             // namespace mtx::bluray::clpi
