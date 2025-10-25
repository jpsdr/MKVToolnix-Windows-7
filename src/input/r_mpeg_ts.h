/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the MPEG TS demultiplexer module

   Written by Massimo Callegari <massimocallegari@yahoo.it>
   and Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/aac.h"
#include "common/avc/es_parser.h"
#include "common/byte_buffer.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/dts.h"
#include "common/hevc/util.h"
#include "common/truehd.h"
#include "common/vc1_fwd.h"
#include "input/packet_converter.h"
#include "input/teletext_to_srt_packet_converter.h"
#include "merge/generic_reader.h"
#include "mpegparser/M2VParser.h"

namespace mtx::bluray::clpi {
class parser_c;
}

namespace mtx::mpeg_ts {

enum class processing_state_e {
  probing,
  determining_timestamp_offset,
  muxing,
};

enum class pid_type_e {
  pat,
  pmt,
  sdt,
  video,
  audio,
  subtitles,
  unknown,
};

enum class stream_type_e : uint8_t {
  iso_11172_video              = 0x01, // ISO/IEC 11172 Video
  iso_13818_video              = 0x02, // ISO/IEC 13818-2 Video
  iso_11172_audio              = 0x03, // ISO/IEC 11172 Audio
  iso_13818_audio              = 0x04, // ISO/IEC 13818-3 Audio
  iso_13818_private            = 0x05, // ISO/IEC 13818-1 private sections
  iso_13818_pes_private        = 0x06, // ISO/IEC 13818-1 PES packets containing private data
  iso_13522_mheg               = 0x07, // ISO/IEC 13512 MHEG
  iso_13818_dsmcc              = 0x08, // ISO/IEC 13818-1 Annex A  DSM CC
  iso_13818_type_a             = 0x0a, // ISO/IEC 13818-6 Multiprotocol encapsulation
  iso_13818_type_b             = 0x0b, // ISO/IEC 13818-6 DSM-CC U-N Messages
  iso_13818_type_c             = 0x0c, // ISO/IEC 13818-6 Stream Descriptors
  iso_13818_type_d             = 0x0d, // ISO/IEC 13818-6 Sections (any type, including private data)
  iso_13818_aux                = 0x0e, // ISO/IEC 13818-1 auxiliary
  iso_13818_part7_audio        = 0x0f, // ISO/IEC 13818-7 Audio with ADTS transport sytax
  iso_14496_part2_video        = 0x10, // ISO/IEC 14496-2 Visual (MPEG-4)
  iso_14496_part3_audio        = 0x11, // ISO/IEC 14496-3 Audio with LATM transport syntax
  iso_14496_part10_video       = 0x1b, // ISO/IEC 14496-10 Video (MPEG-4 part 10/AVC, aka H.264)
  iso_23008_part2_video        = 0x24, // ISO/IEC 14496-10 Video (MPEG-H part 2/HEVC, aka H.265)
  stream_audio_pcm             = 0x80, // PCM
  stream_audio_ac3             = 0x81, // Audio AC-3 (A/52)
  stream_audio_dts             = 0x82, // Audio DTS
  stream_audio_ac3_lossless    = 0x83, // Audio AC-3 - Dolby lossless
  stream_audio_eac3            = 0x84, // Audio AC-3 - Dolby Digital Plus (E-AC-3)
  stream_audio_dts_hd          = 0x85, // Audio DTS HD
  stream_audio_dts_hd_ma       = 0x86, // Audio DTS HD Master Audio
  stream_audio_eac3_atsc       = 0x87, // Audio AC-3 - Dolby Digital Plus (E-AC-3) as defined in ATSC A/52:2012 Annex G
  stream_audio_eac3_2          = 0xa1, // Audio AC-3 - Dolby Digital Plus (E-AC-3); secondary stream
  stream_audio_dts_hd2         = 0xa2, // Audio DTS HD Express; secondary stream
  stream_video_vc1             = 0xea, // Video VC-1
  stream_subtitles_hdmv_pgs    = 0x90, // HDMV PGS subtitles
  stream_subtitles_hdmv_textst = 0x92, // HDMV TextST subtitles
};

enum class drop_decision_e {
  keep,
  drop,
};

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif

// TS packet header
struct PACKED_STRUCTURE packet_header_t {
  uint8_t sync_byte;
  uint8_t pid_msb_flags1; // 0x80:transport_error_indicator 0x40:payload_unit_start_indicator 0x20:transport_priority 0x1f:pid_msb
  uint8_t pid_lsb;
  uint8_t flags2;         // 0xc0:transport_scrambling_control 0x20:adaptation_field_flag 0x10:payload_flag 0x0f:continuity_counter

