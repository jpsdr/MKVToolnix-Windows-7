/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the generic_packetizer_c implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <matroska/KaxTracks.h>

#include "common/common_urls.h"
#include "common/compression.h"
#include "common/container.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/hacks.h"
#include "common/option_with_source.h"
#include "common/strings/formatting.h"
#include "common/unique_numbers.h"
#include "common/xml/ebml_tags_converter.h"
#include "merge/cluster_helper.h"
#include "merge/filelist.h"
#include "merge/generic_packetizer.h"
#include "merge/generic_reader.h"
#include "merge/output_control.h"
#include "merge/webm.h"

namespace {

constexpr auto
track_type_to_deftrack_type(int type) {
  return track_audio == type ? DEFTRACK_TYPE_AUDIO
       : track_video == type ? DEFTRACK_TYPE_VIDEO
       :                       DEFTRACK_TYPE_SUBS;
}

template<typename T>
auto
lookup_track_id(T const &container,
                int64_t track_id) {
  return mtx::includes(container, track_id) ? track_id
       : mtx::includes(container, -1)       ? -1
       :                                      -2;
}

debugging_option_c s_debug{"generic_packetizer"};

}

static std::unordered_map<std::string, bool> s_experimental_status_warning_shown;
std::vector<generic_packetizer_c *> ptzrs_in_header_order;

int generic_packetizer_c::ms_track_number = 0;

generic_packetizer_c::generic_packetizer_c(generic_reader_c *reader,
                                           track_info_c &ti,
                                           track_type type)
  : m_num_packets{}
  , m_next_packet_wo_assigned_timestamp{}
  , m_free_refs{-1}
  , m_next_free_refs{-1}
  , m_enqueued_bytes{}
  , m_safety_last_timestamp{}
  , m_safety_last_duration{}
  , m_track_entry{}
  , m_hserialno{0}
  , m_htrack_type{-1}
  , m_htrack_default_duration{-1}
  , m_htrack_default_duration_indicates_fields{}
  , m_default_duration_forced{true}
  , m_huid{}
  , m_haudio_sampling_freq{-1.0}
  , m_haudio_output_sampling_freq{-1.0}
  , m_haudio_channels{-1}
  , m_haudio_bit_depth{-1}
  , m_hvideo_interlaced_flag{-1}
  , m_hvideo_pixel_width{-1}
  , m_hvideo_pixel_height{-1}
  , m_hvideo_display_width{-1}
  , m_hvideo_display_height{-1}
  , m_hvideo_display_unit{ddu_pixels}
  , m_hcompression{COMPRESSION_UNSPECIFIED}
  , m_timestamp_factory_application_mode{TFA_AUTOMATIC}
  , m_last_cue_timestamp{-1}
  , m_has_been_flushed{}
  , m_prevent_lacing{}
  , m_connected_successor{}
  , m_ti{ti}
  , m_reader{reader}
  , m_connected_to{}
  , m_correction_timestamp_offset{}
  , m_append_timestamp_offset{}
  , m_max_timestamp_seen{}
  , m_relaxed_timestamp_checking{}
{
  auto set_bool_maybe = [this](std::map<int64_t, bool> &flags, option_with_source_c<bool> &flag) {
    if (mtx::includes(flags, m_ti.m_id))
      flag.set(flags[m_ti.m_id], option_source_e::command_line);
    else if (mtx::includes(flags, -1))
      flag.set(flags[-1], option_source_e::command_line);
  };

  // Let's see if the user specified timestamp sync for this track.
  if (mtx::includes(m_ti.m_timestamp_syncs, m_ti.m_id))
    m_ti.m_tcsync = m_ti.m_timestamp_syncs[m_ti.m_id];
  else if (mtx::includes(m_ti.m_timestamp_syncs, -1))
    m_ti.m_tcsync = m_ti.m_timestamp_syncs[-1];
  if (0 == m_ti.m_tcsync.factor)
    m_ti.m_tcsync.factor = mtx_mp_rational_t{1, 1};

  // Let's see if the user specified "reset timestamps" for this track.
  m_ti.m_reset_timestamps = mtx::includes(m_ti.m_reset_timestamps_specs, m_ti.m_id) || mtx::includes(m_ti.m_reset_timestamps_specs, -1);

  // Let's see if the user has specified which cues he wants for this track.
  if (mtx::includes(m_ti.m_cue_creations, m_ti.m_id))
    m_ti.m_cues = m_ti.m_cue_creations[m_ti.m_id];
  else if (mtx::includes(m_ti.m_cue_creations, -1))
    m_ti.m_cues = m_ti.m_cue_creations[-1];

  if (mtx::includes(m_ti.m_fix_bitstream_frame_rate_flags, m_ti.m_id))
    m_ti.m_fix_bitstream_frame_rate = m_ti.m_fix_bitstream_frame_rate_flags[m_ti.m_id];
  else if (mtx::includes(m_ti.m_fix_bitstream_frame_rate_flags, -1))
    m_ti.m_fix_bitstream_frame_rate = m_ti.m_fix_bitstream_frame_rate_flags[-1];

  set_bool_maybe(m_ti.m_default_track_flags,     m_ti.m_default_track);
  set_bool_maybe(m_ti.m_forced_track_flags,      m_ti.m_forced_track);
  set_bool_maybe(m_ti.m_enabled_track_flags,     m_ti.m_enabled_track);
  set_bool_maybe(m_ti.m_hearing_impaired_flags,  m_ti.m_hearing_impaired_flag);
  set_bool_maybe(m_ti.m_visual_impaired_flags,   m_ti.m_visual_impaired_flag);
  set_bool_maybe(m_ti.m_text_descriptions_flags, m_ti.m_text_descriptions_flag);
  set_bool_maybe(m_ti.m_original_flags,          m_ti.m_original_flag);
  set_bool_maybe(m_ti.m_commentary_flags,        m_ti.m_commentary_flag);

  // Let's see if the user has specified a language for this track.
  if (mtx::includes(m_ti.m_languages, m_ti.m_id))
    m_ti.m_language = m_ti.m_languages[m_ti.m_id];
  else if (mtx::includes(m_ti.m_languages, -1))
    m_ti.m_language = m_ti.m_languages[-1];

  // Let's see if the user has specified a sub charset for this track.
  if (mtx::includes(m_ti.m_sub_charsets, m_ti.m_id))
    m_ti.m_sub_charset = m_ti.m_sub_charsets[m_ti.m_id];
  else if (mtx::includes(m_ti.m_sub_charsets, -1))
    m_ti.m_sub_charset = m_ti.m_sub_charsets[-1];

  // Let's see if the user has specified a sub charset for this track.
  if (mtx::includes(m_ti.m_all_tags, m_ti.m_id))
    m_ti.m_tags_file_name = m_ti.m_all_tags[m_ti.m_id];
  else if (mtx::includes(m_ti.m_all_tags, -1))
    m_ti.m_tags_file_name = m_ti.m_all_tags[-1];
  if (!m_ti.m_tags_file_name.empty())
    m_ti.m_tags = mtx::xml::ebml_tags_converter_c::parse_file(m_ti.m_tags_file_name, false);

  // Let's see if the user has specified how this track should be compressed.
  if (mtx::includes(m_ti.m_compression_list, m_ti.m_id))
    m_ti.m_compression = m_ti.m_compression_list[m_ti.m_id];
  else if (mtx::includes(m_ti.m_compression_list, -1))
    m_ti.m_compression = m_ti.m_compression_list[-1];

  // Let's see if the user has specified a name for this track.
  if (mtx::includes(m_ti.m_track_names, m_ti.m_id))
    m_ti.m_track_name = m_ti.m_track_names[m_ti.m_id];
  else if (mtx::includes(m_ti.m_track_names, -1))
    m_ti.m_track_name = m_ti.m_track_names[-1];

  // Let's see if the user has specified external timestamps for this track.
  if (mtx::includes(m_ti.m_all_ext_timestamps, m_ti.m_id))
    m_ti.m_ext_timestamps = m_ti.m_all_ext_timestamps[m_ti.m_id];
  else if (mtx::includes(m_ti.m_all_ext_timestamps, -1))
    m_ti.m_ext_timestamps = m_ti.m_all_ext_timestamps[-1];

  // Let's see if the user has specified an audio emphasis mode for this track.
  int i = lookup_track_id(m_ti.m_audio_emphasis_list, m_ti.m_id);
  if (-2 != i)
    set_audio_emphasis(m_ti.m_audio_emphasis_list[m_ti.m_id], option_source_e::command_line);

  // Let's see if the user has specified an aspect ratio or display dimensions
  // for this track.
  i = lookup_track_id(m_ti.m_display_properties, m_ti.m_id);
  if (-2 != i) {
    display_properties_t &dprop = m_ti.m_display_properties[i];
    if (0 > dprop.aspect_ratio) {
      set_video_display_dimensions(dprop.width, dprop.height, generic_packetizer_c::ddu_pixels, option_source_e::command_line);
    } else {
      set_video_aspect_ratio(dprop.aspect_ratio, dprop.ar_factor, option_source_e::command_line);
      m_ti.m_aspect_ratio_given = true;
    }
  }

  if (m_ti.m_aspect_ratio_given && m_ti.m_display_dimensions_given) {
    if (m_ti.m_aspect_ratio_is_factor)
      mxerror_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Both the aspect ratio factor and '--display-dimensions' were given.\n")));
    else
      mxerror_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Both the aspect ratio and '--display-dimensions' were given.\n")));
  }

  // Let's see if the user has specified a FourCC for this track.
  if (mtx::includes(m_ti.m_all_fourccs, m_ti.m_id))
    m_ti.m_fourcc = m_ti.m_all_fourccs[m_ti.m_id];
  else if (mtx::includes(m_ti.m_all_fourccs, -1))
    m_ti.m_fourcc = m_ti.m_all_fourccs[-1];

  // Let's see if the user has specified cropping parameters for this track.
  i = lookup_track_id(m_ti.m_pixel_crop_list, m_ti.m_id);
  if (-2 != i)
    set_video_pixel_cropping(m_ti.m_pixel_crop_list[i], option_source_e::command_line);

  // Let's see if the user has specified color matrix for this track.
  i = lookup_track_id(m_ti.m_color_matrix_coeff_list, m_ti.m_id);
  if (-2 != i)
    set_video_color_matrix(m_ti.m_color_matrix_coeff_list[i], option_source_e::command_line);

  // Let's see if the user has specified bits per channel parameter for this track.
  i = lookup_track_id(m_ti.m_bits_per_channel_list, m_ti.m_id);
  if (-2 != i)
    set_video_bits_per_channel(m_ti.m_bits_per_channel_list[i], option_source_e::command_line);

  // Let's see if the user has specified chroma subsampling parameter for this track.
  i = lookup_track_id(m_ti.m_chroma_subsample_list, m_ti.m_id);
  if (-2 != i)
    set_video_chroma_subsample(m_ti.m_chroma_subsample_list[i], option_source_e::command_line);

  // Let's see if the user has specified Cb subsampling parameter for this track.
  i = lookup_track_id(m_ti.m_cb_subsample_list, m_ti.m_id);
  if (-2 != i)
    set_video_cb_subsample(m_ti.m_cb_subsample_list[i], option_source_e::command_line);

  // Let's see if the user has specified chroma siting parameter for this track.
  i = lookup_track_id(m_ti.m_chroma_siting_list, m_ti.m_id);
  if (-2 != i)
    set_video_chroma_siting(m_ti.m_chroma_siting_list[i], option_source_e::command_line);

  // Let's see if the user has specified color range parameter for this track.
  i = lookup_track_id(m_ti.m_color_range_list, m_ti.m_id);
  if (-2 != i)
    set_video_color_range(m_ti.m_color_range_list[i], option_source_e::command_line);

  // Let's see if the user has specified color transfer characteristics parameter for this track.
  i = lookup_track_id(m_ti.m_color_transfer_list, m_ti.m_id);
  if (-2 != i)
    set_video_color_transfer_character(m_ti.m_color_transfer_list[i], option_source_e::command_line);

  // Let's see if the user has specified color primaries parameter for this track.
  i = lookup_track_id(m_ti.m_color_primaries_list, m_ti.m_id);
  if (-2 != i)
    set_video_color_primaries(m_ti.m_color_primaries_list[i], option_source_e::command_line);

  // Let's see if the user has specified max content light parameter for this track.
  i = lookup_track_id(m_ti.m_max_cll_list, m_ti.m_id);
  if (-2 != i)
    set_video_max_cll(m_ti.m_max_cll_list[i], option_source_e::command_line);

  // Let's see if the user has specified max frame light parameter for this track.
  i = lookup_track_id(m_ti.m_max_fall_list, m_ti.m_id);
  if (-2 != i)
    set_video_max_fall(m_ti.m_max_fall_list[i], option_source_e::command_line);

  // Let's see if the user has specified chromaticity coordinates parameter for this track.
  i = lookup_track_id(m_ti.m_chroma_coordinates_list, m_ti.m_id);
  if (-2 != i)
    set_video_chroma_coordinates(m_ti.m_chroma_coordinates_list[i], option_source_e::command_line);

  // Let's see if the user has specified white color coordinates parameter for this track.
  i = lookup_track_id(m_ti.m_white_coordinates_list, m_ti.m_id);
  if (-2 != i)
    set_video_white_color_coordinates(m_ti.m_white_coordinates_list[i], option_source_e::command_line);

  // Let's see if the user has specified max luminance parameter for this track.
  i = lookup_track_id(m_ti.m_max_luminance_list, m_ti.m_id);
  if (-2 != i)
    set_video_max_luminance(m_ti.m_max_luminance_list[i], option_source_e::command_line);

  // Let's see if the user has specified min luminance parameter for this track.
  i = lookup_track_id(m_ti.m_min_luminance_list, m_ti.m_id);
  if (-2 != i)
    set_video_min_luminance(m_ti.m_min_luminance_list[i], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_projection_type_list, m_ti.m_id);
  if (-2 != i)
    set_video_projection_type(m_ti.m_projection_type_list[i], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_projection_private_list, m_ti.m_id);
  if (-2 != i)
    set_video_projection_private(m_ti.m_projection_private_list[i], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_projection_pose_yaw_list, m_ti.m_id);
  if (-2 != i)
    set_video_projection_pose_yaw(m_ti.m_projection_pose_yaw_list[i], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_projection_pose_pitch_list, m_ti.m_id);
  if (-2 != i)
    set_video_projection_pose_pitch(m_ti.m_projection_pose_pitch_list[i], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_projection_pose_roll_list, m_ti.m_id);
  if (-2 != i)
    set_video_projection_pose_roll(m_ti.m_projection_pose_roll_list[i], option_source_e::command_line);

  // Let's see if the user has specified a field order for this track.
  i = lookup_track_id(m_ti.m_field_order_list, m_ti.m_id);
  if (-2 != i)
    set_video_field_order(m_ti.m_field_order_list[m_ti.m_id], option_source_e::command_line);

  // Let's see if the user has specified a stereo mode for this track.
  i = lookup_track_id(m_ti.m_stereo_mode_list, m_ti.m_id);
  if (-2 != i)
    set_video_stereo_mode(m_ti.m_stereo_mode_list[m_ti.m_id], option_source_e::command_line);

  i = lookup_track_id(m_ti.m_alpha_mode_list, m_ti.m_id);
  if (-2 != i)
    set_video_alpha_mode(m_ti.m_alpha_mode_list[m_ti.m_id], option_source_e::command_line);

  // Let's see if the user has specified a default duration for this track.
  if (mtx::includes(m_ti.m_default_durations, m_ti.m_id)) {
    m_htrack_default_duration                  = m_ti.m_default_durations[m_ti.m_id].first;
    m_htrack_default_duration_indicates_fields = m_ti.m_default_durations[m_ti.m_id].second;

  } else if (mtx::includes(m_ti.m_default_durations, -1)) {
    m_htrack_default_duration                  = m_ti.m_default_durations[-1].first;
    m_htrack_default_duration_indicates_fields = m_ti.m_default_durations[-1].second;

  } else
    m_default_duration_forced = false;

  // Let's see if the user has specified a compression scheme for this track.
  if (COMPRESSION_UNSPECIFIED != m_ti.m_compression)
    m_hcompression = m_ti.m_compression;

  m_timestamp_factory = timestamp_factory_c::create(m_ti.m_ext_timestamps, m_ti.m_fname, m_ti.m_id);

  // If no external timestamp file but a default duration has been
  // given then create a simple timestamp factory that generates the
  // timestamps for the given FPS.
  if (!m_timestamp_factory && (-1 != m_htrack_default_duration)) {
    auto divisor        = m_htrack_default_duration_indicates_fields ? 2 : 1;
    m_timestamp_factory = timestamp_factory_c::create_fps_factory(m_htrack_default_duration / divisor, m_ti.m_tcsync);
  }

  set_track_type(type);
}

