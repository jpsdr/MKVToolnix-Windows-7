/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the OGG media stream reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ogg/ogg.h>

#include "common/codec.h"
#include "merge/generic_reader.h"
#include "common/theora.h"
#include "common/kate.h"

namespace mtx::tags {
struct converted_vorbis_comments_t;
struct vorbis_comments_t;
}

class ogm_reader_c;

class ogm_demuxer_c {
public:
  ogm_reader_c *reader;
  track_info_c &m_ti;

  ogg_stream_state os;
  int ptzr;
  int64_t track_id;

  codec_c codec;
  bool ms_compat;
  int serialno, eos;
  unsigned int units_processed, num_non_header_packets;
  bool headers_read;
  mtx::bcp47::language_c language;
  std::string title;
  std::vector<memory_cptr> packet_data, nh_packet_data;
  int64_t first_granulepos, last_granulepos, default_duration;
  bool in_use;

  int display_width, display_height;
  int channels, sample_rate, bits_per_sample;

  std::shared_ptr<libmatroska::KaxTags> m_tags;

public:
  ogm_demuxer_c(ogm_reader_c *p_reader);

  virtual ~ogm_demuxer_c();

  virtual const char *get_type() {
    return "unknown";
  };
  virtual std::string get_codec() {
    return codec.get_name("unknown");
  };

  virtual void initialize() {
  };
  virtual generic_packetizer_c *create_packetizer() {
    return nullptr;
  };
  virtual void process_page(int64_t granulepos);
  virtual void process_header_page();

  virtual void get_duration_and_len(ogg_packet &op, int64_t &duration, int &duration_len);
  virtual bool is_header_packet(ogg_packet &op) {
    return (op.bytes > 0) && ((op.packet[0] & 1) == 1);
  };

  virtual std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
};

using ogm_demuxer_cptr = std::shared_ptr<ogm_demuxer_c>;

class ogm_reader_c: public generic_reader_c {
private:
  ogg_sync_state oy;
  std::vector<ogm_demuxer_cptr> sdemuxers;
  int bos_pages_read;
  int64_t m_attachment_id{};
  bool m_charset_warning_printed{}, m_chapters_set{}, m_exception_parsing_chapters{}, m_segment_title_set{};
  charset_converter_cptr m_chapter_charset_converter;
  debugging_option_c m_debug_tags{"ogg_tags"};

public:
  virtual ~ogm_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::ogm;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  virtual bool probe_file() override;

private:
  virtual ogm_demuxer_cptr find_demuxer(int serialno);
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
  virtual int read_page(ogg_page *);
  virtual void handle_new_stream(ogg_page *);
  virtual void handle_new_stream_and_packets(ogg_page *);
  virtual void process_page(ogg_page *);
  virtual int packet_available();
  virtual int read_headers_internal();
  virtual void process_header_page(ogg_page *pg);
  virtual void process_header_packets(ogm_demuxer_c &dmx);
  virtual void handle_stream_comments();
  virtual void handle_cover_art(mtx::tags::converted_vorbis_comments_t const &converted);
  virtual void handle_chapters(mtx::tags::vorbis_comments_t const &comments);
  virtual void handle_language_and_title(mtx::tags::converted_vorbis_comments_t const &converted, ogm_demuxer_cptr const &dmx);
  virtual void handle_tags(mtx::tags::converted_vorbis_comments_t const &converted, ogm_demuxer_cptr const &dmx);
};
