/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AV1 parser code

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace av1 {

class obu_without_size_unsupported_x: public mtx::exception {
public:
  virtual char const *what() const throw() override {
    return Y("Raw OBUs without a size field are not supported.");
  }
};

class obu_invalid_structure_x: public mtx::exception {
public:
  virtual char const *what() const throw() override {
    return Y("Encountered OBUs with invalid data.");
  }
};

namespace {

unsigned int constexpr OBU_SEQUENCE_HEADER        =  1;
unsigned int constexpr OBU_TD                     =  2;
unsigned int constexpr OBU_FRAME_HEADER           =  3;
unsigned int constexpr OBU_TITLE_GROUP            =  4;
unsigned int constexpr OBU_METADATA               =  5;
unsigned int constexpr OBU_FRAME                  =  6;
unsigned int constexpr OBU_REDUNDANT_FRAME_HEADER =  7;
unsigned int constexpr OBU_PADDING                = 15;

unsigned int constexpr FRAME_TYPE_KEY             =  0;
unsigned int constexpr FRAME_TYPE_INTER           =  1;
unsigned int constexpr FRAME_TYPE_INTRA_ONLY      =  2;
unsigned int constexpr FRAME_TYPE_SWITCH          =  3;

}

class parser_private_c;
class parser_c {
protected:
  std::unique_ptr<parser_private_c> const p;

public:
  parser_c();
  ~parser_c();

  boost::optional<uint64_t> parse_obu_common_data(unsigned char const *buffer, uint64_t buffer_size);
  boost::optional<uint64_t> parse_obu_common_data(memory_c const &buffer);
  bool parse_obu(unsigned char const *buffer, uint64_t buffer_size, uint64_t unit_size);

  bool is_keyframe(unsigned char const *buffer, uint64_t buffer_size);
  bool is_keyframe(memory_c const &buffer);

public:
  static char const *get_obu_type_name(unsigned int obu_type);

protected:
  uint64_t read_leb128();

  boost::optional<uint64_t> parse_obu_common_data();
  bool parse_sequence_header_obu();
};

}}
