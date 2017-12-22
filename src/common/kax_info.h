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

#include <matroska/KaxSegment.h>
#include <matroska/KaxCluster.h>

#include "common/xml/ebml_converter.h"

namespace mtx {

using namespace libebml;
using namespace libmatroska;

class kax_info_x: public std::runtime_error {
public:
  kax_info_x(std::string const &error)
    : std::runtime_error{error}
  {
  }
};

class kax_info_c {
protected:
  struct track_t {
    uint64_t tnum{}, tuid{};
    char type{' '};
    int64_t default_duration{};
    std::size_t mkvmerge_track_id{};
  };

  struct track_info_t {
    int64_t m_size{}, m_blocks{}, m_blocks_by_ref_num[3]{0, 0, 0}, m_add_duration_for_n_packets{};
    boost::optional<int64_t> m_min_timestamp, m_max_timestamp;
  };

  using track_cptr = std::shared_ptr<track_t>;

protected:
  static std::vector<boost::format> ms_common_boost_formats;

  std::vector<track_cptr> m_tracks;
  std::map<unsigned int, track_cptr> m_tracks_by_number;
  std::map<unsigned int, track_info_t> m_track_info;
  uint64_t m_ts_scale{TIMESTAMP_SCALE};
  std::size_t m_mkvmerge_track_id{};
  std::shared_ptr<EbmlStream> m_es;
  mm_io_cptr m_in;

  bool m_use_gui{}, m_calc_checksums{}, m_show_summary{}, m_show_hexdump{}, m_show_size{}, m_show_track_info{}, m_hex_positions{};
  int m_hexdump_max_size{}, m_verbose{};

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

  void reset();
  bool process_file(std::string const &file_name);

  std::string create_element_text(std::string const &text, int64_t position, int64_t size);
  std::string create_hexdump(unsigned char const *buf, int size);
  std::string create_codec_dependent_private_info(KaxCodecPrivate &c_priv, char track_type, std::string const &codec_id);
  std::string format_binary(EbmlBinary &bin, std::size_t max_len = 16);

  virtual void ui_show_error(std::string const &error);
  virtual void ui_show_element(int level, std::string const &text, int64_t position, int64_t size);
  virtual void ui_show_progress(int percentage, std::string const &text);

protected:
  void show_unknown_element(EbmlElement *e, int level);
  void show_element(EbmlElement *l, int level, std::string const &info, bool skip = false);
  void show_element(EbmlElement *l, int level, boost::format const &info, bool skip = false);

  void add_track(track_cptr const &t);
  track_t *find_track(int tnum);

  bool is_global(EbmlElement *l, int level);
  void read_master(EbmlMaster *m, EbmlSemanticContext const &ctx, int &upper_lvl_el, EbmlElement *&l2);

  void handle_chaptertranslate(EbmlElement *&l2);
  void handle_info(int &upper_lvl_el, EbmlElement *&l1);
  void handle_audio_track(EbmlElement *&l3, std::vector<std::string> &summary);
  void handle_video_colour_master_meta(EbmlElement *&l5);
  void handle_video_colour(EbmlElement *&l4);
  void handle_video_projection(EbmlElement *&l4);
  void handle_video_track(EbmlElement *&l3, std::vector<std::string> &summary);
  void handle_content_encodings(EbmlElement *&l3);
  void handle_tracks(int &upper_lvl_el, EbmlElement *&l1);
  void handle_seek_head(int &upper_lvl_el, EbmlElement *&l1);
  void handle_cues(int &upper_lvl_el, EbmlElement *&l1);
  void handle_attachments(int &upper_lvl_el, EbmlElement *&l1);
  void handle_silent_track(EbmlElement *&l2);
  void handle_block_group(EbmlElement *&l2, KaxCluster *&cluster);
  void handle_simple_block(EbmlElement *&l2, KaxCluster *&cluster);
  void handle_cluster(int &upper_lvl_el, EbmlElement *&l1, int64_t file_size);
  void handle_elements_rec(int level, EbmlElement *e, mtx::xml::ebml_converter_c const &converter);
  void handle_chapters(int &upper_lvl_el, EbmlElement *&l1);
  void handle_tags(int &upper_lvl_el, EbmlElement *&l1);
  void handle_ebml_head(EbmlElement *l0);
  void handle_segment(EbmlElement *l0);

  void display_track_info();

public:
  static void init_common_boost_formats();
};
using kax_info_cptr = std::shared_ptr<kax_info_c>;

}
