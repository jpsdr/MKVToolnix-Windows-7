/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AV1 parser code

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/math_fwd.h"
#include "common/av1/types.h"
#include "common/dovi_meta.h"

namespace mtx {

namespace bits {
class reader_c;
}

namespace av1 {

class exception: public mtx::exception {
public:
  virtual char const *what() const throw() override {
    return "OBU exception base class";
  }
};

class obu_without_size_unsupported_x: public exception {
public:
  virtual char const *what() const throw() override {
    return Y("Raw OBUs without a size field are not supported.");
  }
};

class obu_invalid_structure_x: public exception {
public:
  virtual char const *what() const throw() override {
    return Y("Encountered OBUs with invalid data.");
  }
};

namespace {

unsigned int constexpr OBU_SEQUENCE_HEADER         =  1;
unsigned int constexpr OBU_TEMPORAL_DELIMITER      =  2;
unsigned int constexpr OBU_FRAME_HEADER            =  3;
unsigned int constexpr OBU_TILE_GROUP              =  4;
unsigned int constexpr OBU_METADATA                =  5;
unsigned int constexpr OBU_FRAME                   =  6;
unsigned int constexpr OBU_REDUNDANT_FRAME_HEADER  =  7;
unsigned int constexpr OBU_TILE_LIST               =  8;
unsigned int constexpr OBU_PADDING                 = 15;

unsigned int constexpr FRAME_TYPE_KEY              =  0;
unsigned int constexpr FRAME_TYPE_INTER            =  1;
unsigned int constexpr FRAME_TYPE_INTRA_ONLY       =  2;
unsigned int constexpr FRAME_TYPE_SWITCH           =  3;

unsigned int constexpr SELECT_SCREEN_CONTENT_TOOLS =  2;

unsigned int constexpr CP_BT_709                   =  1;
unsigned int constexpr CP_UNSPECIFIED              =  2;

unsigned int constexpr TC_UNSPECIFIED              =  2;
unsigned int constexpr TC_SRGB                     = 13;

unsigned int constexpr MC_IDENTITY                 =  0;
unsigned int constexpr MC_UNSPECIFIED              =  2;

unsigned int constexpr METADATA_TYPE_ITUT_T35      =  4;

uint8_t constexpr ITU_T_T35_DOVI_RPU_PAYLOAD_HEADER[] = {
  0x00, 0x3B, 0x00, 0x00, 0x08, 0x00, 0x37, 0xCD, 0x08
};

}

struct frame_t {
  memory_cptr mem;
  uint64_t timestamp{};
  bool is_keyframe{};
};

class parser_private_c;
class parser_c {
protected:
  std::unique_ptr<parser_private_c> const p;

  mtx::dovi::dovi_rpu_data_header_t m_dovi_rpu_data_header;

public:
  parser_c();
  ~parser_c();

  void set_default_duration(mtx_mp_rational_t default_duration);
  void set_parse_sequence_header_obus_only(bool parse_sequence_header_obus_only);

  void parse(uint8_t const *buffer, uint64_t buffer_size);
  void parse(memory_c const &buffer);

  void flush();
  bool frame_available() const;
  frame_t get_next_frame();

  bool is_keyframe(uint8_t const *buffer, uint64_t buffer_size);
  bool is_keyframe(memory_c const &buffer);

  void debug_obu_types(uint8_t const *buffer, uint64_t buffer_size);
  void debug_obu_types(memory_c const &buffer);

  std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
  mtx_mp_rational_t get_frame_duration() const;
  bool headers_parsed() const;
  memory_cptr get_av1c() const;
  color_config_t get_color_config() const;

  bool has_dovi_rpu_header() const {
    return m_dovi_rpu_data_header.rpu_nal_prefix == 25;
  }

  mtx::dovi::dovi_rpu_data_header_t get_dovi_rpu_header() const {
    return m_dovi_rpu_data_header;
  }

public:
  static char const *get_obu_type_name(unsigned int obu_type);

protected:
  static uint64_t read_leb128(mtx::bits::reader_c &r);
  static uint64_t read_uvlc(mtx::bits::reader_c &r);

  std::optional<uint64_t> parse_obu_common_data(uint8_t const *buffer, uint64_t buffer_size);
  std::optional<uint64_t> parse_obu_common_data(memory_c const &buffer);
  std::optional<uint64_t> parse_obu_common_data();
  void parse_sequence_header_obu(mtx::bits::reader_c &r);
  void parse_color_config(mtx::bits::reader_c &r);
  void parse_timing_info(mtx::bits::reader_c &r);
  void parse_decoder_model_info(mtx::bits::reader_c &r);
  void parse_operating_parameters_info(mtx::bits::reader_c &r);
  void parse_frame_header_obu(mtx::bits::reader_c &r);
  void parse_metadata_obu(mtx::bits::reader_c &r);
  bool parse_obu();

  uint64_t get_next_timestamp();

  void parse_metadata_type_itu_t_t35(mtx::bits::reader_c &r);
  void handle_itu_t_t35_dovi_rpu_payload(const memory_cptr payload_mem);
};

void maybe_shrink_size_fields(memory_c &mem);

}
}