  uint16_t get_pid() const {
    return ((static_cast<uint16_t>(pid_msb_flags1) & 0x1f) << 8) | static_cast<uint16_t>(pid_lsb);
  }

  bool is_payload_unit_start() const {
    return (pid_msb_flags1 & 0x40) == 0x40;
  }

  bool has_transport_error() const {
    return (pid_msb_flags1 & 0x80) == 0x80;
  }

  bool has_adaptation_field() const {
    return (flags2 & 0x20) == 0x20;
  }

  bool has_payload() const {
    return (flags2 & 0x10) == 0x10;
  }

  uint8_t continuity_counter() const {
    return flags2 & 0x0f;
  }
};

// PAT header
struct PACKED_STRUCTURE pat_t {
  uint8_t table_id;
  uint8_t section_length_msb_flags1; // 0x80:section_syntax_indicator 0x40:zero 0x30:reserved 0x0f:section_length_msb
  uint8_t section_length_lsb;
  uint8_t transport_stream_id_msb;
  uint8_t transport_stream_id_lsb;
  uint8_t flags2;                    // 0xc0:reserved2 0x3e:version 0x01:current_next_indicator
  uint8_t section_number;
  uint8_t last_section_number;

  uint16_t get_section_length() const {
    return ((static_cast<uint16_t>(section_length_msb_flags1) & 0x0f) << 8) | static_cast<uint16_t>(section_length_lsb);
  }

  uint8_t get_section_syntax_indicator() const {
    return (section_length_msb_flags1 & 0x80) >> 7;
  }

  uint8_t get_current_next_indicator() const {
    return flags2 & 0x01;
  }
};

// PAT section
struct PACKED_STRUCTURE pat_section_t {
  uint16_t program_number;
  uint8_t pid_msb_flags;
  uint8_t pid_lsb;

