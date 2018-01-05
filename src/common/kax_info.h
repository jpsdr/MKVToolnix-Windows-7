/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxCluster.h>

namespace mtx {

using namespace libebml;
using namespace libmatroska;

namespace kax_info {

class exception: public std::runtime_error {
public:
  exception(std::string const &error)
    : std::runtime_error{error}
  {
  }
};

struct track_t;
class private_c;

}

class kax_info_c {
protected:
  MTX_DECLARE_PRIVATE(kax_info::private_c);

  std::unique_ptr<kax_info::private_c> const p_ptr;

  explicit kax_info_c(kax_info::private_c &p);

public:
  enum class result_e {
    succeeded,
    failed,
    aborted,
  };

public:
  kax_info_c();
  virtual ~kax_info_c();

  void set_use_gui(bool enable);
  void set_calc_checksums(bool enable);
  void set_show_summary(bool enable);
  void set_show_hexdump(bool enable);
  void set_show_size(bool enable);
  void set_show_track_info(bool enable);
  void set_hex_positions(bool enable);
  void set_hexdump_max_size(int max_size);
  void set_verbosity(int verbosity);
  void set_destination_file_name(std::string const &file_name);

  void reset();
  virtual result_e open_and_process_file(std::string const &file_name);
  virtual result_e process_file(mm_io_cptr const &file);
  void abort();

  std::string create_element_text(std::string const &text, int64_t position, int64_t size);
  std::string create_unknown_element_text(EbmlElement &e);
  std::string create_known_element_but_not_allowed_here_text(EbmlElement &e);
  std::string create_hexdump(unsigned char const *buf, int size);
  std::string create_codec_dependent_private_info(KaxCodecPrivate &c_priv, char track_type, std::string const &codec_id);
  std::string create_text_representation(EbmlElement &e);
  std::string format_binary(EbmlBinary &bin, std::size_t max_len = 16);
  std::string format_binary_as_hex(EbmlElement &e);
  std::string format_element_size(EbmlElement &e);
  std::string format_element_value(EbmlElement &e);
  std::string format_element_value_default(EbmlElement &e);
  std::string format_ebml_id_as_hex(EbmlElement &e);
  std::string format_ebml_id_as_hex(EbmlId const &id);
  std::string format_unsigned_integer_as_timestamp(EbmlElement &e);
  std::string format_unsigned_integer_as_scaled_timestamp(EbmlElement &e);
  std::string format_signed_integer_as_timestamp(EbmlElement &e);
  std::string format_signed_integer_as_scaled_timestamp(EbmlElement &e);
  std::string format_block(EbmlElement &e);
  std::string format_simple_block(EbmlElement &e);

  bool pre_block_group(EbmlElement &e);
  bool pre_block(EbmlElement &e);
  bool pre_simple_block(EbmlElement &e);

  void post_block_group(EbmlElement &e);
  void post_block(EbmlElement &e);
  void post_simple_block(EbmlElement &e);

  virtual void ui_show_error(std::string const &error);
  virtual void ui_show_element_info(int level, std::string const &text, int64_t position, int64_t size);
  virtual void ui_show_element(EbmlElement &e);
  virtual void ui_show_progress(int percentage, std::string const &text);

protected:
  void init();
  void init_custom_element_value_formatters_and_processors();

  void show_element(EbmlElement *l, int level, std::string const &info);
  void show_element(EbmlElement *l, int level, boost::format const &info);

  void add_track(std::shared_ptr<kax_info::track_t> const &t);
  kax_info::track_t *find_track(int tnum);

  void read_master(EbmlMaster *m, EbmlSemanticContext const &ctx, int &upper_lvl_el, EbmlElement *&l2);

  void handle_block_group(EbmlElement *&l2, KaxCluster *&cluster);
  void handle_elements_generic(EbmlElement &e);
  result_e handle_segment(EbmlElement *l0);

  void display_track_info();

public:
  static void init_common_formats();
};
using kax_info_cptr = std::shared_ptr<kax_info_c>;

}