generic_packetizer_c::~generic_packetizer_c() {
}

void
generic_packetizer_c::set_tag_track_uid() {
  if (!m_ti.m_tags)
    return;

  mtx::tags::convert_old(*m_ti.m_tags);
  size_t idx_tags;
  for (idx_tags = 0; m_ti.m_tags->ListSize() > idx_tags; ++idx_tags) {
    libmatroska::KaxTag *tag = (libmatroska::KaxTag *)(*m_ti.m_tags)[idx_tags];

    mtx::tags::remove_track_uid_targets(tag);

    get_child<libmatroska::KaxTagTrackUID>(get_child<libmatroska::KaxTagTargets>(tag)).SetValue(m_huid);

    fix_mandatory_elements(tag);

    if (!tag->CheckMandatory())
      mxerror(fmt::format(FY("The tags in '{0}' could not be parsed: some mandatory elements are missing.\n"),
                          m_ti.m_tags_file_name != "" ? m_ti.m_tags_file_name : m_ti.m_fname));
  }
}

bool
generic_packetizer_c::set_uid(uint64_t uid) {
  if (!is_unique_number(uid, UNIQUE_TRACK_IDS))
    return false;

  add_unique_number(uid, UNIQUE_TRACK_IDS);
  m_huid = uid;
  if (m_track_entry)
    get_child<libmatroska::KaxTrackUID>(m_track_entry).SetValue(m_huid);

  return true;
}

void
generic_packetizer_c::set_track_type(track_type type) {
  m_htrack_type = type;

  if (CUE_STRATEGY_UNSPECIFIED == get_cue_creation())
    m_ti.m_cues = track_audio    == type ? CUE_STRATEGY_SPARSE
                : track_video    == type ? CUE_STRATEGY_IFRAMES
                : track_subtitle == type ? CUE_STRATEGY_IFRAMES
                :                          CUE_STRATEGY_UNSPECIFIED;

  if (track_audio == type)
    m_reader->m_num_audio_tracks++;

  else if (track_video == type)
    m_reader->m_num_video_tracks++;

  else {
    m_reader->m_num_subtitle_tracks++;
    m_timestamp_factory_application_mode = TFA_IMMEDIATE;
  }

  g_cluster_helper->register_new_packetizer(*this);
}

void
generic_packetizer_c::set_timestamp_factory_application_mode(timestamp_factory_application_e tfa_mode) {
  if (   (TFA_AUTOMATIC == tfa_mode)
      && (TFA_AUTOMATIC == m_timestamp_factory_application_mode))
    m_timestamp_factory_application_mode
      = (track_video    == m_htrack_type) ? TFA_FULL_QUEUEING
      : (track_subtitle == m_htrack_type) ? TFA_IMMEDIATE
      : (track_buttons  == m_htrack_type) ? TFA_IMMEDIATE
      :                                     TFA_FULL_QUEUEING;

  else if (TFA_AUTOMATIC != tfa_mode)
    m_timestamp_factory_application_mode = tfa_mode;

  if (m_timestamp_factory && (track_video != m_htrack_type) && (track_audio != m_htrack_type))
    m_timestamp_factory->set_preserve_duration(true);
}

void
generic_packetizer_c::set_track_name(const std::string &name) {
  m_ti.m_track_name = name;
  if (m_track_entry && !name.empty())
    get_child<libmatroska::KaxTrackName>(m_track_entry).SetValueUTF8(m_ti.m_track_name);
}

