/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <matroska/KaxCluster.h>

namespace mtx {

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
  MTX_DECLARE_PRIVATE(kax_info::private_c)

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
  void set_continue_at_cluster(bool enable);
  void set_show_all_elements(bool enable);
  void set_show_summary(bool enable);
  void set_show_hexdump(bool enable);
  void set_show_positions(bool enable);
  void set_show_size(bool enable);
  void set_show_track_info(bool enable);
  void set_hex_positions(bool enable);
  void set_hexdump_max_size(int max_size);
  void set_destination_file_name(std::string const &file_name);
  void set_source_file(mm_io_cptr const &file);
  void set_source_file_name(std::string const &file_name);
  void set_retain_elements(bool enable);

  void reset();
  virtual result_e open_and_process_file(std::string const &file_name);
  virtual result_e open_and_process_file();
  virtual result_e process_file();
  void abort();

  std::string create_element_text(std::string const &text, std::optional<int64_t> position, std::optional<int64_t> size, std::optional<int64_t> data_size);
  std::string create_unknown_element_text(libebml::EbmlElement &e);
  std::string create_known_element_but_not_allowed_here_text(libebml::EbmlElement &e);
  std::string create_hexdump(uint8_t const *buf, int size);
  std::string create_codec_dependent_private_info(libmatroska::KaxCodecPrivate &c_priv, char track_type, std::string const &codec_id);
  std::string create_text_representation(libebml::EbmlElement &e);
  std::string format_binary(libebml::EbmlBinary &bin);
  std::string format_binary_as_hex(libebml::EbmlElement &e);
  std::string format_element_size(libebml::EbmlElement &e);
  std::string format_element_value(libebml::EbmlElement &e);
  std::string format_element_value_default(libebml::EbmlElement &e);
  std::string format_unsigned_integer_as_timestamp(libebml::EbmlElement &e);
  std::string format_unsigned_integer_as_scaled_timestamp(libebml::EbmlElement &e);
  std::string format_signed_integer_as_timestamp(libebml::EbmlElement &e);
  std::string format_signed_integer_as_scaled_timestamp(libebml::EbmlElement &e);
  std::string format_block(libebml::EbmlElement &e);
  std::string format_simple_block(libebml::EbmlElement &e);

  bool pre_block_group(libebml::EbmlElement &e);
  bool pre_block(libebml::EbmlElement &e);
  bool pre_simple_block(libebml::EbmlElement &e);

  void post_block_group(libebml::EbmlElement &e);
  void post_block(libebml::EbmlElement &e);
  void post_simple_block(libebml::EbmlElement &e);

  bool run_generic_pre_processors(libebml::EbmlElement &e);
  void run_generic_post_processors(libebml::EbmlElement &e);

  virtual void ui_show_error(std::string const &error);
  virtual void ui_show_element_info(int level, std::string const &text, std::optional<int64_t> position, std::optional<int64_t> size, std::optional<int64_t> data_size);
  virtual void ui_show_element(libebml::EbmlElement &e);
  virtual void ui_show_progress(int percentage, std::string const &text);

  void discard_retained_element(libebml::EbmlElement &element_to_remove);

protected:
  void init();
  void init_custom_element_value_formatters_and_processors();

  void show_element(libebml::EbmlElement *l, int level, std::string const &info, std::optional<int64_t> position = {}, std::optional<int64_t> size = {});
  void show_frame_summary(libebml::EbmlElement &e);

  void add_track(std::shared_ptr<kax_info::track_t> const &t);
  kax_info::track_t *find_track(int tnum);

  void read_master(libebml::EbmlMaster *m, libebml::EbmlSemanticContext const &ctx, int &upper_lvl_el, libebml::EbmlElement *&l2);

  void handle_block_group(libebml::EbmlElement *&l2, libmatroska::KaxCluster *&cluster);
  void handle_elements_generic(libebml::EbmlElement &e);
  result_e handle_segment(libmatroska::KaxSegment &l0);

  void display_track_info();

  void retain_element(std::shared_ptr<libebml::EbmlElement> const &element);

public:
  static std::string format_ebml_id_as_hex(libebml::EbmlElement &e);
  static std::string format_ebml_id_as_hex(libebml::EbmlId const &id);
  static std::string format_ebml_id_as_hex(uint32_t id);
};
using kax_info_cptr = std::shared_ptr<kax_info_c>;

}
