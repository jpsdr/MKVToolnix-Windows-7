/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the Matroska reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modified by Steve Lhomme <s.lhomme@free.fr>.
*/

#pragma once

#include "common/common_pch.h"

#include <ctime>

#include "common/codec.h"
#include "common/content_decoder.h"
#include "common/dts.h"
#include "common/error.h"
#include "common/kax_file.h"
#include "common/mm_io.h"
#include "merge/block_addition_mapping.h"
#include "merge/generic_reader.h"
#include "merge/track_info.h"

#include <ebml/EbmlUnicodeString.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxCluster.h>
#include <unordered_map>

namespace mtx::id {
class info_c;
}

namespace mtx::tags {
struct converted_vorbis_comments_t;
}

struct kax_track_t {
  uint64_t tnum, track_number, track_uid, num_cue_points;

  std::string codec_id, codec_name, source_id;
  codec_c codec;
  bool ms_compat;

  char type; // 'v' = video, 'a' = audio, 't' = text subs, 'b' = buttons
  char sub_type;                // 't' = text, 'v' = VobSub
  bool passthrough;             // No special packetizer available.

  uint32_t max_blockadd_id;
  bool lacing_flag;
  int64_t default_duration;
  timestamp_c seek_pre_roll, codec_delay;

  // Parameters for video tracks
  uint64_t v_width, v_height, v_dwidth, v_dheight;
  std::optional<uint64_t> v_dunit;
  unsigned int v_display_unit;
  uint64_t v_pcleft, v_pctop, v_pcright, v_pcbottom;
  std::optional<int64_t> v_color_matrix, v_bits_per_channel;
  subsample_or_siting_t v_chroma_subsample, v_cb_subsample, v_chroma_siting;
  std::optional<uint64_t> v_color_range, v_transfer_character, v_color_primaries, v_max_cll, v_max_fall;
  chroma_coordinates_t v_chroma_coordinates;
  white_color_coordinates_t v_white_color_coordinates;
  std::optional<double> v_max_luminance, v_min_luminance;
  std::optional<int64_t> v_field_order;
  stereo_mode_c::mode v_stereo_mode;
  std::optional<bool> v_alpha_mode;
  char v_fourcc[5];
  std::optional<uint64_t> v_projection_type;
  memory_cptr v_projection_private;
  std::optional<double> v_projection_pose_yaw, v_projection_pose_pitch, v_projection_pose_roll;
  std::vector<block_addition_mapping_t> block_addition_mappings;

  // Parameters for audio tracks
  uint64_t a_channels, a_bps, a_formattag;
  std::optional<uint64_t> a_emphasis;
  double a_sfreq, a_osfreq;

  memory_cptr private_data;

  std::vector<memory_cptr> headers;

  bool default_track, forced_track, enabled_track;
  std::optional<bool> hearing_impaired_flag, visual_impaired_flag, text_descriptions_flag, original_flag, commentary_flag;
  mtx::bcp47::language_c language, language_ietf, effective_language;

  int64_t units_processed;

  std::string track_name;

  bool ok;

  int64_t previous_timestamp;

  content_decoder_c content_decoder;

  std::shared_ptr<libmatroska::KaxTags> tags;

  int ptzr;
  generic_packetizer_c *ptzr_ptr;
  bool headers_set;

  bool ignore_duration_hack;

  std::vector<memory_cptr> first_frames_data;

  mtx::dts::header_t dts_header;

  memory_cptr v_color_space;

  std::unordered_map<uint64_t, bool> m_registered_used_of_webm_block_addition_id;

  kax_track_t()
    : tnum(0)
    , track_number(0)
    , track_uid(0)
    , num_cue_points(0)
    , ms_compat(false)
    , type(' ')
    , sub_type(' ')
    , passthrough(false)
    , max_blockadd_id(0)
    , lacing_flag(true)
    , default_duration(0)
    , v_width(0)
    , v_height(0)
    , v_dwidth(0)
    , v_dheight(0)
    , v_display_unit(0)
    , v_pcleft(0)
    , v_pctop(0)
    , v_pcright(0)
    , v_pcbottom(0)
    , v_chroma_coordinates{}
    , v_white_color_coordinates{}
    , v_stereo_mode(stereo_mode_c::unspecified)
    , a_channels(0)
    , a_bps(0)
    , a_formattag(0)
    , a_sfreq(8000.0)
    , a_osfreq(0.0)
    , default_track(true)
    , forced_track(false)
    , enabled_track(true)
    , language{mtx::bcp47::language_c::parse("eng")}
    , units_processed(0)
    , ok(false)
    , previous_timestamp(0)
    , tags(nullptr)
    , ptzr(-1)
    , ptzr_ptr(nullptr)
    , headers_set(false)
    , ignore_duration_hack(false)
    , v_color_space(0)
  {
    memset(v_fourcc, 0, 5);
  }