void
generic_packetizer_c::set_codec_id(const std::string &id) {
  m_hcodec_id = id;
  if (m_track_entry && !id.empty())
    get_child<libmatroska::KaxCodecID>(m_track_entry).SetValue(m_hcodec_id);
}

void
generic_packetizer_c::set_codec_private(memory_cptr const &buffer) {
  if (buffer && buffer->get_size()) {
    m_hcodec_private = buffer->clone();

    if (m_track_entry)
      get_child<libmatroska::KaxCodecPrivate>(*m_track_entry).CopyBuffer(static_cast<uint8_t *>(m_hcodec_private->get_buffer()), m_hcodec_private->get_size());

  } else
    m_hcodec_private.reset();
}

void
generic_packetizer_c::set_codec_name(std::string const &name) {
  m_hcodec_name = name;
  if (m_track_entry && !name.empty())
    get_child<libmatroska::KaxCodecName>(m_track_entry).SetValueUTF8(m_hcodec_name);
}

void
generic_packetizer_c::set_track_default_duration(int64_t def_dur,
                                                 bool force) {
  if (!force && m_default_duration_forced)
    return;

  m_htrack_default_duration = mtx::to_int(m_ti.m_tcsync.factor * def_dur);

  if (m_track_entry) {
    if (m_htrack_default_duration)
      get_child<libmatroska::KaxTrackDefaultDuration>(m_track_entry).SetValue(m_htrack_default_duration);
    else
      delete_children<libmatroska::KaxTrackDefaultDuration>(m_track_entry);
  }
}

int64_t
generic_packetizer_c::get_track_default_duration()
  const {
  return m_htrack_default_duration;
}

void
generic_packetizer_c::set_track_default_flag(bool default_track,
                                             option_source_e source) {
  m_ti.m_default_track.set(default_track, source);
  if (m_track_entry)
    get_child<libmatroska::KaxTrackFlagDefault>(m_track_entry).SetValue(m_ti.m_default_track.get() ? 1 : 0);
}

void
generic_packetizer_c::set_track_forced_flag(bool forced_track,
                                            option_source_e source) {
  m_ti.m_forced_track.set(forced_track, source);
  if (m_track_entry)
    get_child<libmatroska::KaxTrackFlagForced>(m_track_entry).SetValue(m_ti.m_forced_track.get() ? 1 : 0);
}

void
generic_packetizer_c::set_track_enabled_flag(bool enabled_track,
                                             option_source_e source) {
  m_ti.m_enabled_track.set(enabled_track, source);
  if (m_track_entry)
    get_child<libmatroska::KaxTrackFlagEnabled>(m_track_entry).SetValue(m_ti.m_enabled_track.get() ? 1 : 0);
}

void
generic_packetizer_c::set_hearing_impaired_flag(bool hearing_impaired_flag,
                                                option_source_e source) {
  m_ti.m_hearing_impaired_flag.set(hearing_impaired_flag, source);
  if (m_track_entry)
    get_child<libmatroska::KaxFlagHearingImpaired>(m_track_entry).SetValue(m_ti.m_hearing_impaired_flag.get() ? 1 : 0);
}

void
generic_packetizer_c::set_visual_impaired_flag(bool visual_impaired_flag,
                                               option_source_e source) {
  m_ti.m_visual_impaired_flag.set(visual_impaired_flag, source);
  if (m_track_entry)
    get_child<libmatroska::KaxFlagVisualImpaired>(m_track_entry).SetValue(m_ti.m_visual_impaired_flag.get() ? 1 : 0);
}

void
generic_packetizer_c::set_text_descriptions_flag(bool text_descriptions_flag,
                                                 option_source_e source) {
  m_ti.m_text_descriptions_flag.set(text_descriptions_flag, source);
  if (m_track_entry)
    get_child<libmatroska::KaxFlagTextDescriptions>(m_track_entry).SetValue(m_ti.m_text_descriptions_flag.get() ? 1 : 0);
}

void
generic_packetizer_c::set_original_flag(bool original_flag,
                                        option_source_e source) {
  m_ti.m_original_flag.set(original_flag, source);
  if (m_track_entry)
    get_child<libmatroska::KaxFlagOriginal>(m_track_entry).SetValue(m_ti.m_original_flag.get() ? 1 : 0);
}

void
generic_packetizer_c::set_commentary_flag(bool commentary_flag,
                                          option_source_e source) {
  m_ti.m_commentary_flag.set(commentary_flag, source);
  if (m_track_entry)
    get_child<libmatroska::KaxFlagCommentary>(m_track_entry).SetValue(m_ti.m_commentary_flag.get() ? 1 : 0);
}

void
generic_packetizer_c::set_track_seek_pre_roll(timestamp_c const &seek_pre_roll) {
  m_seek_pre_roll = seek_pre_roll;
  if (m_track_entry)
    get_child<libmatroska::KaxSeekPreRoll>(m_track_entry).SetValue(seek_pre_roll.to_ns());
}

void
generic_packetizer_c::set_codec_delay(timestamp_c const &codec_delay) {
  m_codec_delay = codec_delay;
  if (m_track_entry)
    get_child<libmatroska::KaxCodecDelay>(m_track_entry).SetValue(codec_delay.to_ns());
}

void
generic_packetizer_c::set_audio_sampling_freq(double freq) {
  m_haudio_sampling_freq = freq;
  if (m_track_entry)
    get_child<libmatroska::KaxAudioSamplingFreq>(get_child<libmatroska::KaxTrackAudio>(m_track_entry)).SetValue(m_haudio_sampling_freq);
}

void
generic_packetizer_c::set_audio_output_sampling_freq(double freq) {
  m_haudio_output_sampling_freq = freq;
  if (m_track_entry)
    get_child<libmatroska::KaxAudioOutputSamplingFreq>(get_child<libmatroska::KaxTrackAudio>(m_track_entry)).SetValue(m_haudio_output_sampling_freq);
}

void
generic_packetizer_c::set_audio_channels(int channels) {
  m_haudio_channels = channels;
  if (m_track_entry)
    get_child<libmatroska::KaxAudioChannels>(get_child<libmatroska::KaxTrackAudio>(*m_track_entry)).SetValue(m_haudio_channels);
}

void
generic_packetizer_c::set_audio_bit_depth(int bit_depth) {
  m_haudio_bit_depth = bit_depth;
  if (m_track_entry)
    get_child<libmatroska::KaxAudioBitDepth>(get_child<libmatroska::KaxTrackAudio>(*m_track_entry)).SetValue(m_haudio_bit_depth);
}

void
generic_packetizer_c::set_audio_emphasis(audio_emphasis_c::mode_e audio_emphasis,
                                         option_source_e source) {
  m_ti.m_audio_emphasis.set(audio_emphasis, source);

  if (m_track_entry && (audio_emphasis_c::unspecified != m_ti.m_audio_emphasis.get()))
    set_audio_emphasis_impl(get_child<libmatroska::KaxTrackAudio>(*m_track_entry), m_ti.m_audio_emphasis.get());
}

void
generic_packetizer_c::set_audio_emphasis_impl(libebml::EbmlMaster &audio,
                                              audio_emphasis_c::mode_e audio_emphasis) {
  get_child<libmatroska::KaxEmphasis>(audio).SetValue(static_cast<unsigned int>(audio_emphasis));
}

void
generic_packetizer_c::set_video_interlaced_flag(bool interlaced) {
  m_hvideo_interlaced_flag = interlaced ? 1 : 0;
  if (m_track_entry)
    get_child<libmatroska::KaxVideoFlagInterlaced>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_interlaced_flag);
}

void
generic_packetizer_c::set_video_pixel_dimensions(int width,
                                                 int height) {
  m_hvideo_pixel_width  = width;
  m_hvideo_pixel_height = height;
  if (m_track_entry) {
    get_child<libmatroska::KaxVideoPixelHeight>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_pixel_height);
    get_child<libmatroska::KaxVideoPixelWidth>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_pixel_width);
  }
}

void
generic_packetizer_c::set_video_display_dimensions(int width,
                                                   int height,
                                                   int unit,
                                                   option_source_e source) {
  if (display_dimensions_or_aspect_ratio_set() && (m_ti.m_display_dimensions_source >= source))
    return;

  m_ti.m_display_width             = width;
  m_ti.m_display_height            = height;
  m_ti.m_display_unit              = unit;
  m_ti.m_display_dimensions_source = source;
  m_ti.m_display_dimensions_given  = true;
  m_ti.m_aspect_ratio_given        = false;

  m_hvideo_display_width           = width;
  m_hvideo_display_height          = height;
  m_hvideo_display_unit            = unit;

  if (m_track_entry) {
    get_child<libmatroska::KaxVideoDisplayWidth>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_display_width);
    get_child<libmatroska::KaxVideoDisplayHeight>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_display_height);

    if (   (unit != ddu_pixels)
        || (unit != find_child_value<libmatroska::KaxVideoDisplayUnit>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry), ddu_pixels)))
      get_child<libmatroska::KaxVideoDisplayUnit>(get_child<libmatroska::KaxTrackVideo>(*m_track_entry)).SetValue(m_hvideo_display_unit);
  }
}

void
generic_packetizer_c::set_video_aspect_ratio(double aspect_ratio,
                                             bool is_factor,
                                             option_source_e source) {
  if (display_dimensions_or_aspect_ratio_set() && (m_ti.m_display_dimensions_source >= source))
    return;

  m_ti.m_aspect_ratio              = aspect_ratio;
  m_ti.m_aspect_ratio_is_factor    = is_factor;
  m_ti.m_display_unit              = ddu_pixels;
  m_ti.m_display_dimensions_source = source;
  m_ti.m_display_dimensions_given  = false;
  m_ti.m_aspect_ratio_given        = true;
}

void
generic_packetizer_c::set_language(mtx::bcp47::language_c const &language) {
  if (!language.is_valid())
    return;

  m_ti.m_language = language;

  if (!m_track_entry || !language.is_valid())
    return;

  get_child<libmatroska::KaxTrackLanguage>(m_track_entry).SetValue(language.get_closest_iso639_2_alpha_3_code());
  if (!mtx::bcp47::language_c::is_disabled())
    get_child<libmatroska::KaxLanguageIETF>(m_track_entry).SetValue(language.format());
}