  uint16_t get_pid() const {
    return ((static_cast<uint16_t>(pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pid_lsb);
  }

  uint16_t get_program_number() const {
    return get_uint16_be(&program_number);
  }
};

// PMT header
struct PACKED_STRUCTURE pmt_t {
  uint8_t table_id;
  uint8_t section_length_msb_flags1; // 0x80:section_syntax_indicator 0x40:zero 0x30:reserved 0x0f:section_length_msb
  uint8_t section_length_lsb;
  uint16_t program_number;
  uint8_t flags2;                    // 0xc0:reserved2 0x3e:version_number 0x01:current_next_indicator
  uint8_t section_number;
  uint8_t last_section_number;
  uint8_t pcr_pid_msb_flags;
  uint8_t pcr_pid_lsb;
  uint8_t program_info_length_msb_reserved; // 0xf0:reserved4 0x0f:program_info_length_msb
  uint8_t program_info_length_lsb;

  uint16_t get_pcr_pid() const {
    return ((static_cast<uint16_t>(pcr_pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pcr_pid_lsb);
  }

  uint16_t get_section_length() const {
    return ((static_cast<uint16_t>(section_length_msb_flags1) & 0x0f) << 8) | static_cast<uint16_t>(section_length_lsb);
  }

  uint16_t get_program_info_length() const {
    return ((static_cast<uint16_t>(program_info_length_msb_reserved) & 0x0f) << 8) | static_cast<uint16_t>(program_info_length_lsb);
  }

  uint8_t get_section_syntax_indicator() const {
    return (section_length_msb_flags1 & 0x80) >> 7;
  }

  uint8_t get_current_next_indicator() const {
    return flags2 & 0x01;
  }

  uint16_t get_program_number() const {
    return get_uint16_be(&program_number);
  }
};

// PMT descriptor
struct PACKED_STRUCTURE pmt_descriptor_t {
  uint8_t tag;
  uint8_t length;
};

// PMT pid info
struct PACKED_STRUCTURE pmt_pid_info_t {
  stream_type_e stream_type;
  uint8_t pid_msb_flags;
  uint8_t pid_lsb;
  uint8_t es_info_length_msb_flags;
  uint8_t es_info_length_lsb;

  uint16_t get_pid() const {
    return ((static_cast<uint16_t>(pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pid_lsb);
  }

  uint16_t get_es_info_length() const {
    return ((static_cast<uint16_t>(es_info_length_msb_flags) & 0x0f) << 8) | static_cast<uint16_t>(es_info_length_lsb);
  }
};

// PES header
struct PACKED_STRUCTURE pes_header_t {
  uint8_t packet_start_code[3];
  uint8_t stream_id;
  uint16_t pes_packet_length;
  uint8_t flags1; // 0xc0:onezero 0x30:pes_scrambling_control 0x08:pes_priority 0x04:data_alignment_indicator  0x02:copyright 0x01:original_or_copy
  uint8_t flags2; // 0xc0:pts_dts_flags 0x20:escr 0x10:es_rate 0x08:dsm_trick_mode 0x04:additional_copy_info 0x02:pes_crc 0x01:pes_extension
  uint8_t pes_header_data_length;
  uint8_t pts_dts;

  uint16_t get_pes_packet_length() const {
    return get_uint16_be(&pes_packet_length);
  }

  uint8_t get_pes_extension() const{
    return flags2 & 0x01;
  }

  uint8_t get_pes_crc() const {
    return (flags2 & 0x02) >> 1;
  }

  uint8_t get_additional_copy_info() const{
    return (flags2 & 0x04) >> 2;
  }

  uint8_t get_dsm_trick_mode() const {
    return (flags2 & 0x08) >> 3;
  }

  uint8_t get_es_rate() const {
    return (flags2 & 0x10) >> 4;
  }

  uint8_t get_escr() const {
    return (flags2 & 0x20) >> 5;
  }

  uint8_t get_pts_dts_flags() const {
    return (flags2 & 0xc0) >> 6;
  }
};

#if defined(COMP_MSC)
#pragma pack(pop)
#endif

struct program_t {
  uint16_t program_number;
  std::string service_provider, service_name;

  bool operator<(program_t const &other) const {
    return program_number < other.program_number;
  };
};

class reader_c;

class track_c;
using track_ptr = std::shared_ptr<track_c>;

class track_c {
public:
  reader_c &reader;
  std::size_t m_file_num;
  uint64_t m_id;

  bool processed, m_skip_pes_payload{};
  pid_type_e type;
  codec_c codec;
  uint16_t pid;
  std::optional<uint16_t> program_number, m_dovi_base_layer_pid;
  std::optional<int> m_ttx_wanted_page;
  std::unordered_map<unsigned int, bool> m_ttx_known_subtitle_pages, m_ttx_known_non_subtitle_pages;
  std::optional<uint8_t> m_expected_next_continuity_counter;
  std::optional<bool> m_hearing_impaired_flag;
  std::size_t pes_payload_size_to_read; // size of the current PID payload in bytes
  mtx::bytes::buffer_cptr pes_payload_read;    // buffer with the current PID payload

  bool probed_ok, m_hidden{};
  int ptzr;                         // the actual packetizer instance

  timestamp_c m_timestamp, m_previous_timestamp, m_previous_valid_timestamp, m_timestamp_wrap_add, m_subtitle_timestamp_correction;
  mtx_mp_rational_t m_default_duration;

  // video related parameters
  bool v_interlaced;
  int v_version, v_width, v_height, v_dwidth, v_dheight;
  mtx_mp_rational_t v_aspect_ratio;
  memory_cptr m_codec_private_data;
  std::optional<mtx::dovi::dovi_decoder_configuration_record_t> m_dovi_config;
  track_c *m_dovi_el_track{};

  // audio related parameters
  int a_channels, a_sample_rate, a_bits_per_sample, a_bsid;
  mtx::dts::header_t a_dts_header;
  aac::frame_c m_aac_frame;
  aac::parser_c::multiplex_type_e m_aac_multiplex_type;

  bool m_apply_dts_timestamp_fix, m_use_dts, m_timestamps_wrapped, m_truehd_found_truehd, m_truehd_found_ac3;
  std::vector<track_ptr> m_coupled_tracks;
  track_c *m_master;

  // general track parameters
  mtx::bcp47::language_c language;

  // used for probing for stream types
  mtx::bytes::buffer_cptr m_probe_data;
  mtx::avc::es_parser_cptr m_avc_parser;
  mtx::hevc::es_parser_cptr m_hevc_parser;
  mtx::truehd::parser_cptr m_truehd_parser;
  std::shared_ptr<M2VParser> m_m2v_parser;
  mtx::vc1::es_parser_cptr m_vc1_parser;
  std::shared_ptr<teletext_to_srt_packet_converter_c> m_ttx_parser;

  unsigned int skip_packet_data_bytes;

  packet_converter_cptr converter;

  bool m_debug_delivery, m_debug_timestamp_wrapping;
  debugging_option_c m_debug_headers;

  track_c(reader_c &p_reader, pid_type_e p_type = pid_type_e::unknown);

  void send_to_packetizer();
  void add_pes_payload(uint8_t *ts_payload, size_t ts_payload_size);
  void add_pes_payload_to_probe_data();
  void clear_pes_payload();
  bool is_pes_payload_complete() const;
  bool is_pes_payload_size_unbounded() const;
  std::size_t remaining_payload_size_to_read() const;

  int new_stream_v_mpeg_1_2(bool end_of_detection);
  int new_stream_v_avc(bool end_of_detection);
  int new_stream_v_hevc(bool end_of_detection);
  int new_stream_v_vc1();
  int new_stream_a_mpeg();
  int new_stream_a_aac();
  int new_stream_a_ac3();
  int new_stream_a_dts();
  int new_stream_a_pcm();
  int new_stream_a_truehd();
  int new_stream_s_hdmv_textst();
  int new_stream_s_dvbsub();
  int new_stream_s_teletext();

  bool parse_ac3_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);
  bool parse_dovi_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);
  bool parse_dts_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);
  bool parse_registration_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);
  bool parse_teletext_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);
  bool parse_subtitling_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor, pmt_pid_info_t const &pmt_pid_info);

