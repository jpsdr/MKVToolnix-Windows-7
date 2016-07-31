/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definitions for the MPEG TS demultiplexer module

   Written by Massimo Callegari <massimocallegari@yahoo.it>
   and Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_R_MPEG_TS_H
#define MTX_R_MPEG_TS_H

#include "common/common_pch.h"

#include "common/aac.h"
#include "common/byte_buffer.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/dts.h"
#include "common/hevc.h"
#include "common/mm_io.h"
#include "common/mpeg4_p10.h"
#include "common/truehd.h"
#include "input/packet_converter.h"
#include "merge/generic_reader.h"
#include "mpegparser/M2VParser.h"

namespace vc1 {
class es_parser_c;
}

enum mpeg_ts_pid_type_e {
  PAT_TYPE      = 0,
  PMT_TYPE      = 1,
  ES_VIDEO_TYPE = 2,
  ES_AUDIO_TYPE = 3,
  ES_SUBT_TYPE  = 4,
  ES_UNKNOWN    = 5,
};

enum mpeg_ts_stream_type_e {
  ISO_11172_VIDEO           = 0x01, // ISO/IEC 11172 Video
  ISO_13818_VIDEO           = 0x02, // ISO/IEC 13818-2 Video
  ISO_11172_AUDIO           = 0x03, // ISO/IEC 11172 Audio
  ISO_13818_AUDIO           = 0x04, // ISO/IEC 13818-3 Audio
  ISO_13818_PRIVATE         = 0x05, // ISO/IEC 13818-1 private sections
  ISO_13818_PES_PRIVATE     = 0x06, // ISO/IEC 13818-1 PES packets containing private data
  ISO_13522_MHEG            = 0x07, // ISO/IEC 13512 MHEG
  ISO_13818_DSMCC           = 0x08, // ISO/IEC 13818-1 Annex A  DSM CC
  ISO_13818_TYPE_A          = 0x0a, // ISO/IEC 13818-6 Multiprotocol encapsulation
  ISO_13818_TYPE_B          = 0x0b, // ISO/IEC 13818-6 DSM-CC U-N Messages
  ISO_13818_TYPE_C          = 0x0c, // ISO/IEC 13818-6 Stream Descriptors
  ISO_13818_TYPE_D          = 0x0d, // ISO/IEC 13818-6 Sections (any type, including private data)
  ISO_13818_AUX             = 0x0e, // ISO/IEC 13818-1 auxiliary
  ISO_13818_PART7_AUDIO     = 0x0f, // ISO/IEC 13818-7 Audio with ADTS transport sytax
  ISO_14496_PART2_VIDEO     = 0x10, // ISO/IEC 14496-2 Visual (MPEG-4)
  ISO_14496_PART3_AUDIO     = 0x11, // ISO/IEC 14496-3 Audio with LATM transport syntax
  ISO_14496_PART10_VIDEO    = 0x1b, // ISO/IEC 14496-10 Video (MPEG-4 part 10/AVC, aka H.264)
  ISO_23008_PART2_VIDEO     = 0x24, // ISO/IEC 14496-10 Video (MPEG-H part 2/HEVC, aka H.265)
  STREAM_AUDIO_PCM          = 0x80, // PCM
  STREAM_AUDIO_AC3          = 0x81, // Audio AC-3 (A/52)
  STREAM_AUDIO_DTS          = 0x82, // Audio DTS
  STREAM_AUDIO_AC3_LOSSLESS = 0x83, // Audio AC-3 - Dolby lossless
  STREAM_AUDIO_EAC3         = 0x84, // Audio AC-3 - Dolby Digital Plus (E-AC-3)
  STREAM_AUDIO_DTS_HD       = 0x85, // Audio DTS HD
  STREAM_AUDIO_DTS_HD_MA    = 0x86, // Audio DTS HD Master Audio
  STREAM_AUDIO_EAC3_ATSC    = 0x87, // Audio AC-3 - Dolby Digital Plus (E-AC-3) as defined in ATSC A/52:2012 Annex G
  STREAM_AUDIO_EAC3_2       = 0xa1, // Audio AC-3 - Dolby Digital Plus (E-AC-3); secondary stream
  STREAM_AUDIO_DTS_HD2      = 0xa2, // Audio DTS HD Express; secondary stream
  STREAM_VIDEO_VC1          = 0xEA, // Video VC-1
  STREAM_SUBTITLES_HDMV_PGS = 0x90, // HDMV PGS subtitles
};

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif

// TS packet header
struct PACKED_STRUCTURE mpeg_ts_packet_header_t {
  unsigned char sync_byte;
  unsigned char pid_msb_flags1; // 0x80:transport_error_indicator 0x40:payload_unit_start_indicator 0x20:transport_priority 0x1f:pid_msb
  unsigned char pid_lsb;
  unsigned char flags2;         // 0xc0:transport_scrambling_control 0x20:adaptation_field_flag 0x10:payload_flag 0x0f:continuity_counter

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
};

// PAT header
struct PACKED_STRUCTURE mpeg_ts_pat_t {
  unsigned char table_id;
  unsigned char section_length_msb_flags1; // 0x80:section_syntax_indicator 0x40:zero 0x30:reserved 0x0f:section_length_msb
  unsigned char section_length_lsb;
  unsigned char transport_stream_id_msb;
  unsigned char transport_stream_id_lsb;
  unsigned char flags2;                    // 0xc0:reserved2 0x3e:version 0x01:current_next_indicator
  unsigned char section_number;
  unsigned char last_section_number;

  uint16_t get_section_length() const {
    return ((static_cast<uint16_t>(section_length_msb_flags1) & 0x0f) << 8) | static_cast<uint16_t>(section_length_lsb);
  }

  unsigned char get_section_syntax_indicator() const {
    return (section_length_msb_flags1 & 0x80) >> 7;
  }

  unsigned char get_current_next_indicator() const {
    return flags2 & 0x01;
  }
};

// PAT section
struct PACKED_STRUCTURE mpeg_ts_pat_section_t {
  uint16_t program_number;
  unsigned char pid_msb_flags;
  unsigned char pid_lsb;