void
generic_packetizer_c::set_video_pixel_cropping(int left,
                                               int top,
                                               int right,
                                               int bottom,
                                               option_source_e source) {
  m_ti.m_pixel_cropping.set(pixel_crop_t{left, top, right, bottom}, source);

  if (m_track_entry) {
    libmatroska::KaxTrackVideo &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto crop            = m_ti.m_pixel_cropping.get();

    get_child<libmatroska::KaxVideoPixelCropLeft  >(video).SetValue(crop.left);
    get_child<libmatroska::KaxVideoPixelCropTop   >(video).SetValue(crop.top);
    get_child<libmatroska::KaxVideoPixelCropRight >(video).SetValue(crop.right);
    get_child<libmatroska::KaxVideoPixelCropBottom>(video).SetValue(crop.bottom);
  }
}

void
generic_packetizer_c::set_video_color_matrix(uint64_t matrix_index,
                                             option_source_e source) {
  m_ti.m_color_matrix_coeff.set(matrix_index, source);
  if (   m_track_entry
      && (matrix_index <= 10)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourMatrix>(color).SetValue(m_ti.m_color_matrix_coeff.get());
  }
}

void
generic_packetizer_c::set_video_bits_per_channel(uint64_t num_bits,
                                                 option_source_e source) {
  m_ti.m_bits_per_channel.set(num_bits, source);
  if (m_track_entry) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoBitsPerChannel>(color).SetValue(m_ti.m_bits_per_channel.get());
  }
}

void
generic_packetizer_c::set_video_chroma_subsample(subsample_or_siting_t const &subsample,
                                                 option_source_e source) {
  m_ti.m_chroma_subsample.set(subsample_or_siting_t(subsample.hori, subsample.vert), source);
  if (   m_track_entry
      && (subsample.hori || subsample.vert)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    if (subsample.hori)
      get_child<libmatroska::KaxVideoChromaSubsampHorz>(color).SetValue(*subsample.hori);
    if (subsample.vert)
      get_child<libmatroska::KaxVideoChromaSubsampVert>(color).SetValue(*subsample.vert);
  }
}

void
generic_packetizer_c::set_video_cb_subsample(subsample_or_siting_t const &subsample,
                                             option_source_e source) {
  m_ti.m_cb_subsample.set(subsample_or_siting_t(subsample.hori, subsample.vert), source);
  if (   m_track_entry
      && (subsample.hori || subsample.vert)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    if (subsample.hori)
      get_child<libmatroska::KaxVideoCbSubsampHorz>(color).SetValue(*subsample.hori);
    if (subsample.vert)
      get_child<libmatroska::KaxVideoCbSubsampVert>(color).SetValue(*subsample.vert);
  }
}

void
generic_packetizer_c::set_video_chroma_siting(subsample_or_siting_t const &siting,
                                              option_source_e source) {
  m_ti.m_chroma_siting.set(subsample_or_siting_t(siting.hori, siting.vert), source);
  if (   m_track_entry
      && (   (siting.hori && (siting.hori >= 0) && (siting.hori <= 2))
          || (siting.vert && (siting.vert >= 0) && (siting.hori <= 2)))) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    if (siting.hori && (siting.hori >= 0) && (siting.hori <= 2))
      get_child<libmatroska::KaxVideoChromaSitHorz>(color).SetValue(*siting.hori);
    if (siting.vert && (siting.vert >= 0) && (siting.hori <= 2))
      get_child<libmatroska::KaxVideoChromaSitVert>(color).SetValue(*siting.vert);
  }
}

void
generic_packetizer_c::set_video_color_range(uint64_t range,
                                            option_source_e source) {
  m_ti.m_color_range.set(range, source);
  if (m_track_entry && (range <= 3)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourRange>(color).SetValue(range);
  }
}

void
generic_packetizer_c::set_video_color_transfer_character(uint64_t transfer_index,
                                                         option_source_e source) {
  m_ti.m_color_transfer.set(transfer_index, source);
  if (m_track_entry && (transfer_index <= 18)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourTransferCharacter>(color).SetValue(transfer_index);
  }
}

void
generic_packetizer_c::set_video_color_primaries(uint64_t primary_index,
                                                option_source_e source) {
  m_ti.m_color_primaries.set(primary_index, source);
  if (     m_track_entry
      && ((primary_index <= 10) || (primary_index == 22))) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourPrimaries>(color).SetValue(primary_index);
  }
}

void
generic_packetizer_c::set_video_max_cll(uint64_t max_cll,
                                        option_source_e source) {
  m_ti.m_max_cll.set(max_cll, source);
  if (m_track_entry) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourMaxCLL>(color).SetValue(max_cll);
  }
}

void
generic_packetizer_c::set_video_max_fall(uint64_t max_fall,
                                         option_source_e source) {
  m_ti.m_max_fall.set(max_fall, source);
  if (m_track_entry) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoColourMaxFALL>(color).SetValue(max_fall);
  }
}

void
generic_packetizer_c::set_video_chroma_coordinates(chroma_coordinates_t const &coordinates,
                                                   option_source_e source) {
  m_ti.m_chroma_coordinates.set(coordinates, source);
  if (   m_track_entry
      && (   (coordinates.red_x   && (coordinates.red_x   >= 0) && (coordinates.red_x   <= 1))
          || (coordinates.red_y   && (coordinates.red_y   >= 0) && (coordinates.red_y   <= 1))
          || (coordinates.green_x && (coordinates.green_x >= 0) && (coordinates.green_x <= 1))
          || (coordinates.green_y && (coordinates.green_y >= 0) && (coordinates.green_y <= 1))
          || (coordinates.blue_x  && (coordinates.blue_x  >= 0) && (coordinates.blue_x  <= 1))
          || (coordinates.blue_y  && (coordinates.blue_y  >= 0) && (coordinates.blue_y  <= 1)))) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    if (coordinates.red_x && (coordinates.red_x >= 0) && (coordinates.red_x <= 1))
      get_child<libmatroska::KaxVideoRChromaX>(color).SetValue(*coordinates.red_x);
    if (coordinates.red_y && (coordinates.red_y >= 0) && (coordinates.red_y <= 1))
      get_child<libmatroska::KaxVideoRChromaY>(color).SetValue(*coordinates.red_y);
    if (coordinates.green_x && (coordinates.green_x >= 0) && (coordinates.green_x <= 1))
      get_child<libmatroska::KaxVideoGChromaX>(color).SetValue(*coordinates.green_x);
    if (coordinates.green_y && (coordinates.green_y >= 0) && (coordinates.green_y <= 1))
      get_child<libmatroska::KaxVideoGChromaY>(color).SetValue(*coordinates.green_y);
    if (coordinates.blue_x && (coordinates.blue_x >= 0) && (coordinates.blue_x <= 1))
      get_child<libmatroska::KaxVideoBChromaX>(color).SetValue(*coordinates.blue_x);
    if (coordinates.blue_y && (coordinates.blue_y >= 0) && (coordinates.blue_y <= 1))
      get_child<libmatroska::KaxVideoBChromaY>(color).SetValue(*coordinates.blue_y);
  }
}

void
generic_packetizer_c::set_video_white_color_coordinates(white_color_coordinates_t const &coordinates,
                                                         option_source_e source) {
  m_ti.m_white_coordinates.set(white_color_coordinates_t(coordinates.hori, coordinates.vert), source);
  if (   m_track_entry
      && (   (coordinates.hori && (coordinates.hori >= 0) && (coordinates.hori <= 1))
          || (coordinates.vert && (coordinates.vert >= 0) && (coordinates.vert <= 1)))) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    if (coordinates.hori && (coordinates.hori >= 0) && (coordinates.hori <= 1))
      get_child<libmatroska::KaxVideoWhitePointChromaX>(color).SetValue(*coordinates.hori);
    if (coordinates.vert && (coordinates.vert >= 0) && (coordinates.vert <= 1))
      get_child<libmatroska::KaxVideoWhitePointChromaY>(color).SetValue(*coordinates.vert);
  }
}

void
generic_packetizer_c::set_video_max_luminance(double luminance,
                                              option_source_e source) {
  m_ti.m_max_luminance.set(luminance, source);
  if (   m_track_entry
      && (luminance >= 0)
      && (luminance <= 9999.99)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoLuminanceMax>(color).SetValue(luminance);
  }
}

void
generic_packetizer_c::set_video_min_luminance(double luminance,
                                              option_source_e source) {
  m_ti.m_min_luminance.set(luminance, source);
  if (m_track_entry && (luminance >= 0) && (luminance <= 999.9999)) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    auto &color = get_child<libmatroska::KaxVideoColour>(video);
    get_child<libmatroska::KaxVideoLuminanceMin>(color).SetValue(luminance);
  }
}

void
generic_packetizer_c::set_video_projection_type(uint64_t value,
                                                option_source_e source) {
  m_ti.m_projection_type.set(value, source);
  if (m_track_entry) {
    auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(get_child<libmatroska::KaxTrackVideo>(m_track_entry));
    get_child<libmatroska::KaxVideoProjectionType>(projection).SetValue(value);
  }
}

void
generic_packetizer_c::set_video_projection_private(memory_cptr const &value,
                                                   option_source_e source) {
  m_ti.m_projection_private.set(value, source);
  if (m_track_entry && value) {
    auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(get_child<libmatroska::KaxTrackVideo>(m_track_entry));
    if (value->get_size())
      get_child<libmatroska::KaxVideoProjectionPrivate>(projection).CopyBuffer(value->get_buffer(), value->get_size());
    else
      delete_children<libmatroska::KaxVideoProjectionPrivate>(projection);
  }
}

void
generic_packetizer_c::set_video_projection_pose_yaw(double value,
                                                    option_source_e source) {
  m_ti.m_projection_pose_yaw.set(value, source);
  if (m_track_entry && (value >= 0)) {
    auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(get_child<libmatroska::KaxTrackVideo>(m_track_entry));
    get_child<libmatroska::KaxVideoProjectionPoseYaw>(projection).SetValue(value);
  }
}