  bool has_packetizer() const;
  bool transport_error_detected(packet_header_t &ts_header) const;

  void set_pid(uint16_t new_pid);

  drop_decision_e handle_bogus_subtitle_timestamps(timestamp_c &pts, timestamp_c &dts);
  void handle_timestamp_wrap(timestamp_c &pts, timestamp_c &dts);
  bool detect_timestamp_wrap(timestamp_c const &timestamp) const;
  void adjust_timestamp_for_wrap(timestamp_c &timestamp);

  timestamp_c derive_pts_from_content();
  timestamp_c derive_hdmv_textst_pts_from_content();

  void determine_codec_from_stream_type(stream_type_e stream_type);
  codec_c determine_codec_for_hdmv_registration_descriptor(pmt_descriptor_t const &pmt_descriptor);

  bool contains_dovi_base_layer_for_enhancement_layer(track_c const &el_track) const;

  void process(packet_cptr const &packet);

  void parse_iso639_language_from(void const *buffer);

  void reset_processing_state();

  void set_packetizer_source_id() const;

  track_ptr set_up_teletext_track(int ttx_page, int ttx_type = 0x02);
  void handle_probed_teletext_pages(std::vector<track_ptr> &newly_created_tracks);
};

struct file_t {
  mm_io_cptr m_in;