  uint16_t get_pid() const {
    return ((static_cast<uint16_t>(pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pid_lsb);
  }

  uint16_t get_program_number() const {
    return get_uint16_be(&program_number);
  }
};

// PMT header
struct PACKED_STRUCTURE mpeg_ts_pmt_t {
  unsigned char table_id;
  unsigned char section_length_msb_flags1; // 0x80:section_syntax_indicator 0x40:zero 0x30:reserved 0x0f:section_length_msb
  unsigned char section_length_lsb;
  uint16_t program_number;
  unsigned char flags2;                    // 0xc0:reserved2 0x3e:version_number 0x01:current_next_indicator
  unsigned char section_number;
  unsigned char last_section_number;
  unsigned char pcr_pid_msb_flags;
  unsigned char pcr_pid_lsb;
  unsigned char program_info_length_msb_reserved; // 0xf0:reserved4 0x0f:program_info_length_msb
  unsigned char program_info_length_lsb;

  uint16_t get_pcr_pid() const {
    return ((static_cast<uint16_t>(pcr_pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pcr_pid_lsb);
  }

  uint16_t get_section_length() const {
    return ((static_cast<uint16_t>(section_length_msb_flags1) & 0x0f) << 8) | static_cast<uint16_t>(section_length_lsb);
  }

  uint16_t get_program_info_length() const {
    return ((static_cast<uint16_t>(program_info_length_msb_reserved) & 0x0f) << 8) | static_cast<uint16_t>(program_info_length_lsb);
  }

  unsigned char get_section_syntax_indicator() const {
    return (section_length_msb_flags1 & 0x80) >> 7;
  }

  unsigned char get_current_next_indicator() const {
    return flags2 & 0x01;
  }

  uint16_t get_program_number() const {
    return get_uint16_be(&program_number);
  }
};

// PMT descriptor
struct PACKED_STRUCTURE mpeg_ts_pmt_descriptor_t {
  unsigned char tag;
  unsigned char length;
};

// PMT pid info
struct PACKED_STRUCTURE mpeg_ts_pmt_pid_info_t {
  unsigned char stream_type;
  unsigned char pid_msb_flags;
  unsigned char pid_lsb;
  unsigned char es_info_length_msb_flags;
  unsigned char es_info_length_lsb;

  uint16_t get_pid() const {
    return ((static_cast<uint16_t>(pid_msb_flags) & 0x1f) << 8) | static_cast<uint16_t>(pid_lsb);
  }

  uint16_t get_es_info_length() const {
    return ((static_cast<uint16_t>(es_info_length_msb_flags) & 0x0f) << 8) | static_cast<uint16_t>(es_info_length_lsb);
  }
};

// PES header
struct PACKED_STRUCTURE mpeg_ts_pes_header_t {
  unsigned char packet_start_code[3];
  unsigned char stream_id;
  uint16_t pes_packet_length;
  unsigned char flags1; // 0xc0:onezero 0x30:pes_scrambling_control 0x08:pes_priority 0x04:data_alignment_indicator  0x02:copyright 0x01:original_or_copy
  unsigned char flags2; // 0xc0:pts_dts_flags 0x20:escr 0x10:es_rate 0x08:dsm_trick_mode 0x04:additional_copy_info 0x02:pes_crc 0x01:pes_extension
  unsigned char pes_header_data_length;
  unsigned char pts_dts;

  uint16_t get_pes_packet_length() const {
    return get_uint16_be(&pes_packet_length);
  }

  unsigned char get_pes_extension() const{
    return flags2 & 0x01;
  }

  unsigned char get_pes_crc() const {
    return (flags2 & 0x02) >> 1;
  }

  unsigned char get_additional_copy_info() const{
    return (flags2 & 0x04) >> 2;
  }

  unsigned char get_dsm_trick_mode() const {
    return (flags2 & 0x08) >> 3;
  }

  unsigned char get_es_rate() const {
    return (flags2 & 0x10) >> 4;
  }

  unsigned char get_escr() const {
    return (flags2 & 0x20) >> 5;
  }

  unsigned char get_pts_dts_flags() const {
    return (flags2 & 0xc0) >> 6;
  }
};

#if defined(COMP_MSC)
#pragma pack(pop)
#endif

class mpeg_ts_reader_c;

class mpeg_ts_track_c;
using mpeg_ts_track_ptr = std::shared_ptr<mpeg_ts_track_c>;

class mpeg_ts_track_c {
public:
  mpeg_ts_reader_c &reader;

  bool processed;
  mpeg_ts_pid_type_e type;          //can be PAT_TYPE, PMT_TYPE, ES_VIDEO_TYPE, ES_AUDIO_TYPE, ES_SUBT_TYPE, ES_UNKNOWN
  codec_c codec;
  uint16_t pid;
  boost::optional<int> m_ttx_wanted_page;
  std::size_t pes_payload_size_to_read; // size of the current PID payload in bytes
  byte_buffer_cptr pes_payload_read;    // buffer with the current PID payload

  bool probed_ok;
  int ptzr;                         // the actual packetizer instance

  timestamp_c m_timestamp, m_previous_timestamp, m_previous_valid_timestamp, m_timestamp_wrap_add;

  // video related parameters
  bool v_interlaced;
  int v_version, v_width, v_height, v_dwidth, v_dheight;
  double v_frame_rate, v_aspect_ratio;
  memory_cptr raw_seq_hdr;

  // audio related parameters
  int a_channels, a_sample_rate, a_bits_per_sample, a_bsid;
  mtx::dts::header_t a_dts_header;
  aac::frame_c m_aac_frame;

  bool m_apply_dts_timestamp_fix, m_use_dts, m_timestamps_wrapped, m_truehd_found_truehd, m_truehd_found_ac3;
  std::vector<mpeg_ts_track_ptr> m_coupled_tracks;

  // general track parameters
  std::string language;

  // used for probing for stream types
  byte_buffer_cptr m_probe_data;
  mpeg4::p10::avc_es_parser_cptr m_avc_parser;
  mtx::hevc::es_parser_cptr m_hevc_parser;
  truehd_parser_cptr m_truehd_parser;
  std::shared_ptr<M2VParser> m_m2v_parser;
  std::shared_ptr<vc1::es_parser_c> m_vc1_parser;

  unsigned int skip_packet_data_bytes;

  packet_converter_cptr converter;

  bool m_debug_delivery, m_debug_timestamp_wrapping;
  debugging_option_c m_debug_headers;

  mpeg_ts_track_c(mpeg_ts_reader_c &p_reader)
    : reader(p_reader)
    , processed(false)
    , type(ES_UNKNOWN)
    , pid(0)
    , pes_payload_size_to_read{}
    , pes_payload_read(new byte_buffer_c)
    , probed_ok(false)
    , ptzr(-1)
    , m_timestamp_wrap_add{timestamp_c::ns(0)}
    , v_interlaced(false)
    , v_version(0)
    , v_width(0)
    , v_height(0)
    , v_dwidth(0)
    , v_dheight(0)
    , v_frame_rate(0)
    , v_aspect_ratio(0)
    , a_channels(0)
    , a_sample_rate(0)
    , a_bits_per_sample(0)
    , a_bsid(0)
    , m_apply_dts_timestamp_fix(false)
    , m_use_dts(false)
    , m_timestamps_wrapped{false}
    , m_truehd_found_truehd{}
    , m_truehd_found_ac3{}
    , skip_packet_data_bytes{}
    , m_debug_delivery{}
    , m_debug_timestamp_wrapping{}
    , m_debug_headers{"mpeg_ts|mpeg_ts_headers"}
  {
  }

  void send_to_packetizer();
  void add_pes_payload(unsigned char *ts_payload, size_t ts_payload_size);
  void add_pes_payload_to_probe_data();
  void clear_pes_payload();
  bool is_pes_payload_complete() const;
  bool is_pes_payload_size_unbounded() const;
  std::size_t remaining_payload_size_to_read() const;

  int new_stream_v_mpeg_1_2();
  int new_stream_v_avc();
  int new_stream_v_hevc();
  int new_stream_v_vc1();
  int new_stream_a_mpeg();
  int new_stream_a_aac();
  int new_stream_a_ac3();
  int new_stream_a_dts();
  int new_stream_a_pcm();
  int new_stream_a_truehd();

  bool parse_ac3_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor, mpeg_ts_pmt_pid_info_t const &pmt_pid_info);
  bool parse_dts_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor, mpeg_ts_pmt_pid_info_t const &pmt_pid_info);
  bool parse_registration_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor, mpeg_ts_pmt_pid_info_t const &pmt_pid_info);
  bool parse_srt_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor, mpeg_ts_pmt_pid_info_t const &pmt_pid_info);
  bool parse_vobsub_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor, mpeg_ts_pmt_pid_info_t const &pmt_pid_info);

  bool has_packetizer() const;

  void set_pid(uint16_t new_pid);

  void handle_timestamp_wrap(timestamp_c &pts, timestamp_c &dts);
  bool detect_timestamp_wrap(timestamp_c &timestamp);
  void adjust_timestamp_for_wrap(timestamp_c &timestamp);

  void process(packet_cptr const &packet);

  void parse_iso639_language_from(void const *buffer);
};