void
generic_packetizer_c::set_video_projection_pose_pitch(double value,
                                                      option_source_e source) {
  m_ti.m_projection_pose_pitch.set(value, source);
  if (m_track_entry && (value >= 0)) {
    auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(get_child<libmatroska::KaxTrackVideo>(m_track_entry));
    get_child<libmatroska::KaxVideoProjectionPosePitch>(projection).SetValue(value);
  }
}

void
generic_packetizer_c::set_video_projection_pose_roll(double value,
                                                     option_source_e source) {
  m_ti.m_projection_pose_roll.set(value, source);
  if (m_track_entry && (value >= 0)) {
    auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(get_child<libmatroska::KaxTrackVideo>(m_track_entry));
    get_child<libmatroska::KaxVideoProjectionPoseRoll>(projection).SetValue(value);
  }
}

void
generic_packetizer_c::set_video_pixel_cropping(const pixel_crop_t &cropping,
                                               option_source_e source) {
  set_video_pixel_cropping(cropping.left, cropping.top, cropping.right, cropping.bottom, source);
}

void
generic_packetizer_c::set_video_field_order(uint64_t order,
                                            option_source_e source) {
  m_ti.m_field_order.set(order, source);
  if (m_track_entry && m_ti.m_field_order) {
    auto &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);
    get_child<libmatroska::KaxVideoFieldOrder>(video).SetValue(m_ti.m_field_order.get());
  }
}

void
generic_packetizer_c::set_video_alpha_mode(bool alpha_mode,
                                           option_source_e source) {
  m_ti.m_alpha_mode.set(alpha_mode, source);

  if (m_track_entry)
    set_video_alpha_mode_impl(get_child<libmatroska::KaxTrackVideo>(*m_track_entry), m_ti.m_alpha_mode.get());
}

void
generic_packetizer_c::set_video_alpha_mode_impl(libebml::EbmlMaster &video,
                                                bool alpha_mode) {
  if (alpha_mode)
    get_child<libmatroska::KaxVideoAlphaMode>(video).SetValue(1);
  else
    delete_children<libmatroska::KaxVideoAlphaMode>(video);
}

void
generic_packetizer_c::set_video_stereo_mode(stereo_mode_c::mode stereo_mode,
                                            option_source_e source) {
  m_ti.m_stereo_mode.set(stereo_mode, source);

  if (m_track_entry && (stereo_mode_c::unspecified != m_ti.m_stereo_mode.get()))
    set_video_stereo_mode_impl(get_child<libmatroska::KaxTrackVideo>(*m_track_entry), m_ti.m_stereo_mode.get());
}

void
generic_packetizer_c::set_video_stereo_mode_impl(libebml::EbmlMaster &video,
                                                 stereo_mode_c::mode stereo_mode) {
  get_child<libmatroska::KaxVideoStereoMode>(video).SetValue(stereo_mode);
}

void
generic_packetizer_c::set_video_color_space(memory_cptr const &value,
                                             option_source_e source) {
  m_ti.m_color_space.set(value, source);

  if (m_track_entry && value && value->get_size())
    get_child<libmatroska::KaxVideoColourSpace>(m_track_entry).CopyBuffer(value->get_buffer(), value->get_size());
}

void
generic_packetizer_c::set_block_addition_mappings(std::vector<block_addition_mapping_t> const &mappings) {
  m_block_addition_mappings = mappings;
  apply_block_addition_mappings();
  update_max_block_addition_id();
}

void
generic_packetizer_c::update_max_block_addition_id() {
  if (!m_track_entry || outputting_webm() || m_block_addition_mappings.empty())
    return;

  auto new_max_block_add_id = std::accumulate(m_block_addition_mappings.begin(), m_block_addition_mappings.end(), m_max_block_add_id,
                                              [](auto max, auto const &mapping) { return std::max(max, mapping.id_value.value_or(1)); });

  if ((new_max_block_add_id > 0) && (new_max_block_add_id > m_max_block_add_id)) {
    m_max_block_add_id = new_max_block_add_id;
    get_child<libmatroska::KaxMaxBlockAdditionID>(m_track_entry).SetValue(m_max_block_add_id);
  }
}

bool
generic_packetizer_c::have_block_addition_mapping_type(uint64_t type)
  const {
  return std::find_if(m_block_addition_mappings.begin(), m_block_addition_mappings.end(),
                      [type](auto const &mapping) { return mapping.is_valid() && (*mapping.id_type == type); })
    != m_block_addition_mappings.end();
}

