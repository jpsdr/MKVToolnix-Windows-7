/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the Quicktime & MP4 reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ac3.h"
#include "common/codec.h"
#include "common/dts.h"
#include "common/fourcc.h"
#include "input/packet_converter.h"
#include "input/qtmp4_atoms.h"
#include "merge/generic_reader.h"
#include "output/p_pcm.h"
#include "output/p_video_for_windows.h"

constexpr auto QTMP4_TKHD_FLAG_ENABLED            = 0x00000001;

constexpr auto QTMP4_TFHD_BASE_DATA_OFFSET        = 0x000001;
constexpr auto QTMP4_TFHD_SAMPLE_DESCRIPTION_ID   = 0x000002;
constexpr auto QTMP4_TFHD_DEFAULT_DURATION        = 0x000008;
constexpr auto QTMP4_TFHD_DEFAULT_SIZE            = 0x000010;
constexpr auto QTMP4_TFHD_DEFAULT_FLAGS           = 0x000020;
constexpr auto QTMP4_TFHD_DURATION_IS_EMPTY       = 0x010000;
constexpr auto QTMP4_TFHD_DEFAULT_BASE_IS_MOOF    = 0x020000;

constexpr auto QTMP4_TRUN_DATA_OFFSET             = 0x000001;
constexpr auto QTMP4_TRUN_FIRST_SAMPLE_FLAGS      = 0x000004;
constexpr auto QTMP4_TRUN_SAMPLE_DURATION         = 0x000100;
constexpr auto QTMP4_TRUN_SAMPLE_SIZE             = 0x000200;
constexpr auto QTMP4_TRUN_SAMPLE_FLAGS            = 0x000400;
constexpr auto QTMP4_TRUN_SAMPLE_CTS_OFFSET       = 0x000800;

constexpr auto QTMP4_FRAG_SAMPLE_FLAG_IS_NON_SYNC = 0x00010000;
constexpr auto QTMP4_FRAG_SAMPLE_FLAG_DEPENDS_YES = 0x01000000;

struct qt_durmap_t {
  uint32_t number;
  uint32_t duration;

  qt_durmap_t()
    : number{}
    , duration{}
  {
  }

  qt_durmap_t(uint32_t p_number, uint32_t p_duration)
    : number{p_number}
    , duration{p_duration}
  {
  }
};

struct qt_chunk_t {
  uint32_t samples;
  uint32_t size;
  uint32_t desc;
  uint64_t pos;

  qt_chunk_t()
    : samples{}
    , size{}
    , desc{}
    , pos{}
  {
  }

  qt_chunk_t(uint32_t p_size, uint64_t p_pos)
    : samples{}
    , size{p_size}
    , desc{}
    , pos{p_pos}
  {
  }
};

struct qt_chunkmap_t {
  uint32_t first_chunk;
  uint32_t samples_per_chunk;
  uint32_t sample_description_id;

  qt_chunkmap_t()
    : first_chunk{}
    , samples_per_chunk{}
    , sample_description_id{}
  {
  }
};

struct qt_editlist_t {
  int64_t  segment_duration{}, media_time{};
  uint16_t media_rate_integer{}, media_rate_fraction{};

  bool operator ==(qt_editlist_t const &other) const {
    return (segment_duration    == other.segment_duration)
        && (media_time          == other.media_time)
        && (media_rate_integer  == other.media_rate_integer)
        && (media_rate_fraction == other.media_rate_fraction);
  };
};

struct qt_sample_t {
  int64_t  pts;
  uint32_t size;
  int64_t  pos;

  qt_sample_t()
    : pts{}
    , size{}
    , pos{}
  {
  }

  qt_sample_t(uint32_t p_size)
    : pts{}
    , size{p_size}
    , pos{}
  {
  }
};

struct qt_frame_offset_t {
  unsigned int count;
  int64_t offset;

  qt_frame_offset_t()
    : count{}
    , offset{}
  {
  }

  qt_frame_offset_t(unsigned int p_count, int64_t p_offset)
    : count{p_count}
    , offset{p_offset}
  {
  }
};

struct qt_index_t {
  int64_t file_pos, size;
  int64_t timestamp, duration;
  bool    is_keyframe;

