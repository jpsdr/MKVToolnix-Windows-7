/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for DTS data

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/codec.h"
#include "common/timestamp.h"

namespace mtx::bits {
class reader_c;
}

namespace mtx::dts {

enum class sync_word_e {
    core = 0x7ffe8001
  , exss = 0x64582025
  , lbr  = 0x0a801921
  , xll  = 0x41a29547
  , x96  = 0x1d95f262
  , xch  = 0x5a5a5a5a
};

enum class frametype_e {
  // Used to extremely precisely specify the end-of-stream (single PCM
  // sample resolution).
    termination = 0
  , normal
 };

enum class extension_audio_descriptor_e {
    xch = 0                      // channel extension
  , unknown1
  , x96k                         // frequency extension
  , xch_x96k                     // both channel and frequency extension
  , unknown4
  , unknown5
  , unknown6
  , unknown7
};

enum extension_mask_e {
    css_core  = 0x001
  , css_xxch  = 0x002
  , css_x96   = 0x004
  , css_xch   = 0x008
  , exss_core = 0x010
  , exss_xbr  = 0x020
  , exss_xxch = 0x040
  , exss_x96  = 0x080
  , exss_lbr  = 0x100
  , exss_xll  = 0x200
  , exss_rsv1 = 0x400
  , exss_rsv2 = 0x800
};

enum class lfe_type_e {
    none
  , lfe_128 // 128 indicates the interpolation factor to reconstruct the lfe channel
  , lfe_64  //  64 indicates the interpolation factor to reconstruct the lfe channel
  , invalid
};

enum class multirate_interpolator_e {
    non_perfect
  , perfect
};

enum class dts_type_e {
    normal
  , high_resolution
  , master_audio
  , express
  , es
  , x96_24
};

enum class source_pcm_resolution_e {
    spr_16 = 0
  , spr_16_ES  //_ES means: surround channels mastered in DTS-ES
  , spr_20
  , spr_20_ES
  , spr_invalid4
  , spr_24_ES
  , spr_24
  , spr_invalid7
};

enum class lbr_format_info_code_e {
    sync_only    = 1
  , decoder_init = 2
};

static const int64_t max_packet_size = 15384;

struct header_t {
  frametype_e frametype{ frametype_e::normal };

  // 0 for normal frames, 1 to 30 for termination frames. Number of PCM
  // samples the frame is shorter than normal.
  unsigned int deficit_sample_count{};

  // If true, a CRC-sum is included in the data.
  bool crc_present{};

  // number of PCM core sample blocks in this frame. Each PCM core sample block
  // consists of 32 samples. Notice that "core samples" means "samples
  // after the input decimator", so at sampling frequencies >48kHz, one core
  // sample represents 2 (or 4 for frequencies >96kHz) output samples.
  unsigned int num_pcm_sample_blocks{};

  // Number of bytes this frame occupies (range: 95 to 16 383).
  unsigned int frame_byte_size{};

  // Number of audio channels, -1 for "unknown".
  int audio_channels{};

  // String describing the audio channel arrangement
  const char *audio_channel_arrangement{};

  // -1 for "invalid"
  unsigned int core_sampling_frequency{};
  std::optional<unsigned int> extension_sampling_frequency;

  // in bit per second, or -1 == "open", -2 == "variable", -3 == "lossless"
  int transmission_bitrate{};

  // if true, sub-frames contain coefficients for downmixing to stereo
  bool embedded_down_mix{};

  // if true, sub-frames contain coefficients for dynamic range correction
  bool embedded_dynamic_range{};

  // if true, a time stamp is embedded at the end of the core audio data
  bool embedded_time_stamp{};

  // if true, auxiliary data is appended at the end of the core audio data
  bool auxiliary_data{};

  // if true, the source material was mastered in HDCD format
  bool hdcd_master{};

  extension_audio_descriptor_e extension_audio_descriptor{ extension_audio_descriptor_e::xch }; // significant only if extended_coding == true

  // if true, extended coding data is placed after the core audio data
  bool extended_coding{};

  // if true, audio data check words are placed in each sub-sub-frame
  // rather than in each sub-frame, only
  bool audio_sync_word_in_sub_sub{};

  lfe_type_e lfe_type{ lfe_type_e::none };

  // if true, past frames will be used to predict ADPCM values for the
  // current one. This means, if this flag is false, the current frame is
  // better suited as an audio-jump-point (like an "I-frame" in video-coding).
  bool predictor_history_flag{};

