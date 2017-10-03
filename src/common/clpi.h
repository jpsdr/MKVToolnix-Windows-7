/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  definitions and helper functions for BluRay clip info data

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

namespace mtx { namespace bits {
class reader_c;
}}

#define CLPI_FILE_MAGIC   FOURCC('H', 'D', 'M', 'V')
#define CLPI_FILE_MAGIC2A FOURCC('0', '2', '0', '0')
#define CLPI_FILE_MAGIC2B FOURCC('0', '1', '0', '0')
#define CLPI_FILE_MAGIC2C FOURCC('0', '3', '0', '0')

namespace mtx { namespace bluray { namespace clpi {
  struct program_stream_t {
    uint16_t pid;
    unsigned char coding_type;
    unsigned char format;
    unsigned char rate;
    unsigned char aspect;
    unsigned char oc_flag;
    unsigned char char_code;
    std::string language;

    program_stream_t();

    void dump();
  };
  using program_stream_cptr = std::shared_ptr<program_stream_t>;

  struct program_t {
    uint32_t spn_program_sequence_start;
    uint16_t program_map_pid;
    unsigned char num_streams;
    unsigned char num_groups;
    std::vector<program_stream_cptr> program_streams;

    program_t();

    void dump();
  };
  using program_cptr = std::shared_ptr<program_t>;

  class parser_c {
  protected:
    std::string m_file_name;
    bool m_ok;
    debugging_option_c m_debug;

    size_t m_sequence_info_start, m_program_info_start;

  public:
    std::vector<program_cptr> m_programs;

  public:
    parser_c(const std::string &file_name);
    virtual ~parser_c();

    virtual bool parse();
    virtual bool is_ok() {
      return m_ok;
    }

    virtual void dump();

  protected:
    virtual void parse_header(mtx::bits::reader_c &bc);
    virtual void parse_program_info(mtx::bits::reader_c &bc);
    virtual void parse_program_stream(mtx::bits::reader_c &bc, program_cptr &program);
  };
  using parser_cptr = std::shared_ptr<parser_c>;

}}}                             // namespace mtx::bluray::clpi