  unsigned int m_header_offset{};

  std::unordered_map<uint16_t, track_ptr> m_pid_to_track_map;
  std::unordered_map<uint16_t, bool> m_ignored_pids, m_pmt_pid_seen;
  std::vector<generic_packetizer_c *> m_packetizers;
  std::vector<program_t> m_programs;

  bool m_pat_found;
  unsigned int m_num_pmts_found, m_num_pmts_to_find;
  int m_es_to_process;
  timestamp_c m_global_timestamp_offset, m_stream_timestamp, m_timestamp_restriction_min, m_timestamp_restriction_max, m_timestamp_mpls_sync, m_last_non_subtitle_pts, m_last_non_subtitle_dts;
  bool m_timestamp_restriction_min_seen{};

  processing_state_e m_state;
  uint64_t m_probe_range, m_position{}, m_start_source_packet_number{};

  bool m_file_done, m_packet_sent_to_packetizer;

  unsigned int m_detected_packet_size, m_num_pat_crc_errors, m_num_pmt_crc_errors;
  bool m_validate_pat_crc, m_validate_pmt_crc, m_has_audio_or_video_track;

  std::shared_ptr<mtx::bluray::clpi::parser_c> m_clpi_parser;

  file_t(mm_io_cptr const &in);

  int64_t get_queued_bytes() const;
  void reset_processing_state(processing_state_e new_state);
  bool all_pmts_found() const;
  uint64_t get_start_source_packet_position() const;
};
using file_cptr = std::shared_ptr<file_t>;

class reader_c: public generic_reader_c {
protected:
  bool m_is_reading_mpls{};
  std::vector<file_cptr> m_files;
  std::size_t m_current_file{}, m_packet_num{};

  std::vector<track_ptr> m_tracks, m_all_probed_tracks;
  std::map<generic_packetizer_c *, track_ptr> m_ptzr_to_track_map;

  mtx::bluray::mpls::chapters_t m_mpls_chapters;

  int64_t m_bytes_to_process{}, m_bytes_processed{};

  debugging_option_c
      m_dont_use_audio_pts{      "mpeg_ts|mpeg_ts_dont_use_audio_pts"}
    , m_debug_resync{            "mpeg_ts|mpeg_ts_resync"}
    , m_debug_pat_pmt{           "mpeg_ts|mpeg_ts_pat|mpeg_ts_headers|mpeg_ts_pmt"}
    , m_debug_sdt{               "mpeg_ts|mpeg_ts_sdt|mpeg_ts_headers"}
    , m_debug_headers{           "mpeg_ts|mpeg_ts_headers"}
    , m_debug_pes_headers{       "mpeg_ts|mpeg_ts_headers|mpeg_ts_pes_headers"}
    , m_debug_packet{            "mpeg_ts|mpeg_ts_packet"}
    , m_debug_aac{               "mpeg_ts|mpeg_ts_aac"}
    , m_debug_timestamp_wrapping{"mpeg_ts|mpeg_ts_timestamp_wrapping"}
    , m_debug_clpi{              "mpeg_ts|mpeg_ts_clpi|clpi"}
    , m_debug_mpls{              "mpeg_ts|mpeg_ts_mpls|mpls"}
    , m_debug_timestamp_offset{  "mpeg_ts|mpeg_ts_headers|mpeg_ts_timestamp_offset|mpeg_ts_timestamp_offsets"}
    , m_debug_dovi{              "mpeg_ts|mpeg_ts_dovi|dovi"};

protected:
  static int potential_packet_sizes[];

public:
  virtual bool probe_file() override;

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::mpeg_ts;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t tid);
  virtual void create_packetizers();
  virtual void add_available_track_ids();