  // which FIR coefficients to use for sub-band reconstruction
  multirate_interpolator_e multirate_interpolator{ multirate_interpolator_e::non_perfect };

  // 0 to 15
  unsigned int encoder_software_revision{};

  // 0 to 3 - "top-secret" bits indicating the "copy history" of the material
  unsigned int copy_history{};

  // 16, 20 or 24 bits per sample, or -1 == invalid
  int source_pcm_resolution{};

  // if true, source surround channels are mastered in DTS-ES
  bool source_surround_in_es{};

  // if true, left and right front channels are encoded as
  // sum and difference (L = L + R, R = L - R)
  bool front_sum_difference{};

  // same as front_sum_difference for surround left and right channels
  bool surround_sum_difference{};

  // gain in dB to apply for dialog normalization
  int dialog_normalization_gain{}, extension_dialog_normalization_gain{};
  unsigned int dialog_normalization_gain_bit_position{}, extension_dialog_normalization_gain_bit_position{};

  std::optional<unsigned int> crc{};

  bool has_core{}, has_exss{}, has_xch{};
  unsigned int exss_offset{}, exss_header_size{}, exss_part_size{};

  dts_type_e dts_type{ dts_type_e::normal };

  bool static_fields_present{}, mix_metadata_enabled{};
  unsigned int reference_clock_code{}, substream_frame_duration{};
  unsigned int substream_size_bits{}, num_presentations{1}, num_assets{1}, num_mixing_configurations{};
  unsigned int num_mixing_channels[5];

  struct substream_asset_t {
    std::size_t asset_offset{}, asset_size{}, asset_index{};

    unsigned int pcm_bit_res{}, max_sample_rate{}, num_channels_total{};
    bool one_to_one_map_channel_to_speaker{}, embedded_stereo{}, embedded_6ch{};
    int representation_type{};

    int coding_mode{};
    extension_mask_e extension_mask{};

    std::size_t core_offset{}, core_size{};
    std::size_t xbr_offset{},  xbr_size{};
    std::size_t xxch_offset{}, xxch_size{};
    std::size_t x96_offset{},  x96_size{};
    std::size_t lbr_offset{},  lbr_size{};
    std::size_t xll_offset{},  xll_size{};

    bool lbr_sync_present{}, xll_sync_present{};
    int xll_delay_num_frames{};
    std::size_t xll_sync_offset{};

    unsigned int hd_stream_id{};
  };

  std::vector<substream_asset_t> substream_assets;

public:
  uint64_t get_packet_length_in_core_samples() const;
  timestamp_c get_packet_length_in_nanoseconds() const;

  unsigned int get_core_num_audio_channels() const;
  unsigned int get_total_num_audio_channels() const;
  codec_c::specialization_e get_codec_specialization() const;
  unsigned int get_effective_sampling_frequency() const;

  void print() const;

  bool decode_core_header(uint8_t const *buf, std::size_t size, bool allow_no_exss_search = false);
  bool decode_exss_header(uint8_t const *buf, std::size_t size);
  bool decode_x96_header(uint8_t const *buf, std::size_t size);

protected:
  bool decode_asset(mtx::bits::reader_c &bc, substream_asset_t &asset);
  bool decode_lbr_header(mtx::bits::reader_c &bc, substream_asset_t &asset);
  bool decode_xll_header(mtx::bits::reader_c &bc, substream_asset_t &asset);
  void parse_lbr_parameters(mtx::bits::reader_c &bc, substream_asset_t &asset);
  void parse_xll_parameters(mtx::bits::reader_c &bc, substream_asset_t &asset);
  void locate_and_decode_xch_header(uint8_t const *buf, std::size_t size);

  bool set_one_extension_offset(substream_asset_t &asset, extension_mask_e wanted_mask, std::size_t &offset, std::size_t &size, std::size_t &offset_in_asset, std::size_t size_in_asset);
  bool set_extension_offsets(substream_asset_t &asset);
};

int find_sync_word(uint8_t const *buf, std::size_t size);
int find_header(uint8_t const *buf, std::size_t size, header_t &header, bool allow_no_exss_search = false);
int find_consecutive_headers(uint8_t const *buf, std::size_t size, unsigned int num);

bool operator ==(header_t const &h1, header_t const &h2);
bool operator!=(header_t const &h1, header_t const &h2);

void convert_14_to_16_bits(const unsigned short *src, unsigned long srcwords, unsigned short *dst);

bool detect(const void *src_buf, int len, bool &convert_14_to_16, bool &swap_bytes);

void remove_dialog_normalization_gain(uint8_t *buf, std::size_t size);


}