  qt_index_t()
    : file_pos{}
    , size{}
    , timestamp{}
    , duration{}
    , is_keyframe{}
  {
  };

  qt_index_t(int64_t p_file_pos, int64_t p_size, int64_t p_timestamp, int64_t p_duration, bool p_is_keyframe)
    : file_pos{p_file_pos}
    , size{p_size}
    , timestamp{p_timestamp}
    , duration{p_duration}
    , is_keyframe{p_is_keyframe}
  {
  }
};

struct qt_track_defaults_t {
  unsigned int sample_description_id, sample_duration, sample_size, sample_flags;

  qt_track_defaults_t()
    : sample_description_id{}
    , sample_duration{}
    , sample_size{}
    , sample_flags{}
  {
  }
};

struct qt_fragment_t {
  unsigned int track_id, sample_description_id, sample_duration, sample_size, sample_flags;
  uint64_t base_data_offset, moof_offset, implicit_offset;

  qt_fragment_t()
    : track_id{}
    , sample_description_id{}
    , sample_duration{}
    , sample_size{}
    , sample_flags{}
    , base_data_offset{}
    , moof_offset{}
    , implicit_offset{}
  {}
};

struct qt_random_access_point_t {
  bool num_leading_samples_known{};
  unsigned int num_leading_samples{};

  qt_random_access_point_t(bool p_num_leading_samples_known, unsigned int p_num_leading_samples)
    : num_leading_samples_known{p_num_leading_samples_known}
    , num_leading_samples{p_num_leading_samples}
  {
  }
};

struct qt_sample_to_group_t {
  uint32_t sample_count{}, group_description_index{};

  qt_sample_to_group_t(uint32_t p_sample_count, uint32_t p_group_description_index)
    : sample_count{p_sample_count}
    , group_description_index{p_group_description_index}
  {
  }
};

class qtmp4_reader_c;

struct qtmp4_demuxer_c {
  qtmp4_reader_c &m_reader;

  bool ok{}, m_tables_updated{}, m_timestamps_calculated{}, m_enabled{true};

  char type;
  uint32_t id, container_id;
  fourcc_c fourcc;
  uint32_t pos;

  codec_c codec;
  pcm_packetizer_c::pcm_format_e m_pcm_format;

  int64_t time_scale, track_duration, global_duration, num_frames_from_trun;
  uint32_t sample_size;
  double yaw, roll;

  std::vector<qt_sample_t> sample_table;
  std::vector<qt_chunk_t> chunk_table;
  std::vector<qt_chunkmap_t> chunkmap_table;
  std::vector<qt_durmap_t> durmap_table;
  std::vector<uint32_t> keyframe_table;
  std::vector<qt_editlist_t> editlist_table;
  std::vector<qt_frame_offset_t> raw_frame_offset_table;
  std::vector<int32_t> frame_offset_table;
  std::vector<qt_random_access_point_t> random_access_point_table;
  std::unordered_map<uint32_t, std::vector<qt_sample_to_group_t> > sample_to_group_tables;

  std::vector<int64_t> timestamps, durations, frame_indices;

  std::vector<qt_index_t> m_index;
  std::vector<qt_fragment_t> m_fragments;

  mtx_mp_rational_t frame_rate;
  std::optional<int64_t> m_use_frame_rate_for_duration;

  std::vector<block_addition_mapping_t> m_block_addition_mappings;

  esds_t esds;
  bool esds_parsed;

  memory_cptr stsd;
  unsigned int stsd_non_priv_struct_size;
  uint32_t v_width, v_height, v_bitdepth, v_display_width_flt{}, v_display_height_flt{};
  uint16_t v_color_primaries, v_color_transfer_characteristics, v_color_matrix_coefficients;
  bool m_hevc_is_annex_b{};
  std::deque<int64_t> references;
  uint32_t a_channels, a_bitdepth, a_flags{};
  double a_samplerate;
  std::optional<mtx::aac::audio_config_t> a_aac_audio_config;
  mtx::ac3::frame_c m_ac3_header;
  mtx::dts::header_t m_dts_header;

  std::vector<memory_cptr> priv;