  virtual void parse_packet(uint8_t *buf);

  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;

  static timestamp_c read_timestamp(uint8_t *p);
  static std::optional<std::pair<unsigned int, unsigned int>> detect_packet_size(mm_io_c &in, uint64_t size);

private:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  void read_headers_for_file(std::size_t file_num);

  track_ptr find_track_for_pid(uint16_t pid) const;
  std::pair<uint8_t *, std::size_t> determine_ts_payload_start(packet_header_t *hdr) const;
  void setup_initial_tracks();

  void handle_ts_payload(track_c &track, packet_header_t &ts_header, uint8_t *ts_payload, std::size_t ts_payload_size);
  void handle_pat_pmt_payload(track_c &track, packet_header_t &ts_header, uint8_t *ts_payload, std::size_t ts_payload_size);
  void handle_pes_payload(track_c &track, packet_header_t &ts_header, uint8_t *ts_payload, std::size_t ts_payload_size);
  drop_decision_e handle_transport_errors(track_c &track, packet_header_t &ts_header);
  track_ptr handle_packet_for_pid_not_listed_in_pmt(uint16_t pid);

  bool parse_pat(track_c &track);
  bool parse_pmt(track_c &track);
  bool parse_pmt_pid_info(mm_mem_io_c &mem, uint16_t program_number);
  bool parse_sdt(track_c &track);
  void parse_sdt_service_desciptor(mtx::bits::reader_c &r, uint16_t program_number);
  void parse_pes(track_c &track);
  void probe_packet_complete(track_c &track, bool end_of_detection = false);
  int determine_track_parameters(track_c &track, bool end_of_detection);
  void determine_track_type_by_pes_content(track_c &track);
  void pair_dovi_base_and_enhancement_layer_tracks();
  void handle_probed_teletext_pages();

  file_status_e finish();
  bool all_files_done() const;
  int send_to_packetizer(track_ptr &track);
  void create_mpeg1_2_video_packetizer(track_ptr &track);
  void create_mpeg4_p10_es_video_packetizer(track_ptr &track);
  void create_mpegh_p2_es_video_packetizer(track_ptr &track);
  void create_vc1_video_packetizer(track_ptr &track);
  void create_aac_audio_packetizer(track_ptr const &track);
  void create_ac3_audio_packetizer(track_ptr const &track);
  void create_pcm_audio_packetizer(track_ptr const &track);
  void create_truehd_audio_packetizer(track_ptr const &track);
  void create_hdmv_pgs_subtitles_packetizer(track_ptr &track);
  void create_hdmv_textst_subtitles_packetizer(track_ptr const &track);
  void create_srt_subtitles_packetizer(track_ptr const &track);
  void create_dvbsub_subtitles_packetizer(track_ptr const &track);

  void reset_processing_state(processing_state_e new_state);
  void determine_global_timestamp_offset();

  void parse_clip_info_file(std::size_t file_idx);
  void determine_start_source_packet_number(file_t &file);

  void add_external_files_from_mpls(mm_mpls_multi_file_io_c &mpls_in);
  void add_programs_to_identification_info(mtx::id::info_c &info);

  void process_chapter_entries();

  bool resync(int64_t start_at);

  uint32_t calculate_crc(void const *buffer, size_t size) const;

  file_t &file();

  void add_multiplexed_ids(std::vector<uint64_t> &multiplexed_ids, track_c &track);

  static memory_cptr read_pmt_descriptor(mm_io_c &io);
  static std::string read_descriptor_string(mtx::bits::reader_c &r);
  static charset_converter_cptr get_charset_converter_for_coding_type(unsigned int coding);

  friend class track_c;
};

}