void
generic_packetizer_c::set_headers() {
  if (0 < m_connected_to) {
    mxerror(fmt::format("generic_packetizer_c::set_headers(): connected_to > 0 (type: {0}). {1}\n", typeid(*this).name(), BUGMSG));
    return;
  }

  bool found = false;
  size_t idx;
  for (idx = 0; ptzrs_in_header_order.size() > idx; ++idx)
    if (this == ptzrs_in_header_order[idx]) {
      found = true;
      break;
    }

  if (!found)
    ptzrs_in_header_order.push_back(this);

  if (!m_track_entry) {
    m_track_entry    = !g_kax_last_entry ? &get_child<libmatroska::KaxTrackEntry>(*g_kax_tracks) : &libebml::GetNextChild<libmatroska::KaxTrackEntry>(*g_kax_tracks, *g_kax_last_entry);
    g_kax_last_entry = m_track_entry;
    set_global_timestamp_scale(*m_track_entry, g_timestamp_scale);
    remove_deprecated_elements(*m_track_entry);
  }

  if (!m_hserialno && !m_reader->m_appending) {
    m_hserialno                             = ++ms_track_number;
    g_packetizers_by_track_num[m_hserialno] = this;
  }

  get_child<libmatroska::KaxTrackNumber>(m_track_entry).SetValue(m_hserialno);

  if (0 == m_huid)
    m_huid = create_unique_number(UNIQUE_TRACK_IDS);

  get_child<libmatroska::KaxTrackUID>(m_track_entry).SetValue(m_huid);

  if (-1 != m_htrack_type)
    get_child<libmatroska::KaxTrackType>(m_track_entry).SetValue(m_htrack_type);

  if (!m_hcodec_id.empty())
    get_child<libmatroska::KaxCodecID>(m_track_entry).SetValue(m_hcodec_id);

  if (m_hcodec_private)
    get_child<libmatroska::KaxCodecPrivate>(*m_track_entry).CopyBuffer(static_cast<uint8_t *>(m_hcodec_private->get_buffer()), m_hcodec_private->get_size());

  if (!m_hcodec_name.empty())
    get_child<libmatroska::KaxCodecName>(m_track_entry).SetValueUTF8(m_hcodec_name);

  update_max_block_addition_id();

  if (m_timestamp_factory && (m_htrack_type != track_subtitle))
    m_htrack_default_duration = (int64_t)m_timestamp_factory->get_default_duration(m_htrack_default_duration);
  if (-1.0 != m_htrack_default_duration)
    get_child<libmatroska::KaxTrackDefaultDuration>(m_track_entry).SetValue(m_htrack_default_duration);

  idx = track_type_to_deftrack_type(m_htrack_type);

  auto language = m_ti.m_language.is_valid() ? m_ti.m_language : g_default_language;
  get_child<libmatroska::KaxTrackLanguage>(m_track_entry).SetValue(language.get_closest_iso639_2_alpha_3_code());
  if (!mtx::bcp47::language_c::is_disabled())
    get_child<libmatroska::KaxLanguageIETF>(m_track_entry).SetValue(language.format());

  if (!m_ti.m_track_name.empty())
    get_child<libmatroska::KaxTrackName>(m_track_entry).SetValueUTF8(m_ti.m_track_name);

  if (m_ti.m_default_track.has_value())
    get_child<libmatroska::KaxTrackFlagDefault>(m_track_entry).SetValue(m_ti.m_default_track.value() ? 1 : 0);

  if (m_ti.m_forced_track.has_value())
    get_child<libmatroska::KaxTrackFlagForced>(m_track_entry).SetValue(m_ti.m_forced_track.value() ? 1 : 0);

  if (m_ti.m_enabled_track.has_value())
    get_child<libmatroska::KaxTrackFlagEnabled>(m_track_entry).SetValue(m_ti.m_enabled_track.value() ? 1 : 0);

  if (m_ti.m_hearing_impaired_flag.has_value())
    get_child<libmatroska::KaxFlagHearingImpaired>(m_track_entry).SetValue(m_ti.m_hearing_impaired_flag.value() ? 1 : 0);

  if (m_ti.m_visual_impaired_flag.has_value())
    get_child<libmatroska::KaxFlagVisualImpaired>(m_track_entry).SetValue(m_ti.m_visual_impaired_flag.value() ? 1 : 0);

  if (m_ti.m_text_descriptions_flag.has_value())
    get_child<libmatroska::KaxFlagTextDescriptions>(m_track_entry).SetValue(m_ti.m_text_descriptions_flag.value() ? 1 : 0);

  if (m_ti.m_original_flag.has_value())
    get_child<libmatroska::KaxFlagOriginal>(m_track_entry).SetValue(m_ti.m_original_flag.value() ? 1 : 0);

  if (m_ti.m_commentary_flag.has_value())
    get_child<libmatroska::KaxFlagCommentary>(m_track_entry).SetValue(m_ti.m_commentary_flag.value() ? 1 : 0);

  if (m_seek_pre_roll.valid())
    get_child<libmatroska::KaxSeekPreRoll>(m_track_entry).SetValue(m_seek_pre_roll.to_ns());

  if (m_codec_delay.valid())
    get_child<libmatroska::KaxCodecDelay>(m_track_entry).SetValue(m_codec_delay.to_ns());

  if (track_video == m_htrack_type) {
    libmatroska::KaxTrackVideo &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);

    if (-1 != m_hvideo_interlaced_flag)
      set_video_interlaced_flag(m_hvideo_interlaced_flag != 0);

    if ((-1 != m_hvideo_pixel_height) && (-1 != m_hvideo_pixel_width)) {
      if ((-1 == m_hvideo_display_width) || (-1 == m_hvideo_display_height) || m_ti.m_aspect_ratio_given || m_ti.m_display_dimensions_given) {
        if (m_ti.m_display_dimensions_given) {
          m_hvideo_display_width  = m_ti.m_display_width;
          m_hvideo_display_height = m_ti.m_display_height;

        } else {
          if (!m_ti.m_aspect_ratio_given)
            m_ti.m_aspect_ratio                = static_cast<double>(m_hvideo_pixel_width)                       / m_hvideo_pixel_height;

          else if (m_ti.m_aspect_ratio_is_factor && !m_ti.m_aspect_ratio_factor_applied) {
            m_ti.m_aspect_ratio                = static_cast<double>(m_hvideo_pixel_width) * m_ti.m_aspect_ratio / m_hvideo_pixel_height;
            m_ti.m_aspect_ratio_factor_applied = true;
          }

          if (m_ti.m_aspect_ratio > (static_cast<double>(m_hvideo_pixel_width) / m_hvideo_pixel_height)) {
            m_hvideo_display_width  = std::llround(m_hvideo_pixel_height * m_ti.m_aspect_ratio);
            m_hvideo_display_height = m_hvideo_pixel_height;

          } else {
            m_hvideo_display_width  = m_hvideo_pixel_width;
            m_hvideo_display_height = std::llround(m_hvideo_pixel_width / m_ti.m_aspect_ratio);
          }
        }
      }

      get_child<libmatroska::KaxVideoPixelWidth   >(video).SetValue(m_hvideo_pixel_width);
      get_child<libmatroska::KaxVideoPixelHeight  >(video).SetValue(m_hvideo_pixel_height);

      get_child<libmatroska::KaxVideoDisplayWidth >(video).SetValue(m_hvideo_display_width);
      get_child<libmatroska::KaxVideoDisplayHeight>(video).SetValue(m_hvideo_display_height);

      get_child<libmatroska::KaxVideoDisplayWidth >(video).SetDefaultSize(4);
      get_child<libmatroska::KaxVideoDisplayHeight>(video).SetDefaultSize(4);

      if (m_hvideo_display_unit != ddu_pixels)
        get_child<libmatroska::KaxVideoDisplayUnit>(video).SetValue(m_hvideo_display_unit);

      if (m_ti.m_color_space)
        get_child<libmatroska::KaxVideoColourSpace>(video).CopyBuffer(m_ti.m_color_space.get()->get_buffer(), m_ti.m_color_space.get()->get_size());

      if (m_ti.m_pixel_cropping) {
        auto crop = m_ti.m_pixel_cropping.get();
        get_child<libmatroska::KaxVideoPixelCropLeft  >(video).SetValue(crop.left);
        get_child<libmatroska::KaxVideoPixelCropTop   >(video).SetValue(crop.top);
        get_child<libmatroska::KaxVideoPixelCropRight >(video).SetValue(crop.right);
        get_child<libmatroska::KaxVideoPixelCropBottom>(video).SetValue(crop.bottom);
      }

      if (m_ti.m_color_matrix_coeff) {
        int color_matrix = m_ti.m_color_matrix_coeff.get();
        auto &color      = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourMatrix>(color).SetValue(color_matrix);
      }

      if (m_ti.m_bits_per_channel) {
        int bits    = m_ti.m_bits_per_channel.get();
        auto &color = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoBitsPerChannel>(color).SetValue(bits);
      }

      if (m_ti.m_chroma_subsample) {
        auto const &subsample = m_ti.m_chroma_subsample.get();
        auto &color           = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoChromaSubsampHorz>(color).SetValue(subsample.hori.value_or(0));
        get_child<libmatroska::KaxVideoChromaSubsampVert>(color).SetValue(subsample.vert.value_or(0));
      }

      if (m_ti.m_cb_subsample) {
        auto const &subsample = m_ti.m_cb_subsample.get();
        auto &color           = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoCbSubsampHorz>(color).SetValue(subsample.hori.value_or(0));
        get_child<libmatroska::KaxVideoCbSubsampVert>(color).SetValue(subsample.vert.value_or(0));
      }

      if (m_ti.m_chroma_siting) {
        auto const &siting = m_ti.m_chroma_siting.get();
        auto &color        = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoChromaSitHorz>(color).SetValue(siting.hori.value_or(0));
        get_child<libmatroska::KaxVideoChromaSitVert>(color).SetValue(siting.vert.value_or(0));
      }

      if (m_ti.m_color_range) {
        auto range_index = m_ti.m_color_range.get();
        auto &color      = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourRange>(color).SetValue(range_index);
      }

      if (m_ti.m_color_transfer) {
        auto transfer_index = m_ti.m_color_transfer.get();
        auto &color         = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourTransferCharacter>(color).SetValue(transfer_index);
      }

      if (m_ti.m_color_primaries) {
        auto primary_index = m_ti.m_color_primaries.get();
        auto &color        = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourPrimaries>(color).SetValue(primary_index);
      }

      if (m_ti.m_max_cll) {
        auto cll_index = m_ti.m_max_cll.get();
        auto &color    = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourMaxCLL>(color).SetValue(cll_index);
      }

      if (m_ti.m_max_fall) {
        auto fall_index = m_ti.m_max_fall.get();
        auto &color     = get_child<libmatroska::KaxVideoColour>(video);
        get_child<libmatroska::KaxVideoColourMaxFALL>(color).SetValue(fall_index);
      }

      if (m_ti.m_chroma_coordinates) {
        auto const &coordinates = m_ti.m_chroma_coordinates.get();
        auto &color             = get_child<libmatroska::KaxVideoColour>(video);
        auto &master_meta       = get_child<libmatroska::KaxVideoColourMasterMeta>(color);
        get_child<libmatroska::KaxVideoRChromaX>(master_meta).SetValue(coordinates.red_x.value_or(0));
        get_child<libmatroska::KaxVideoRChromaY>(master_meta).SetValue(coordinates.red_y.value_or(0));
        get_child<libmatroska::KaxVideoGChromaX>(master_meta).SetValue(coordinates.green_x.value_or(0));
        get_child<libmatroska::KaxVideoGChromaY>(master_meta).SetValue(coordinates.green_y.value_or(0));
        get_child<libmatroska::KaxVideoBChromaX>(master_meta).SetValue(coordinates.blue_x.value_or(0));
        get_child<libmatroska::KaxVideoBChromaY>(master_meta).SetValue(coordinates.blue_y.value_or(0));
      }

      if (m_ti.m_white_coordinates) {
        auto const &coordinates = m_ti.m_white_coordinates.get();
        auto &color             = get_child<libmatroska::KaxVideoColour>(video);
        auto &master_meta       = get_child<libmatroska::KaxVideoColourMasterMeta>(color);
        get_child<libmatroska::KaxVideoWhitePointChromaX>(master_meta).SetValue(coordinates.hori.value_or(0));
        get_child<libmatroska::KaxVideoWhitePointChromaY>(master_meta).SetValue(coordinates.vert.value_or(0));
      }

      if (m_ti.m_max_luminance) {
        auto luminance    = m_ti.m_max_luminance.get();
        auto &color       = get_child<libmatroska::KaxVideoColour>(video);
        auto &master_meta = get_child<libmatroska::KaxVideoColourMasterMeta>(color);
        get_child<libmatroska::KaxVideoLuminanceMax>(master_meta).SetValue(luminance);
      }

      if (m_ti.m_min_luminance) {
        auto luminance    = m_ti.m_min_luminance.get();
        auto &color       = get_child<libmatroska::KaxVideoColour>(video);
        auto &master_meta = get_child<libmatroska::KaxVideoColourMasterMeta>(color);
        get_child<libmatroska::KaxVideoLuminanceMin>(master_meta).SetValue(luminance);
      }

      if (m_ti.m_projection_type) {
        auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(video);
        get_child<libmatroska::KaxVideoProjectionType>(projection).SetValue(m_ti.m_projection_type.get());
      }

      if (m_ti.m_projection_private) {
        auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(video);
        get_child<libmatroska::KaxVideoProjectionPrivate>(projection).CopyBuffer(m_ti.m_projection_private.get()->get_buffer(), m_ti.m_projection_private.get()->get_size());
      }

      if (m_ti.m_projection_pose_yaw) {
        auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(video);
        get_child<libmatroska::KaxVideoProjectionPoseYaw>(projection).SetValue(m_ti.m_projection_pose_yaw.get());
      }

      if (m_ti.m_projection_pose_pitch) {
        auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(video);
        get_child<libmatroska::KaxVideoProjectionPosePitch>(projection).SetValue(m_ti.m_projection_pose_pitch.get());
      }

      if (m_ti.m_projection_pose_roll) {
        auto &projection = get_child_empty_if_new<libmatroska::KaxVideoProjection>(video);
        get_child<libmatroska::KaxVideoProjectionPoseRoll>(projection).SetValue(m_ti.m_projection_pose_roll.get());
      }

      if (m_ti.m_field_order)
        get_child<libmatroska::KaxVideoFieldOrder>(video).SetValue(m_ti.m_field_order.get());

      if (m_ti.m_stereo_mode && (stereo_mode_c::unspecified != m_ti.m_stereo_mode.get()))
        set_video_stereo_mode_impl(video, m_ti.m_stereo_mode.get());

      if (m_ti.m_alpha_mode && m_ti.m_alpha_mode.get())
        set_video_alpha_mode_impl(video, m_ti.m_alpha_mode.get());
    }

  } else if (track_audio == m_htrack_type) {
    libmatroska::KaxTrackAudio &audio = get_child<libmatroska::KaxTrackAudio>(m_track_entry);

    if (-1   != m_haudio_sampling_freq)
      get_child<libmatroska::KaxAudioSamplingFreq>(audio).SetValue(m_haudio_sampling_freq);

    if (-1.0 != m_haudio_output_sampling_freq)
      get_child<libmatroska::KaxAudioOutputSamplingFreq>(audio).SetValue(m_haudio_output_sampling_freq);

    if (-1   != m_haudio_channels)
      get_child<libmatroska::KaxAudioChannels>(audio).SetValue(m_haudio_channels);

    if (-1   != m_haudio_bit_depth)
      get_child<libmatroska::KaxAudioBitDepth>(audio).SetValue(m_haudio_bit_depth);

    if (m_ti.m_audio_emphasis && (audio_emphasis_c::mode_e::none != m_ti.m_audio_emphasis.get()))
      set_audio_emphasis_impl(audio, m_ti.m_audio_emphasis.get());

  } else if (track_buttons == m_htrack_type) {
    if ((-1 != m_hvideo_pixel_height) && (-1 != m_hvideo_pixel_width)) {
      libmatroska::KaxTrackVideo &video = get_child<libmatroska::KaxTrackVideo>(m_track_entry);

      get_child<libmatroska::KaxVideoPixelWidth >(video).SetValue(m_hvideo_pixel_width);
      get_child<libmatroska::KaxVideoPixelHeight>(video).SetValue(m_hvideo_pixel_height);
    }

  }

  if ((COMPRESSION_UNSPECIFIED != m_hcompression) && (COMPRESSION_NONE != m_hcompression)) {
    libmatroska::KaxContentEncoding &c_encoding = get_child<libmatroska::KaxContentEncoding>(get_child<libmatroska::KaxContentEncodings>(m_track_entry));

    get_child<libmatroska::KaxContentEncodingOrder>(c_encoding).SetValue(0); // First modification.
    get_child<libmatroska::KaxContentEncodingType >(c_encoding).SetValue(0); // It's a compression.
    get_child<libmatroska::KaxContentEncodingScope>(c_encoding).SetValue(1); // Only the frame contents have been compresed.

    m_compressor = compressor_c::create(m_hcompression);
    m_compressor->set_track_headers(c_encoding);
  }

  apply_block_addition_mappings();

  if (g_no_lacing)
    m_track_entry->EnableLacing(false);

  set_tag_track_uid();
  if (m_ti.m_tags) {
    while (m_ti.m_tags->ListSize() != 0) {
      add_tags(static_cast<libmatroska::KaxTag &>(*(*m_ti.m_tags)[0]));
      m_ti.m_tags->Remove(0);
    }
  }
}