  void handle_packetizer_display_dimensions();
  void handle_packetizer_pixel_cropping();
  void handle_packetizer_color();
  void handle_packetizer_field_order();
  void handle_packetizer_stereo_mode();
  void handle_packetizer_alpha_mode();
  void handle_packetizer_pixel_dimensions();
  void handle_packetizer_default_duration();
  void handle_packetizer_output_sampling_freq();
  void handle_packetizer_codec_delay();
  void handle_packetizer_block_addition_mapping();
  void fix_display_dimension_parameters();
  void get_source_id_from_track_statistics_tags();
  void discard_track_statistics_tags();
  void register_use_of_webm_block_addition_id(uint64_t id);
};
using kax_track_cptr = std::shared_ptr<kax_track_t>;

class kax_reader_c: public generic_reader_c {
private:
  enum deferred_l1_type_e {
    dl1t_unknown,
    dl1t_attachments,
    dl1t_chapters,
    dl1t_tags,
    dl1t_tracks,
    dl1t_seek_head,
    dl1t_info,
    dl1t_cues,
  };

  std::vector<kax_track_cptr> m_tracks;
  std::map<generic_packetizer_c *, kax_track_t *> m_ptzr_to_track_map;
  std::unordered_map<uint64_t, timestamp_c> m_minimum_timestamps_by_track_number;
  std::unordered_map<uint64_t, bool> m_known_bad_track_numbers;

  uint64_t m_tc_scale;

  kax_file_cptr m_in_file;

  std::shared_ptr<libebml::EbmlStream> m_es;

  int64_t m_segment_duration{}, m_last_timestamp{}, m_global_timestamp_offset{};

  using deferred_positions_t = std::map<deferred_l1_type_e, std::vector<int64_t> >;
  deferred_positions_t m_deferred_l1_positions, m_handled_l1_positions;

  std::string m_writing_app, m_raw_writing_app, m_muxing_app;
  int64_t m_writing_app_ver{-1};
  std::optional<std::time_t> m_muxing_date_epoch;

  memory_cptr m_segment_uid, m_next_segment_uid, m_previous_segment_uid;

  int64_t m_attachment_id{};

  std::shared_ptr<libmatroska::KaxTags> m_tags;

  file_status_e m_file_status{FILE_STATUS_MOREDATA};

  bool m_opus_experimental_warning_shown{}, m_regenerate_chapter_uids{}, m_regenerate_track_uids{}, m_is_webm{};
  std::unordered_map<uint64_t, uint64_t> m_track_uid_mapping;