  bool warning_printed;

  int ptzr;
  packet_converter_cptr m_converter;

  mtx::bcp47::language_c language;

  debugging_option_c m_debug_tables, m_debug_tables_full, m_debug_frame_rate, m_debug_headers, m_debug_editlists, m_debug_indexes, m_debug_indexes_full;

  qtmp4_demuxer_c(qtmp4_reader_c &reader)
    : m_reader(reader)
    , type{'?'}
    , id{0}
    , container_id{0}
    , pos{0}
    , m_pcm_format{pcm_packetizer_c::little_endian_integer}
    , time_scale{1}
    , track_duration{0}
    , global_duration{0}
    , num_frames_from_trun{}
    , sample_size{0}
    , esds_parsed{false}
    , stsd_non_priv_struct_size{}
    , v_width{0}
    , v_height{0}
    , v_bitdepth{0}
    , v_color_primaries{2}
    , v_color_transfer_characteristics{2}
    , v_color_matrix_coefficients{2}
    , a_channels{0}
    , a_bitdepth{0}
    , a_samplerate{0.0}
    , warning_printed{false}
    , ptzr{-1}
    , m_debug_tables{            "qtmp4_full|qtmp4_tables|qtmp4_tables_full"}
    , m_debug_tables_full{                               "qtmp4_tables_full"}
    , m_debug_frame_rate{"qtmp4|qtmp4_full|qtmp4_frame_rate"}
    , m_debug_headers{   "qtmp4|qtmp4_full|qtmp4_headers"}
    , m_debug_editlists{ "qtmp4|qtmp4_full|qtmp4_editlists"}
    , m_debug_indexes{         "qtmp4_full|qtmp4_indexes|qtmp4_indexes_full"}
    , m_debug_indexes_full{                             "qtmp4_indexes_full"}
  {
  }

  ~qtmp4_demuxer_c() {
  }

  void calculate_frame_rate();
  int64_t to_nsecs(int64_t value, std::optional<int64_t> time_scale_to_use = std::nullopt);
  void calculate_timestamps();
  void adjust_timestamps(int64_t delta);

  bool update_tables();
  void apply_edit_list();

  void build_index();

  memory_cptr read_first_bytes(int num_bytes);

  bool is_audio() const;
  bool is_video() const;
  bool is_subtitles() const;
  bool is_chapters() const;
  bool is_unknown() const;

  void handle_stsd_atom(uint64_t atom_size, int level);
  void handle_audio_stsd_atom(uint64_t atom_size, int level);
  void handle_video_stsd_atom(uint64_t atom_size, int level);
  void handle_subtitles_stsd_atom(uint64_t atom_size, int level);
  void handle_colr_atom(memory_cptr const &atom_content, int level);

  void parse_audio_header_priv_atoms(uint64_t atom_size, int level);
  void parse_audio_header_priv_atoms(mm_mem_io_c &mio, uint64_t size, int level);
  void parse_video_header_priv_atoms(uint64_t atom_size, int level);
  void parse_subtitles_header_priv_atoms(uint64_t atom_size, int level);

  void parse_esds_audio_header_priv_atom(mm_io_c &io, int level);
  void parse_dfla_audio_header_priv_atom(mm_io_c &io, int level);
  void parse_dops_audio_header_priv_atom(mm_io_c &io, int level);
  void parse_aac_esds_decoder_config();
  void parse_vorbis_esds_decoder_config();

  bool verify_audio_parameters();
  bool verify_alac_audio_parameters();
  bool verify_dts_audio_parameters();
  bool verify_mp4a_audio_parameters();

  bool verify_video_parameters();
  bool verify_avc_video_parameters();
  bool verify_hevc_video_parameters();
  bool verify_mp4v_video_parameters();

  bool verify_subtitles_parameters();
  bool verify_vobsub_subtitles_parameters();

  void derive_track_params_from_ac3_audio_bitstream();
  bool derive_track_params_from_avc_bitstream();
  void derive_track_params_from_dts_audio_bitstream();
  void derive_track_params_from_mp3_audio_bitstream();
  bool derive_track_params_from_opus_private_data();
  bool derive_track_params_from_vorbis_private_data();
  void check_for_hevc_video_annex_b_bitstream();