class mpeg_ts_reader_c: public generic_reader_c {
protected:
  enum processing_state_e {
    ps_probing,
    ps_determining_timestamp_offset,
    ps_muxing,
  };

  bool PAT_found, PMT_found;
  int16_t PMT_pid;
  int es_to_process;
  timestamp_c m_global_timestamp_offset, m_stream_timestamp, m_last_non_subtitle_timestamp;

  processing_state_e m_state;
  uint64_t m_probe_range;

  bool file_done, m_packet_sent_to_packetizer;

  std::vector<mpeg_ts_track_ptr> tracks;
  std::map<generic_packetizer_c *, mpeg_ts_track_ptr> m_ptzr_to_track_map;

  std::vector<timestamp_c> m_chapter_timestamps;

  debugging_option_c m_dont_use_audio_pts, m_debug_resync, m_debug_pat_pmt, m_debug_headers, m_debug_packet, m_debug_aac, m_debug_timestamp_wrapping, m_debug_clpi;

  unsigned int m_detected_packet_size, m_num_pat_crc_errors, m_num_pmt_crc_errors;
  bool m_validate_pat_crc, m_validate_pmt_crc;

protected:
  static int potential_packet_sizes[];

public:
  mpeg_ts_reader_c(const track_info_c &ti, const mm_io_cptr &in);
  virtual ~mpeg_ts_reader_c();