  debugging_option_c m_debug_minimum_timestamp{"kax_reader|kax_reader_minimum_timestamp"}, m_debug_track_headers{"kax_reader|kax_reader_track_headers"};

public:
  kax_reader_c();

  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::matroska;
  }

  virtual void read_headers();

  virtual void set_headers();
  virtual void identify();
  virtual void create_packetizers();
  virtual void create_packetizer(int64_t tid);
  virtual void add_available_track_ids();

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;
  virtual file_status_e finish_file();

  virtual void set_track_packetizer(kax_track_t *t, generic_packetizer_c *packetizer);
  virtual void init_passthrough_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void set_packetizer_headers(kax_track_t *t);
  virtual void read_first_frames(kax_track_t *t, unsigned num_wanted = 1);
  virtual kax_track_t *find_track_by_num(uint64_t num, kax_track_t *c = nullptr);
  virtual kax_track_t *find_track_by_uid(uint64_t uid, kax_track_t *c = nullptr);

  virtual bool verify_acm_audio_track(kax_track_t *t);
  virtual bool verify_ac3_audio_track(kax_track_t *t);
  virtual bool verify_alac_audio_track(kax_track_t *t);
  virtual bool verify_dts_audio_track(kax_track_t *t);
  virtual bool verify_flac_audio_track(kax_track_t *t);
  virtual bool verify_opus_audio_track(kax_track_t *t);
  virtual bool verify_truehd_audio_track(kax_track_t *t);
  virtual bool verify_vorbis_audio_track(kax_track_t *t);
  virtual void verify_audio_track(kax_track_t *t);
  virtual bool verify_mscomp_video_track(kax_track_t *t);
  virtual bool verify_theora_video_track(kax_track_t *t);
  virtual void verify_video_track(kax_track_t *t);
  virtual bool verify_dvb_subtitle_track(kax_track_t *t);
  virtual bool verify_hdmv_textst_subtitle_track(kax_track_t *t);
  virtual bool verify_kate_subtitle_track(kax_track_t *t);
  virtual bool verify_vobsub_subtitle_track(kax_track_t *t);
  virtual void verify_subtitle_track(kax_track_t *t);
  virtual void verify_button_track(kax_track_t *t);
  virtual void verify_tracks();

  virtual bool packets_available();
  virtual void handle_attachments(mm_io_c *io, libebml::EbmlElement *l0, int64_t pos);
  virtual void handle_chapters(mm_io_c *io, libebml::EbmlElement *l0, int64_t pos);
  virtual void handle_cues(mm_io_c *io, libebml::EbmlElement *l0, int64_t pos);
  virtual void handle_seek_head(mm_io_c *io, libebml::EbmlElement *l0, int64_t pos);
  virtual void handle_tags(mm_io_c *io, libebml::EbmlElement *l0, int64_t pos);
  virtual void process_global_tags();
  virtual void handle_track_statistics_tags();

  virtual bool unlace_vorbis_private_data(kax_track_t *t, uint8_t *buffer, int size);

  virtual void create_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_subtitle_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_button_packetizer(kax_track_t *t, track_info_c &nti);

  virtual void create_aac_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_ac3_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_alac_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_dts_audio_packetizer(kax_track_t *t, track_info_c &nti);
#if defined(HAVE_FLAC_FORMAT_H)
  virtual void create_flac_audio_packetizer(kax_track_t *t, track_info_c &nti);
#endif
  virtual void create_mp3_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_opus_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_pcm_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_truehd_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_tta_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_vorbis_audio_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_wavpack_audio_packetizer(kax_track_t *t, track_info_c &nti);

  virtual void create_av1_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_avc_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_avc_es_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_hevc_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_hevc_es_video_packetizer(kax_track_t *t, track_info_c &nti);
  virtual void create_prores_video_packetizer(kax_track_t &t, track_info_c &nti);
  virtual void create_vc1_video_packetizer(kax_track_t *t, track_info_c &nti);

  virtual void create_dvbsub_subtitle_packetizer(kax_track_t &t, track_info_c &nti);

  virtual void read_headers_info(mm_io_c *io, libebml::EbmlElement *l0, int64_t position);
  virtual void read_headers_info_writing_app(libmatroska::KaxWritingApp *&kwriting_app);
  virtual void read_headers_track_audio(kax_track_t *track, libmatroska::KaxTrackAudio *ktaudio);
  virtual void read_headers_track_video(kax_track_t *track, libmatroska::KaxTrackVideo *ktvideo);
  virtual void read_headers_tracks(mm_io_c *io, libebml::EbmlElement *l0, int64_t position);
  virtual bool read_headers_internal();
  virtual void read_deferred_level1_elements(libmatroska::KaxSegment &segment);
  virtual void find_level1_elements_via_analyzer();

  virtual void process_simple_block(libmatroska::KaxCluster *cluster, libmatroska::KaxSimpleBlock *block_simple);
  virtual void process_block_group(libmatroska::KaxCluster *cluster, libmatroska::KaxBlockGroup *block_group);
  virtual void process_block_group_common(libmatroska::KaxBlockGroup *block_group, packet_t *packet, kax_track_t &track);

  void init_l1_position_storage(deferred_positions_t &storage);
  virtual bool has_deferred_element_been_processed(deferred_l1_type_e type, int64_t position);

  virtual void determine_minimum_timestamps();
  virtual void determine_global_timestamp_offset_to_apply();
  virtual void adjust_chapter_timestamps();

  virtual void handle_vorbis_comments(kax_track_t &t);
  virtual void handle_vorbis_comments_cover_art(mtx::tags::converted_vorbis_comments_t const &converted);
  virtual void handle_vorbis_comments_tags(mtx::tags::converted_vorbis_comments_t const &converted, kax_track_t &t);
};