void
generic_packetizer_c::apply_block_addition_mappings() {
  if (outputting_webm() || !m_track_entry || m_block_addition_mappings.empty())
    return;

  std::sort(m_block_addition_mappings.begin(), m_block_addition_mappings.end(),
            [](auto const &a, auto const &b) { return a.id_value.value_or(1) < b.id_value.value_or(1); });

  delete_children<libmatroska::KaxBlockAdditionMapping>(m_track_entry);

  for (auto const &mapping : m_block_addition_mappings) {
    if (!mapping.is_valid())
      continue;

    auto &kmapping = add_empty_child<libmatroska::KaxBlockAdditionMapping>(m_track_entry);

    if (!mapping.id_name.empty())
      get_child<libmatroska::KaxBlockAddIDName>(kmapping).SetValue(mapping.id_name);

    if (mapping.id_type)
      get_child<libmatroska::KaxBlockAddIDType>(kmapping).SetValue(*mapping.id_type);

    if (mapping.id_value)
      get_child<libmatroska::KaxBlockAddIDValue>(kmapping).SetValue(*mapping.id_value);

    if (mapping.id_extra_data && mapping.id_extra_data->get_size())
      get_child<libmatroska::KaxBlockAddIDExtraData>(kmapping).CopyBuffer(mapping.id_extra_data->get_buffer(), mapping.id_extra_data->get_size());
  }
}

void
generic_packetizer_c::fix_headers() {
  set_global_timestamp_scale(*m_track_entry, g_timestamp_scale);
}

void
generic_packetizer_c::compress_packet(packet_t &packet) {
  if (!m_compressor) {
    return;
  }

  try {
    packet.data = m_compressor->compress(packet.data);
    size_t i;
    for (i = 0; packet.data_adds.size() > i; ++i)
      packet.data_adds[i].data = m_compressor->compress(packet.data_adds[i].data);

  } catch (mtx::compression_x &e) {
    mxerror_tid(m_ti.m_fname, m_ti.m_id, fmt::format(FY("Compression failed: {0}\n"), e.error()));
  }
}

void
generic_packetizer_c::account_enqueued_bytes(packet_t &packet,
                                             int64_t factor) {
  m_enqueued_bytes += packet.calculate_uncompressed_size() * factor;
}

void
generic_packetizer_c::add_packet(packet_cptr const &pack) {
  mxdebug_if(s_debug, fmt::format("add_packet() track {0} timestamp {1} m_connected_to {2}\n", get_source_track_num(), mtx::string::format_timestamp(pack->timestamp), m_connected_to));

  if (m_htrack_type == track_subtitle)
    pack->duration_mandatory = pack->duration >= 0;

  if ((0 == m_num_packets) && m_ti.m_reset_timestamps)
    m_ti.m_tcsync.displacement = -pack->timestamp;

  ++m_num_packets;

  if (!m_reader->m_ptzr_first_packet)
    m_reader->m_ptzr_first_packet = this;

  pack->data->take_ownership();
  for (auto &data_add : pack->data_adds)
    data_add.data->take_ownership();

  pack->source = this;

  // Although this is no longer triggered by "broken" B frames, it does not seem sensical at all
  if ((0 > pack->bref) && (0 <= pack->fref))
    std::swap(pack->bref, pack->fref);

  account_enqueued_bytes(*pack, +1);

  if (!outputting_webm() && !pack->data_adds.empty()) {
    auto new_max = std::accumulate(pack->data_adds.begin(), pack->data_adds.end(), m_max_block_add_id,
                                   [](auto max, auto const &add) { return std::max(max, add.id.value_or(1)); });

    if (new_max > m_max_block_add_id) {
      m_max_block_add_id = new_max;
      get_child<libmatroska::KaxMaxBlockAdditionID>(m_track_entry).SetValue(m_max_block_add_id);

      rerender_track_headers();
    }
  }

  if (1 != m_connected_to)
    add_packet2(pack);
  else
    m_deferred_packets.push_back(pack);
}

void
generic_packetizer_c::add_packet2(packet_cptr const &pack) {
  mxdebug_if(s_debug, fmt::format("add_packet() track {0} ap2 m_timestamp_factory_application_mode {1}\n", get_source_track_num(), static_cast<int>(m_timestamp_factory_application_mode)));
  auto adjust_timestamp = [this](int64_t x) {
    return mtx::to_int(m_ti.m_tcsync.factor * (x + m_correction_timestamp_offset + m_append_timestamp_offset)) + m_ti.m_tcsync.displacement;
  };

  pack->timestamp = adjust_timestamp(pack->timestamp);
  if (pack->has_bref())
    pack->bref = adjust_timestamp(pack->bref);
  if (pack->has_fref())
    pack->fref = adjust_timestamp(pack->fref);
  if (pack->has_duration()) {
    pack->duration = mtx::to_int(m_ti.m_tcsync.factor * pack->duration);
    if (pack->has_discard_padding())
      pack->duration -= std::min(pack->duration, pack->discard_padding.to_ns());
  }

  if (0 > pack->timestamp)
    return;

  // 'timestamp < safety_last_timestamp' may only occur for B frames. In this
  // case we have the coding order, e.g. IPB1B2 and the timestamps
  // I: 0, P: 120, B1: 40, B2: 80.
  if (!m_relaxed_timestamp_checking && (pack->timestamp < m_safety_last_timestamp) && (0 > pack->fref) && mtx::hacks::is_engaged(mtx::hacks::ENABLE_TIMESTAMP_WARNING)) {
    if (track_audio == m_htrack_type) {
      int64_t needed_timestamp_offset  = m_safety_last_timestamp + m_safety_last_duration - pack->timestamp;
      m_correction_timestamp_offset   += needed_timestamp_offset;
      pack->timestamp                 += needed_timestamp_offset;
      if (pack->has_bref())
        pack->bref += needed_timestamp_offset;
      if (pack->has_fref())
        pack->fref += needed_timestamp_offset;

      mxwarn_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format(FY("The current packet's timestamp is smaller than that of the previous packet. "
                                "This usually means that the source file is a Matroska file that has not been created 100% correctly. "
                                "The timestamps of all packets will be adjusted by {0}ms in order not to lose any data. "
                                "This may throw audio/video synchronization off, but that can be corrected with mkvmerge's \"--sync\" option. "
                                "If you already use \"--sync\" and you still get this warning then do NOT worry -- this is normal. "
                                "If this error happens more than once and you get this message more than once for a particular track "
                                "then either is the source file badly mastered, or mkvmerge contains a bug. "
                                "In this case you should open an issue over on {1}.\n"),
                             (needed_timestamp_offset + 500000) / 1000000, MTX_URL_ISSUES));

    } else
      mxwarn_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format("generic_packetizer_c::add_packet2: timestamp < last_timestamp ({0} < {1}). {2}\n",
                             mtx::string::format_timestamp(pack->timestamp), mtx::string::format_timestamp(m_safety_last_timestamp), BUGMSG));
  }

  m_safety_last_timestamp        = pack->timestamp;
  m_safety_last_duration         = pack->duration;
  pack->timestamp_before_factory = pack->timestamp;

  m_packet_queue.push_back(pack);
  if (!m_timestamp_factory || (TFA_IMMEDIATE == m_timestamp_factory_application_mode))
    apply_factory_once(pack);
  else
    apply_factory();

  after_packet_timestamped(*pack);

  compress_packet(*pack);
}

void
generic_packetizer_c::process_deferred_packets() {
  for (auto &packet : m_deferred_packets)
    add_packet2(packet);
  m_deferred_packets.clear();
}

packet_cptr
generic_packetizer_c::get_packet() {
  if (m_packet_queue.empty() || !m_packet_queue.front()->factory_applied)
    return packet_cptr{};

  packet_cptr pack = m_packet_queue.front();
  m_packet_queue.pop_front();

  pack->output_order_timestamp = timestamp_c::ns(pack->assigned_timestamp - std::max(m_codec_delay.to_ns(0), m_seek_pre_roll.to_ns(0)));

  account_enqueued_bytes(*pack, -1);

  --m_next_packet_wo_assigned_timestamp;
  if (0 > m_next_packet_wo_assigned_timestamp)
    m_next_packet_wo_assigned_timestamp = 0;

  return pack;
}