  static bool probe_file(mm_io_c *in, uint64_t size);

  virtual file_type_e get_format_type() const {
    return FILE_TYPE_MPEG_TS;
  }

  virtual void read_headers();
  virtual file_status_e read(generic_packetizer_c *requested_ptzr, bool force = false);
  virtual void identify();
  virtual void create_packetizer(int64_t tid);
  virtual void create_packetizers();
  virtual void add_available_track_ids();

  virtual void parse_packet(unsigned char *buf);

  static timestamp_c read_timestamp(unsigned char *p);
  static int detect_packet_size(mm_io_c *in, uint64_t size);

private:
  mpeg_ts_track_ptr find_track_for_pid(uint16_t pid) const;
  std::pair<unsigned char *, std::size_t> determine_ts_payload_start(mpeg_ts_packet_header_t *hdr) const;

  void handle_ts_payload(mpeg_ts_track_c &track, mpeg_ts_packet_header_t &ts_header, unsigned char *ts_payload, std::size_t ts_payload_size);
  void handle_pat_pmt_payload(mpeg_ts_track_c &track, mpeg_ts_packet_header_t &ts_header, unsigned char *ts_payload, std::size_t ts_payload_size);
  void handle_pes_payload(mpeg_ts_track_c &track, mpeg_ts_packet_header_t &ts_header, unsigned char *ts_payload, std::size_t ts_payload_size);

  bool parse_pat(mpeg_ts_track_c &track);
  bool parse_pmt(mpeg_ts_track_c &track);
  void parse_pes(mpeg_ts_track_c &track);
  void probe_packet_complete(mpeg_ts_track_c &track);
  int determine_track_parameters(mpeg_ts_track_c &track);

  file_status_e finish();
  int send_to_packetizer(mpeg_ts_track_ptr &track);
  void create_mpeg1_2_video_packetizer(mpeg_ts_track_ptr &track);
  void create_mpeg4_p10_es_video_packetizer(mpeg_ts_track_ptr &track);
  void create_mpegh_p2_es_video_packetizer(mpeg_ts_track_ptr &track);
  void create_vc1_video_packetizer(mpeg_ts_track_ptr &track);
  void create_aac_audio_packetizer(mpeg_ts_track_ptr const &track);
  void create_ac3_audio_packetizer(mpeg_ts_track_ptr const &track);
  void create_truehd_audio_packetizer(mpeg_ts_track_ptr const &track);
  void create_hdmv_pgs_subtitles_packetizer(mpeg_ts_track_ptr &track);
  void create_srt_subtitles_packetizer(mpeg_ts_track_ptr const &track);

  void determine_global_timestamp_offset();

  bfs::path find_clip_info_file();
  void parse_clip_info_file();

  void process_chapter_entries();

  bool resync(int64_t start_at);

  uint32_t calculate_crc(void const *buffer, size_t size) const;

  friend class mpeg_ts_track_c;
};

#endif  // MTX_R_MPEG_TS_H