  void set_packetizer_display_dimensions();
  void set_packetizer_color_properties();
  void set_packetizer_block_addition_mappings();


  std::optional<int64_t> min_timestamp() const;

  void determine_codec();

  void process(packet_cptr const &packet);

private:
  void build_index_chunk_mode();
  void build_index_constant_sample_size_mode();
  void dump_index_entries(std::string const &message) const;
  void mark_key_frames_from_key_frame_table();
  void mark_open_gop_random_access_points_as_key_frames();

  void calculate_timestamps_constant_sample_size();
  void calculate_timestamps_variable_sample_size();

  bool parse_esds_atom(mm_io_c &io, int level);
  void add_data_as_block_addition(uint32_t atom_type, memory_cptr const &data);
};
using qtmp4_demuxer_cptr = std::shared_ptr<qtmp4_demuxer_c>;

struct qt_atom_t {
  fourcc_c fourcc;
  uint64_t size;
  uint64_t pos;
  uint32_t hsize;

  qt_atom_t()
    : fourcc{}
    , size{}
    , pos{}
    , hsize{}
  {
  }

  qt_atom_t to_parent() const {
    qt_atom_t parent;

    parent.fourcc = fourcc;
    parent.size   = size - hsize;
    parent.pos    = pos  + hsize;
    return parent;
  }
};

inline std::ostream &
operator <<(std::ostream &out,
            qt_atom_t const &atom) {
  out << fmt::format("<{0} at {1} size {2} hsize {3}>", atom.fourcc, atom.pos, atom.size, atom.hsize);
  return out;
}

struct qtmp4_chapter_entry_t {
  std::string m_name;
  int64_t m_timestamp;

  qtmp4_chapter_entry_t()
    : m_timestamp{}
  {
  }

  qtmp4_chapter_entry_t(const std::string &name,
                        int64_t timestamp)
    : m_name{name}
    , m_timestamp{timestamp}
  {
  }

  bool operator <(const qtmp4_chapter_entry_t &cmp) const {
    return m_timestamp < cmp.m_timestamp;
  }
};

class qtmp4_reader_c: public generic_reader_c {
private:
  std::vector<qtmp4_demuxer_cptr> m_demuxers;
  std::unordered_map<unsigned int, bool> m_chapter_track_ids;
  std::unordered_map<unsigned int, qt_track_defaults_t> m_track_defaults;
  std::unique_ptr<libmatroska::KaxTags> m_tags;

  int64_t m_time_scale{1};
  fourcc_c m_compression_algorithm;
  int m_main_dmx{-1};

  unsigned int m_audio_encoder_delay_samples{};

  uint64_t m_moof_offset{}, m_fragment_implicit_offset{};
  qt_fragment_t *m_fragment{};
  qtmp4_demuxer_c *m_track_for_fragment{};

  bool m_timestamps_calculated{};
  std::optional<uint64_t> m_duration;

  int32_t m_display_matrix[3][3];

  uint64_t m_attachment_id{};

  std::string m_encoder, m_comment;

  int64_t m_bytes_to_process{}, m_bytes_processed{};

  debugging_option_c
      m_debug_chapters{    "qtmp4|qtmp4_full|qtmp4_chapters"}
    , m_debug_headers{     "qtmp4|qtmp4_full|qtmp4_headers"}
    , m_debug_tables{            "qtmp4_full|qtmp4_tables|qtmp4_tables_full"}
    , m_debug_tables_full{                               "qtmp4_tables_full"}
    , m_debug_interleaving{"qtmp4|qtmp4_full|qtmp4_interleaving"}
    , m_debug_resync{      "qtmp4|qtmp4_full|qtmp4_resync"};