void
generic_packetizer_c::apply_factory_once(packet_cptr const &packet) {
  if (!m_timestamp_factory) {
    packet->assigned_timestamp = packet->timestamp;
    packet->gap_following      = false;

  } else {
    packet->gap_following      = m_timestamp_factory->get_next(*packet);
    if (m_htrack_type == track_subtitle)
      packet->duration_mandatory = packet->duration >= 0;
  }

  packet->factory_applied      = true;

  mxdebug_if(s_debug, fmt::format("apply_factory_once(): source {0} timestamp {1} before factory {2} duration {3} at {4}\n",
                                  packet->source->get_source_track_num(), mtx::string::format_timestamp(packet->timestamp), mtx::string::format_timestamp(packet->timestamp_before_factory),
                                  mtx::string::format_timestamp(packet->duration), mtx::string::format_timestamp(packet->assigned_timestamp)));

  m_max_timestamp_seen           = std::max(m_max_timestamp_seen, packet->assigned_timestamp + packet->duration);
  m_reader->m_max_timestamp_seen = std::max(m_max_timestamp_seen, m_reader->m_max_timestamp_seen);

  ++m_next_packet_wo_assigned_timestamp;
}

void
generic_packetizer_c::apply_factory() {
  if (m_packet_queue.empty())
    return;

  // Find the first packet to which the factory hasn't been applied yet.
  packet_cptr_di p_start = m_packet_queue.begin() + m_next_packet_wo_assigned_timestamp;

  while ((m_packet_queue.end() != p_start) && (*p_start)->factory_applied)
    ++p_start;

  if (m_packet_queue.end() == p_start)
    return;

  if (TFA_SHORT_QUEUEING == m_timestamp_factory_application_mode)
    apply_factory_short_queueing(p_start);

  else
    apply_factory_full_queueing(p_start);
}

void
generic_packetizer_c::apply_factory_short_queueing(packet_cptr_di &p_start) {
  while (m_packet_queue.end() != p_start) {
    // Find the next packet with a timestamp bigger than the start packet's
    // timestamp. All packets between those two including the start packet
    // and excluding the end packet can be timestamped.
    packet_cptr_di p_end = p_start + 1;
    while ((m_packet_queue.end() != p_end) && ((*p_end)->timestamp_before_factory < (*p_start)->timestamp_before_factory))
      ++p_end;

    // Abort if no such packet was found, but keep on assigning if the
    // packetizer has been flushed already.
    if (!m_has_been_flushed && (m_packet_queue.end() == p_end))
      return;

    // Now assign timestamps to the ones between p_start and p_end...
    packet_cptr_di p_current;
    for (p_current = p_start + 1; p_current != p_end; ++p_current)
      apply_factory_once(*p_current);
    // ...and to p_start itself.
    apply_factory_once(*p_start);

    p_start = p_end;
  }
}

struct packet_sorter_t {
  int m_index;
  static std::deque<packet_cptr> *m_packet_queue;

  packet_sorter_t(int index)
    : m_index(index)
  {
  }

  bool operator <(const packet_sorter_t &cmp) const {
    return (*m_packet_queue)[m_index]->timestamp < (*m_packet_queue)[cmp.m_index]->timestamp;
  }
};

std::deque<packet_cptr> *packet_sorter_t::m_packet_queue = nullptr;

void
generic_packetizer_c::apply_factory_full_queueing(packet_cptr_di &p_start) {
  mxdebug_if(s_debug, fmt::format("add_packet() track {0} fac full q\n", get_source_track_num()));
  packet_sorter_t::m_packet_queue = &m_packet_queue;

  while (m_packet_queue.end() != p_start) {
    // Find the next I frame packet.
    packet_cptr_di p_end = p_start + 1;
    while ((m_packet_queue.end() != p_end) && !(*p_end)->is_key_frame())
      ++p_end;

    // Abort if no such packet was found, but keep on assigning if the
    // packetizer has been flushed already.
    if (!m_has_been_flushed && (m_packet_queue.end() == p_end))
      return;

    // Now sort the frames by their timestamp as the factory has to be
    // applied to the packets in the same order as they're timestamped.
    std::vector<packet_sorter_t> sorter;
    bool needs_sorting         = false;
    int64_t previous_timestamp = 0;
    size_t i                   = distance(m_packet_queue.begin(), p_start);

    packet_cptr_di p_current;
    for (p_current = p_start; p_current != p_end; ++i, ++p_current) {
      sorter.push_back(packet_sorter_t(i));
      if (m_packet_queue[i]->timestamp < previous_timestamp)
        needs_sorting = true;
      previous_timestamp = m_packet_queue[i]->timestamp;
    }

    if (needs_sorting)
      std::sort(sorter.begin(), sorter.end());

    // Finally apply the factory.
    for (i = 0; sorter.size() > i; ++i)
      apply_factory_once(m_packet_queue[sorter[i].m_index]);

    p_start = p_end;
  }
}

void
generic_packetizer_c::force_duration_on_last_packet() {
  if (m_packet_queue.empty()) {
    mxdebug_if(s_debug, fmt::format("'{0}' track {1}: force_duration_on_last_packet: packet queue is empty\n", m_ti.m_fname, m_ti.m_id));
    return;
  }

  packet_cptr &packet        = m_packet_queue.back();
  packet->duration_mandatory = true;

  mxdebug_if(s_debug, fmt::format("'{0}' track {1}: force_duration_on_last_packet: forcing at {2} with {3:.3f}ms\n", m_ti.m_fname, m_ti.m_id, mtx::string::format_timestamp(packet->timestamp), packet->duration / 1000.0));
}

int64_t
generic_packetizer_c::calculate_avi_audio_sync(int64_t num_bytes,
                                               int64_t samples_per_packet,
                                               int64_t packet_duration) {
  if (!m_ti.m_avi_audio_sync_enabled || mtx::hacks::is_engaged(mtx::hacks::NO_DELAY_FOR_GARBAGE_IN_AVI))
    return -1;

  if (m_ti.m_avi_audio_data_rate)
    return num_bytes * 1000000000 / m_ti.m_avi_audio_data_rate;

  return ((num_bytes + samples_per_packet - 1) / samples_per_packet) * packet_duration;
}

void
generic_packetizer_c::connect(generic_packetizer_c *src,
                              int64_t append_timestamp_offset) {
  m_free_refs                   = src->m_free_refs;
  m_next_free_refs              = src->m_next_free_refs;
  m_track_entry                 = src->m_track_entry;
  m_hserialno                   = src->m_hserialno;
  m_htrack_type                 = src->m_htrack_type;
  m_htrack_default_duration     = src->m_htrack_default_duration;
  m_huid                        = src->m_huid;
  m_hcompression                = src->m_hcompression;
  m_compressor                  = compressor_c::create(m_hcompression);
  m_last_cue_timestamp          = src->m_last_cue_timestamp;
  m_timestamp_factory           = src->m_timestamp_factory;
  m_correction_timestamp_offset = 0;

  if (-1 == append_timestamp_offset)
    m_append_timestamp_offset = src->m_max_timestamp_seen;
  else
    m_append_timestamp_offset = append_timestamp_offset;

  m_connected_to++;
  if (2 == m_connected_to) {
    process_deferred_packets();
    src->m_connected_successor = this;
  }
}

split_result_e
generic_packetizer_c::can_be_split(std::string &/* error_message */) {
  return CAN_SPLIT_YES;
}

void
generic_packetizer_c::set_displacement_maybe(int64_t displacement) {
  if ((1 == m_ti.m_tcsync.factor) && (0 == m_ti.m_tcsync.displacement))
    m_ti.m_tcsync.displacement = displacement;
}

bool
generic_packetizer_c::contains_gap() {
  return m_timestamp_factory ? m_timestamp_factory->contains_gap() : false;
}

void
generic_packetizer_c::flush() {
  flush_impl();

  m_has_been_flushed = true;
  apply_factory();
}

bool
generic_packetizer_c::display_dimensions_or_aspect_ratio_set() {
  return m_ti.display_dimensions_or_aspect_ratio_set();
}

bool
generic_packetizer_c::is_compatible_with(output_compatibility_e compatibility) {
  return OC_MATROSKA == compatibility;
}

void
generic_packetizer_c::discard_queued_packets() {
  m_packet_queue.clear();
  m_enqueued_bytes = 0;
}

bool
generic_packetizer_c::wants_cue_duration()
  const {
  return get_track_type() == track_subtitle;
}

void
generic_packetizer_c::show_experimental_status_version(std::string const &codec_id) {
  auto idx = get_format_name().get_untranslated();
  if (s_experimental_status_warning_shown[idx])
    return;

  s_experimental_status_warning_shown[idx] = true;
  mxwarn(fmt::format(FY("Note that the Matroska specifications regarding the storage of '{0}' have not been finalized yet. "
                       "mkvmerge's support for it is therefore subject to change and uses the CodecID '{1}/EXPERIMENTAL' instead of '{1}'. "
                       "This warning will be removed once the specifications have been finalized and mkvmerge has been updated accordingly.\n"),
                     get_format_name().get_translated(), codec_id));
}

file_status_e
generic_packetizer_c::read(bool force) {
  return m_reader->read_next(this, force);
}

void
generic_packetizer_c::process(packet_cptr const &packet) {
  process_impl(packet);
}

void
generic_packetizer_c::prevent_lacing() {
  m_prevent_lacing = true;
}

bool
generic_packetizer_c::is_lacing_prevented()
  const {
  return m_prevent_lacing;
}

void
generic_packetizer_c::after_packet_timestamped(packet_t &) {
}

void
generic_packetizer_c::after_packet_rendered(packet_t const &) {
}

void
generic_packetizer_c::before_file_finished() {
}

void
generic_packetizer_c::after_file_created() {
}

generic_packetizer_c *
generic_packetizer_c::get_connected_successor()
  const {
  return m_connected_successor;
}

void
generic_packetizer_c::set_source_id(std::string const &source_id) {
  m_source_id = source_id;
}

std::string
generic_packetizer_c::get_source_id()
  const {
  return m_source_id;
}