  friend class qtmp4_demuxer_c;

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::qtmp4;
  }

  virtual void read_headers();
  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
  file_status_e finish();

  virtual void parse_headers();
  virtual void verify_track_parameters_and_update_indexes();
  virtual void calculate_timestamps();
  virtual std::optional<int64_t> calculate_global_min_timestamp() const;
  virtual void calculate_num_bytes_to_process();

  virtual void create_global_tags_from_meta_data();
  virtual void process_global_tags();

  virtual qt_atom_t read_atom(mm_io_c *read_from = nullptr, bool exit_on_error = true);
  virtual bool resync_to_top_level_atom(uint64_t start_pos);
  virtual void parse_itunsmpb(std::string data);

  virtual void handle_cmov_atom(qt_atom_t parent, int level);
  virtual void handle_cmvd_atom(qt_atom_t parent, int level);
  virtual void handle_ctts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sgpd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sbgp_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_dcom_atom(qt_atom_t parent, int level);
  virtual void handle_hdlr_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_mdhd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_mdia_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_minf_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_moov_atom(qt_atom_t parent, int level);
  virtual void handle_mvhd_atom(qt_atom_t parent, int level);
  virtual void handle_udta_atom(qt_atom_t parent, int level);
  virtual void handle_chpl_atom(qt_atom_t parent, int level);
  virtual void handle_meta_atom(qt_atom_t parent, int level);
  virtual void handle_ilst_atom(qt_atom_t parent, int level);
  virtual void handle_4dashes_atom(qt_atom_t parent, int level);
  virtual void handle_covr_atom(qt_atom_t parent, int level);
  virtual void handle_ilst_metadata_atom(qt_atom_t parent, int level, fourcc_c const &fourcc);
  virtual void handle_mvex_atom(qt_atom_t parent, int level);
  virtual void handle_trex_atom(qt_atom_t parent, int level);
  virtual void handle_moof_atom(qt_atom_t parent, int level, qt_atom_t const &moof_atom);
  virtual void handle_traf_atom(qt_atom_t parent, int level);
  virtual void handle_tfhd_atom(qt_atom_t parent, int level);
  virtual void handle_trun_atom(qt_atom_t parent, int level);
  virtual void handle_stbl_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stco_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_co64_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsc_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stss_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stsz_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_sttd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_stts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_tkhd_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_display_matrix(qtmp4_demuxer_c &new_dmx, int level);
  virtual void handle_trak_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_edts_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_elst_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);
  virtual void handle_tref_atom(qtmp4_demuxer_c &new_dmx, qt_atom_t parent, int level);

  virtual memory_cptr create_bitmap_info_header(qtmp4_demuxer_c &dmx, const char *fourcc, size_t extra_size = 0, const void *extra_data = nullptr);

  virtual void create_audio_packetizer_aac(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_ac3(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_alac(qtmp4_demuxer_c &dmx);
  virtual bool create_audio_packetizer_dts(qtmp4_demuxer_c &dmx);
#if defined(HAVE_FLAC_FORMAT_H)
  virtual void create_audio_packetizer_flac(qtmp4_demuxer_c &dmx);
#endif
  virtual void create_audio_packetizer_mp3(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_opus(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_passthrough(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_pcm(qtmp4_demuxer_c &dmx);
  virtual void create_audio_packetizer_vorbis(qtmp4_demuxer_c &dmx);

  virtual void create_video_packetizer_av1(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_avc(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpeg1_2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpeg4_p2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpegh_p2(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_mpegh_p2_es(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_standard(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_svq1(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_prores(qtmp4_demuxer_c &dmx);
  virtual void create_video_packetizer_vpx(qtmp4_demuxer_c &dmx);

  virtual void create_subtitles_packetizer_timed_text(qtmp4_demuxer_c &dmx);
  virtual void create_subtitles_packetizer_vobsub(qtmp4_demuxer_c &dmx);

  virtual void handle_audio_encoder_delay(qtmp4_demuxer_c &dmx);

  virtual mtx::bcp47::language_c decode_and_verify_language(uint16_t coded_language);
  virtual void read_chapter_track();
  virtual void recode_chapter_entries(std::vector<qtmp4_chapter_entry_t> &entries);
  virtual void process_chapter_entries(int level, std::vector<qtmp4_chapter_entry_t> &entries);

  virtual void detect_interleaving();

  virtual std::string read_string_atom(qt_atom_t atom, size_t num_skipped);

  virtual void process_atom(qt_atom_t const &parent, int level, std::function<void(qt_atom_t const &)> const &handler);
};

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<qt_atom_t> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
