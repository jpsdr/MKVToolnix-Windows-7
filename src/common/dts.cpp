/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper function for DTS data

   Written by Peter Niemayer <niemayer@isg.de>.
   Modified by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bit_cursor.h"
#include "common/dts.h"
#include "common/endian.h"
#include "common/math.h"

// ---------------------------------------------------------------------------

namespace mtx { namespace dts {

struct channel_arrangement {
  int num_channels;
  const char * description;
};

static const channel_arrangement channel_arrangements[16] = {
  { 1, "A (mono)"                                                                                                                                    },
  { 2, "A, B (dual mono)"                                                                                                                            },
  { 2, "L, R (left, right)"                                                                                                                          },
  { 2, "L+R, L-R (sum, difference)"                                                                                                                  },
  { 2, "LT, RT (left and right total)"                                                                                                               },
  { 3, "C, L, R (center, left, right)"                                                                                                               },
  { 3, "L, R, S (left, right, surround)"                                                                                                             },
  { 4, "C, L, R, S (center, left, right, surround)"                                                                                                  },
  { 4, "L, R, SL, SR (left, right, surround-left, surround-right)"                                                                                   },
  { 5, "C, L, R, SL, SR (center, left, right, surround-left, surround-right)"                                                                        },
  { 6, "CL, CR, L, R, SL, SR (center-left, center-right, left, right, surround-left, surround-right)"                                                },
  { 6, "C, L, R, LR, RR, OV (center, left, right, left-rear, right-rear, overhead)"                                                                  },
  { 6, "CF, CR, LF, RF, LR, RR  (center-front, center-rear, left-front, right-front, left-rear, right-rear)"                                         },
  { 7, "CL, C, CR, L, R, SL, SR  (center-left, center, center-right, left, right, surround-left, surround-right)"                                    },
  { 8, "CL, CR, L, R, SL1, SL2, SR1, SR2 (center-left, center-right, left, right, surround-left1, surround-left2, surround-right1, surround-right2)" },
  { 8, "CL, C, CR, L, R, SL, S, SR (center-left, center, center-right, left, right, surround-left, surround, surround-right)"                        },
  // other modes are not defined as of yet
};

static const int core_samplefreqs[16] = {
     -1, 8000, 16000, 32000,    -1,    -1, 11025, 22050,
  44100,   -1,    -1, 12000, 24000, 48000,    -1,    -1
};

static const int transmission_bitrates[32] = {
    32000,     56000,     64000,     96000,
   112000,    128000,    192000,    224000,
   256000,    320000,    384000,    448000,
   512000,    576000,    640000,    768000,
   960000,   1024000,   1152000,   1280000,
  1344000,   1408000,   1411200,   1472000,
  1536000,   1920000,   2048000,   3072000,
  3840000,   -1 /*open*/, -2 /*variable*/, -3 /*lossless*/
  // [15]  768000 is actually 754500 for DVD
  // [24] 1536000 is actually 1509750 for DVD
  // [22] 1411200 is actually 1234800 for 14-bit DTS-CD audio
};

#define SPEAKER_PAIR_ALL_2 0xae66

static unsigned int
count_channels_for_mask(unsigned int mask) {
  return mtx::math::count_1_bits(mask) + mtx::math::count_1_bits(mask & SPEAKER_PAIR_ALL_2);
}

int
find_sync_word(unsigned char const *buf,
               size_t size) {
  if (4 > size)
    // not enough data for one header
    return -1;

  unsigned int offset = 0;
  uint32_t sync_word  = get_uint32_be(buf);
  auto sync_word_ok   = false;

  while ((offset + 4) < size) {
    sync_word_ok = (sync_word_e::core == static_cast<sync_word_e>(sync_word)) || (sync_word_e::hd == static_cast<sync_word_e>(sync_word));
    if (sync_word_ok)
      break;

    sync_word = (sync_word << 8) | buf[offset + 4];
    ++offset;
  }

  return sync_word_ok ? offset : -1;
}

static int
find_header_internal(unsigned char const *buf,
                     size_t size,
                     header_t &header,
                     bool allow_no_hd_search) {
  if (15 > size)
    // not enough data for one header
    return -1;

  auto offset = find_sync_word(buf, size);
  if (0 > offset)
    // no header found
    return -1;

  header         = header_t{};
  auto sync_word = static_cast<sync_word_e>(get_uint32_be(&buf[offset]));

  if (sync_word == sync_word_e::core) {
    if (!header.decode_core_header(&buf[offset], size - offset, allow_no_hd_search))
      return -1;

  } else if (sync_word == sync_word_e::hd) {
    if (!header.decode_hd_header(&buf[offset], size - offset))
      return -1;
  }

  // DTS streams without a core (aka DTS Express) is currently not
  // supported as there's no way to determine the number of samples in
  // HD-only packets.
  if (!header.has_core)
    return -1;

  return offset;
}

int
find_header(unsigned char const *buf,
            size_t size,
            header_t &header,
            bool allow_no_hd_search) {
  try {
    return find_header_internal(buf, size, header, allow_no_hd_search);
  } catch (...) {
    mxwarn(Y("Header problem: not enough data to read header\n"));
    return -1;
  }
}

int
find_consecutive_headers(unsigned char const *buf,
                         size_t size,
                         unsigned int num) {
  static auto s_debug = debugging_option_c{"dts_detection"};

  header_t header, new_header;

  auto pos = find_header(buf, size, header, false);

  if (0 > pos)
    return -1;

  if (1 == num)
    return pos;

  unsigned int base = pos;

  do {
    mxdebug_if(s_debug, boost::format("find_cons_dts_h: starting with base at %1%\n") % base);

    int offset = header.frame_byte_size;
    unsigned int i;
    for (i = 0; (num - 1) > i; ++i) {
      if (size < (2 + base + offset))
        break;

      pos = find_header(&buf[base + offset], size - base - offset, new_header, false);
      if (0 == pos) {
        if (new_header == header) {
          mxdebug_if(s_debug, boost::format("find_cons_dts_h: found good header %1%\n") % i);
          offset += new_header.frame_byte_size;
          continue;
        } else
          break;
      } else
        break;
    }

    if (i == (num - 1))
      return base;

    ++base;
    offset = 0;
    pos    = find_header(&buf[base], size - base, header, false);

    if (-1 == pos)
      return -1;

    base += pos;
  } while (base < (size - 5));

  return -1;
}

// ============================================================================

void
header_t::print()
  const {
  mxinfo("DTS Frame Header Information:\n");

  mxinfo("Frame Type             : ");
  if (frametype == frametype_e::normal) {
    mxinfo("normal");
  } else {
    mxinfo(boost::format("termination, deficit sample count = %1%") % deficit_sample_count);
  }
  mxinfo("\n");

  mxinfo(boost::format("CRC available          : %1%\n") % (crc_present ? "yes" : "no"));

  mxinfo(boost::format("Frame Size             : PCM core samples=32*%1%=%2%, %3% milliseconds, %4% byte\n")
         % num_pcm_sample_blocks % (num_pcm_sample_blocks * 32) % ((num_pcm_sample_blocks * 32000.0) / core_sampling_frequency) % frame_byte_size);

  mxinfo(boost::format("Audio Channels         : %1%%2%, arrangement: %3%\n")
         % audio_channels % (source_surround_in_es ? " ES" : "") % audio_channel_arrangement);

  mxinfo(boost::format("Core sampling frequency: %1%\n") % core_sampling_frequency);

  if ((-1 < transmission_bitrate) || (-3 > transmission_bitrate))
    mxinfo(boost::format("Transmission bitrate   : %1%\n") % transmission_bitrate);
  else
    mxinfo(boost::format("Transmission_bitrate   : %1%\n")
           % (  transmission_bitrate == -1 ? "open"
              : transmission_bitrate == -2 ? "variable"
              :                              "lossless"));

  mxinfo(boost::format("Embedded Down Mix      : %1%\n") % (embedded_down_mix      ? "yes" : "no"));
  mxinfo(boost::format("Embedded Dynamic Range : %1%\n") % (embedded_dynamic_range ? "yes" : "no"));
  mxinfo(boost::format("Embedded Time Stamp    : %1%\n") % (embedded_time_stamp    ? "yes" : "no"));
  mxinfo(boost::format("Embedded Auxiliary Data: %1%\n") % (auxiliary_data         ? "yes" : "no"));
  mxinfo(boost::format("HDCD Master            : %1%\n") % (hdcd_master            ? "yes" : "no"));

  mxinfo("Extended Coding        : ");
  if (extended_coding) {
    switch (extension_audio_descriptor) {
      case extension_audio_descriptor_e::xch:
        mxinfo("Extra Channels");
        break;
      case extension_audio_descriptor_e::x96k:
        mxinfo("Extended frequency (x96k)");
        break;
      case extension_audio_descriptor_e::xch_x96k:
        mxinfo("Extra Channels and Extended frequency (x96k)");
        break;
      default:
        mxinfo("yes, but unknown");
        break;
    }
  } else
    mxinfo("no");
  mxinfo("\n");

  mxinfo(boost::format("Audio Sync in sub-subs : %1%\n") % (audio_sync_word_in_sub_sub ? "yes" : "no"));

  mxinfo("Low Frequency Effects  : ");
  switch (lfe_type) {
    case lfe_type_e::none:
      mxinfo("none");
      break;
    case lfe_type_e::lfe_128:
      mxinfo("yes, interpolation factor 128");
      break;
    case lfe_type_e::lfe_64:
      mxinfo("yes, interpolation factor 64");
      break;
    case lfe_type_e::invalid:
      mxinfo("Invalid");
      break;
  }
  mxinfo("\n");

  mxinfo(boost::format("Predictor History used : %1%\n") % (predictor_history_flag ? "yes" : "no"));

  mxinfo(boost::format("Multirate Interpolator : %1%\n") % (multirate_interpolator == multirate_interpolator_e::non_perfect ? "non perfect" : "perfect"));

  mxinfo(boost::format("Encoder Software Vers. : %1%\n") % encoder_software_revision);
  mxinfo(boost::format("Copy History Bits      : %1%\n") % copy_history);
  mxinfo(boost::format("Source PCM Resolution  : %1%\n") % source_pcm_resolution);
  mxinfo(boost::format("Front Encoded as Diff. : %1%\n") % (front_sum_difference    ? "yes" : "no"));
  mxinfo(boost::format("Surr. Encoded as Diff. : %1%\n") % (surround_sum_difference ? "yes" : "no"));
  mxinfo(boost::format("Dialog Normaliz. Gain  : %1%\n") % dialog_normalization_gain);

  if (!has_hd)
    mxinfo("DTS HD                 : no\n");
  else
    mxinfo(boost::format("DTS HD                 : %1%, size %2%\n")
           % (hd_type == hd_type_e::master_audio ? "master audio" : "high resolution") % hd_part_size);
}

unsigned int
header_t::get_total_num_audio_channels()
  const {
  if (has_hd && (0 < num_assets) && substream_assets[0].num_channels_total)
    return substream_assets[0].num_channels_total;

  auto total_num_audio_channels = audio_channels;
  if ((lfe_type_e::lfe_64 == lfe_type) || (lfe_type_e::lfe_128 == lfe_type))
    ++total_num_audio_channels;

  return total_num_audio_channels;
}

bool
header_t::decode_core_header(unsigned char const *buf,
                             size_t size,
                             bool allow_no_hd_search) {
  try {
    auto bc = bit_reader_c{buf, size};
    bc.skip_bits(32);             // sync word

    frametype             = bc.get_bit() ? frametype_e::normal : frametype_e::termination;
    deficit_sample_count  = (bc.get_bits(5) + 1) % 32;
    crc_present           = bc.get_bit();
    num_pcm_sample_blocks = bc.get_bits(7) + 1;
    frame_byte_size       = bc.get_bits(14) + 1;

    if (96 > frame_byte_size) {
      mxwarn(Y("Header problem: invalid frame bytes size\n"));
      return false;
    }

    auto t = bc.get_bits(6);
    if (16 <= t) {
      audio_channels            = -1;
      audio_channel_arrangement = "unknown (user defined)";
    } else {
      audio_channels            = channel_arrangements[t].num_channels;
      audio_channel_arrangement = channel_arrangements[t].description;
    }

    core_sampling_frequency    = core_samplefreqs[bc.get_bits(4)];
    transmission_bitrate       = transmission_bitrates[bc.get_bits(5)];
    embedded_down_mix          = bc.get_bit();
    embedded_dynamic_range     = bc.get_bit();
    embedded_time_stamp        = bc.get_bit();
    auxiliary_data             = bc.get_bit();
    hdcd_master                = bc.get_bit();
    extension_audio_descriptor = static_cast<extension_audio_descriptor_e>(bc.get_bits(3));
    extended_coding            = bc.get_bit();
    audio_sync_word_in_sub_sub = bc.get_bit();
    lfe_type                   = static_cast<lfe_type_e>(bc.get_bits(2));
    predictor_history_flag     = bc.get_bit();

    if (crc_present)
       bc.skip_bits(16);

    multirate_interpolator     = static_cast<multirate_interpolator_e>(bc.get_bit());
    encoder_software_revision  = bc.get_bits(4);
    copy_history               = bc.get_bits(2);

    switch (static_cast<source_pcm_resolution_e>(bc.get_bits(3))) {
      case source_pcm_resolution_e::spr_16:
        source_pcm_resolution = 16;
        source_surround_in_es = false;
        break;

      case source_pcm_resolution_e::spr_16_ES:
        source_pcm_resolution = 16;
        source_surround_in_es = true;
        break;

      case source_pcm_resolution_e::spr_20:
        source_pcm_resolution = 20;
        source_surround_in_es = false;
        break;

      case source_pcm_resolution_e::spr_20_ES:
        source_pcm_resolution = 20;
        source_surround_in_es = true;
        break;

      case source_pcm_resolution_e::spr_24:
        source_pcm_resolution = 24;
        source_surround_in_es = false;
        break;

      case source_pcm_resolution_e::spr_24_ES:
        source_pcm_resolution = 24;
        source_surround_in_es = true;
        break;

      default:
        mxwarn(Y("Header problem: invalid source PCM resolution\n"));
        return false;
    }

    front_sum_difference      = bc.get_bit();
    surround_sum_difference   = bc.get_bit();
    t                         = bc.get_bits(4);
    dialog_normalization_gain = 7 == encoder_software_revision ? -t
                              : 6 == encoder_software_revision ? -16 - t
                              :                                  0;

    has_core                  = true;

    // Detect DTS HD master audio / high resolution part
    has_hd         = false;
    hd_type        = hd_type_e::none;
    hd_part_size   = 0;

    auto hd_offset = frame_byte_size;

    if ((hd_offset + 9) > size)
      return allow_no_hd_search ? true : false;

    if (static_cast<sync_word_e>(get_uint32_be(buf + hd_offset)) == sync_word_e::hd)
      return decode_hd_header(&buf[hd_offset], size - hd_offset);

    return true;

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return false;
}

void
header_t::parse_lbr_parameters(bit_reader_c &bc,
                               substream_asset_t &asset) {
  asset.lbr_size = bc.get_bits(14) + 1; // size of LBR component in extension substream
  if (bc.get_bit())                     // LBR sync word presence flag
    bc.skip_bits(2);                    // LBR sync distance
}

void
header_t::parse_xll_parameters(bit_reader_c &bc,
                               substream_asset_t &asset) {
  asset.xll_size         = bc.get_bits(substream_size_bits) + 1; // size of XLL data in extension substream
  asset.xll_sync_present = bc.get_bit();                         // XLL sync word presence flag

  if (asset.xll_sync_present) {
    bc.skip_bits(4);                                               // peak bit rate smoothing buffer size
    auto xll_delay_num_bits    = bc.get_bits(5) + 1;               // number fof bits for XLL decoding delay
    asset.xll_delay_num_frames = bc.get_bits(xll_delay_num_bits);  // initial XLL decoding delay in frames
    asset.xll_sync_offset      = bc.get_bits(substream_size_bits); // number of bytes offset to XLL sync

  } else {
    asset.xll_delay_num_frames = 0;
    asset.xll_sync_offset      = 0;
  }
}

bool
header_t::decode_asset(bit_reader_c &bc,
                       substream_asset_t &asset) {
  static unsigned int const s_substream_sample_rates[16] = {
      8000,  16000,  32000,  64000,
    128000,  22050,  44100,  88200,
    176400, 352800,  12000,  24000,
     48000,  96000, 192000, 384000
  };

  auto descriptor_pos  = bc.get_bit_position();
  auto descriptor_size = bc.get_bits(9) + 1;
  asset.asset_index    = bc.get_bits(3);

  if (static_fields_present) {
    if (bc.get_bit())                          // asset type descriptor presence
      bc.skip_bits(4);                         // asset type descriptor

    if (bc.get_bit())                          // language descriptor presence
      bc.skip_bits(24);                        // language descriptor

    if (bc.get_bit())                          // additional textual information presence
      bc.skip_bits((bc.get_bits(10) + 1) * 8); // additional textual information

    asset.pcm_bit_res                       = bc.get_bits(5) + 1;
    asset.max_sample_rate                   = s_substream_sample_rates[bc.get_bits(4)];
    asset.num_channels_total                = bc.get_bits(8) + 1;
    asset.one_to_one_map_channel_to_speaker = bc.get_bit();

    if (asset.one_to_one_map_channel_to_speaker) {
      if (asset.num_channels_total > 2)
        asset.embedded_stereo = bc.get_bit();

      if (asset.num_channels_total > 6)
        asset.embedded_6ch = bc.get_bit();

      auto speaker_mask_num_bits = 16;
      if (bc.get_bit()) {                                  // speaker mask enabled flag
        speaker_mask_num_bits = (bc.get_bits(2) + 1) << 2; // number of bits for speaker activity mask
        bc.skip_bits(speaker_mask_num_bits);               // speaker activity mask
      }

      auto num_speaker_remapping_sets = bc.get_bits(3);
      unsigned int num_speakers[8];
      for (auto set_idx = 0u; set_idx < num_speaker_remapping_sets; ++set_idx)
        num_speakers[set_idx] = count_channels_for_mask(bc.get_bits(speaker_mask_num_bits));

      for (auto set_idx = 0u; set_idx < num_speaker_remapping_sets; ++set_idx) {
        auto num_channels_for_remapping = bc.get_bits(5) + 1;
        for (auto spkr_idx = 0u; spkr_idx < num_speakers[set_idx]; ++spkr_idx) {
          auto remap_channel_mask  = bc.get_bits(num_channels_for_remapping);
          auto num_remapping_codes = mtx::math::count_1_bits(remap_channel_mask);
          bc.skip_bits(num_remapping_codes * 5);
        }
      }

    } else {
      asset.embedded_stereo     = false;
      asset.embedded_6ch        = false;
      asset.representation_type = bc.get_bits(3);
    }
  }

  auto drc_present = bc.get_bit();
  if (drc_present)
    bc.skip_bits(8);            // dynamic range coefficients

  if (bc.get_bit())             // dialog normalization presence flag
    bc.skip_bits(5);            // dialog normalization code

  if (drc_present && asset.embedded_stereo)
    bc.skip_bits(8);            // DRC for stereo downmix

  if (mix_metadata_enabled && bc.get_bit()) {
    bc.skip_bit();              // external mixing flag
    bc.skip_bits(6);            // Post mixing / replacement gain adjustment

    if (bc.get_bits(2) == 3)    // DRC prior to mixing
      bc.skip_bits(8);          // custom code for mixing DRC
    else
      bc.skip_bits(3);          // limit for mixing DRC

    // Scaling type for channels of main audio; scaling parameters
    if (bc.get_bit())
      for (auto mix_idx = 0u; mix_idx < num_mixing_configurations; ++mix_idx)
        bc.skip_bits(6 * num_mixing_channels[mix_idx]);
    else
      bc.skip_bits(6 * num_mixing_configurations);

    auto num_channels_downmix = asset.num_channels_total;
    if (asset.embedded_6ch)
      num_channels_downmix += 6;
    if (asset.embedded_stereo)
      num_channels_downmix += 2;

    for (auto mix_idx = 0u; mix_idx < num_mixing_configurations; ++mix_idx)
      for (auto channel_idx = 0u; channel_idx < num_channels_downmix; ++channel_idx) {
        auto mixing_map_mask         = bc.get_bits(num_mixing_channels[mix_idx]);
        auto num_mixing_coefficients = mtx::math::count_1_bits(mixing_map_mask);
        bc.skip_bits(6 * num_mixing_coefficients);
    }
  }

  asset.coding_mode = bc.get_bits(2);
  switch (asset.coding_mode) {
    case 0:                     // multiple coding components
      asset.extension_mask = static_cast<extension_mask_e>(bc.get_bits(12));
      if (asset.extension_mask & exss_core) {
        asset.core_size = bc.get_bits(14) + 1; // size of core component in extension substream
        if (bc.get_bit())                      // core sync word presence flace
          bc.skip_bits(2);                     // core sync distance
      }
      if (asset.extension_mask & exss_xbr)
        asset.xbr_size = bc.get_bits(14) + 1; // size of XBR extension in substream
      if (asset.extension_mask & exss_xxch)
        asset.xxch_size = bc.get_bits(14) + 1; // size of XXCH extension in substream
      if (asset.extension_mask & exss_x96)
        asset.x96_size = bc.get_bits(12) + 1; // size of X96 extension in substream
      if (asset.extension_mask & exss_lbr)
        parse_lbr_parameters(bc, asset);
      if (asset.extension_mask & exss_xll)
        parse_xll_parameters(bc, asset);
      if (asset.extension_mask & exss_rsv1)
        bc.skip_bits(16);
      if (asset.extension_mask & exss_rsv2)
        bc.skip_bits(16);
      break;

    case 1:                     // lossless coding mode without CBR component
      asset.extension_mask = extension_mask_e::exss_xll;
      parse_xll_parameters(bc, asset);
      break;

    case 2:                     // low bit rate mode
      asset.extension_mask = extension_mask_e::exss_lbr;
      parse_lbr_parameters(bc, asset);
      break;

    case 3:                     // auxiliary coding mode
      asset.extension_mask = static_cast<extension_mask_e>(0);
      bc.skip_bits(14);         // size of auxiliary coded data
      bc.skip_bits(8);          // auxiliary codec identification
      if (bc.get_bit())         // auxiliary sync word presence flag
        bc.skip_bits(3);        // auxiliary sync distance
      break;
  }

  if (asset.extension_mask & exss_xll) {
    asset.hd_stream_id = bc.get_bits(3);
    hd_type = hd_type_e::master_audio;

  } else if (asset.extension_mask & (exss_xbr | exss_x96 | exss_xxch))
    hd_type = hd_type_e::high_resolution;

  bc.set_bit_position(descriptor_pos + (descriptor_size * 8));

  return true;
}

#undef test_mask

bool
header_t::decode_hd_header(unsigned char const *buf,
                           size_t size) {
  try {
    auto bc = bit_reader_c{buf, size};
    bc.skip_bits(32);             // sync word
    bc.skip_bits(8);              // user defined
    auto substream_index  = bc.get_bits(2);
    auto header_size_bits =  8;
    substream_size_bits   = 16;

    if (bc.get_bit()) {           // Blown-up header bit
      header_size_bits    = 12;
      substream_size_bits = 20;
    }

    bc.skip_bits(header_size_bits);                          // header size
    hd_part_size     = bc.get_bits(substream_size_bits) + 1; // extension substream size
    frame_byte_size += hd_part_size;
    has_hd           = true;

    num_presentations         = 1u;
    num_assets                = 1u;
    num_mixing_configurations = 0u;
    static_fields_present     = bc.get_bit();

    if (static_fields_present) {
      bc.skip_bits(2);            // reference clock code
      bc.skip_bits(3);            // substream frame duration
      if (bc.get_bit())           // timecode presence flag
        bc.skip_bits(32 + 4);     // timecode data

      num_presentations = bc.get_bits(3) + 1;
      num_assets        = bc.get_bits(3) + 1;

      int active_substream_mask[8];
      for (auto pres_idx = 0u; pres_idx < num_presentations; ++pres_idx)
        active_substream_mask[pres_idx] = bc.get_bits(substream_index + 1);

      for (auto pres_idx = 0u; pres_idx < num_presentations; ++pres_idx)
        for (auto subs_idx = 0u; subs_idx <= substream_index; ++subs_idx)
          if (active_substream_mask[pres_idx] & (1 << subs_idx))
            bc.skip_bits(8);

      mix_metadata_enabled = bc.get_bit();
      if (mix_metadata_enabled) {
        bc.skip_bits(2);          // mixing metadata adjustment level
        auto speaker_mask_num_bits = (bc.get_bits(2) + 1) << 2;
        num_mixing_configurations  = bc.get_bits(2) + 1;

        for (auto mix_idx = 0u; mix_idx < num_mixing_configurations; ++mix_idx)
          num_mixing_channels[mix_idx] = count_channels_for_mask(bc.get_bits(speaker_mask_num_bits));
      }
    }

    substream_assets.resize(num_assets);

    auto offset = header_size_bits;
    for (auto asset_idx = 0u; asset_idx < num_assets; ++asset_idx) {
      auto &asset         = substream_assets[asset_idx];
      asset.asset_offset  = offset;
      asset.asset_size    = bc.get_bits(substream_size_bits) + 1;
      offset             += asset.asset_size;
    }

    for (auto asset_idx = 0u; asset_idx < num_assets; ++asset_idx)
      if (!decode_asset(bc, substream_assets[asset_idx]))
        return false;

    return true;

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return false;
}

void
convert_14_to_16_bits(const unsigned short *src,
                      unsigned long srcwords,
                      unsigned short *dst) {
  // srcwords has to be a multiple of 8!
  // you will get (srcbytes >> 3)*7 destination words!

  const unsigned long l = srcwords >> 3;

  for (unsigned long b = 0; b < l; b++) {
    unsigned short src_0 = (src[0] >>  8) | (src[0] << 8);
    unsigned short src_1 = (src[1] >>  8) | (src[1] << 8);
    // 14 + 2
    unsigned short dst_0 = (src_0  <<  2) | ((src_1 & 0x3fff) >> 12);
    dst[0]               = (dst_0  >>  8) | (dst_0            <<  8);
    // 12 + 4
    unsigned short src_2 = (src[2] >>  8) | (src[2]           <<  8);
    unsigned short dst_1 = (src_1  <<  4) | ((src_2 & 0x3fff) >> 10);
    dst[1]               = (dst_1  >>  8) | (dst_1            <<  8);
    // 10 + 6
    unsigned short src_3 = (src[3] >>  8) | (src[3]           <<  8);
    unsigned short dst_2 = (src_2  <<  6) | ((src_3 & 0x3fff) >>  8);
    dst[2]               = (dst_2  >>  8) | (dst_2            <<  8);
    // 8  + 8
    unsigned short src_4 = (src[4] >>  8) | (src[4]           <<  8);
    unsigned short dst_3 = (src_3  <<  8) | ((src_4 & 0x3fff) >>  6);
    dst[3]               = (dst_3  >>  8) | (dst_3            <<  8);
    // 6  + 10
    unsigned short src_5 = (src[5] >>  8) | (src[5]           <<  8);
    unsigned short dst_4 = (src_4  << 10) | ((src_5 & 0x3fff) >>  4);
    dst[4]               = (dst_4  >>  8) | (dst_4            <<  8);
    // 4  + 12
    unsigned short src_6 = (src[6] >>  8) | (src[6]           <<  8);
    unsigned short dst_5 = (src_5  << 12) | ((src_6 & 0x3fff) >>  2);
    dst[5]               = (dst_5  >>  8) | (dst_5            <<  8);
    // 2  + 14
    unsigned short src_7 = (src[7] >>  8) | (src[7]           <<  8);
    unsigned short dst_6 = (src_6  << 14) | (src_7 & 0x3fff);
    dst[6]               = (dst_6  >>  8) | (dst_6            <<  8);

    dst                 += 7;
    src                 += 8;
  }
}

bool
detect(const void *src_buf,
       int len,
       bool &convert_14_to_16,
       bool &swap_bytes) {
  header_t dtsheader;
  int dts_swap_bytes     = 0;
  int dts_14_16          = 0;
  bool is_dts            = false;
  len                   &= ~0xf;
  int cur_buf            = 0;
  memory_cptr af_buf[2]  = { memory_c::alloc(len),                                        memory_c::alloc(len)                                        };
  unsigned short *buf[2] = { reinterpret_cast<unsigned short *>(af_buf[0]->get_buffer()), reinterpret_cast<unsigned short *>(af_buf[1]->get_buffer()) };

  for (dts_swap_bytes = 0; 2 > dts_swap_bytes; ++dts_swap_bytes) {
    memcpy(buf[cur_buf], src_buf, len);

    if (dts_swap_bytes) {
      swab((char *)buf[cur_buf], (char *)buf[cur_buf^1], len);
      cur_buf ^= 1;
    }

    for (dts_14_16 = 0; 2 > dts_14_16; ++dts_14_16) {
      if (dts_14_16) {
        convert_14_to_16_bits(buf[cur_buf], len / 2, buf[cur_buf^1]);
        cur_buf ^= 1;
      }

      int dst_buf_len = dts_14_16 ? (len * 7 / 8) : len;

      if (find_header(af_buf[cur_buf]->get_buffer(), dst_buf_len, dtsheader) >= 0) {
        is_dts = true;
        break;
      }
    }

    if (is_dts)
      break;
  }

  convert_14_to_16 = dts_14_16      != 0;
  swap_bytes       = dts_swap_bytes != 0;

  return is_dts;
}

bool
operator ==(const header_t &h1,
            const header_t &h2) {
  return (h1.core_sampling_frequency            == h2.core_sampling_frequency)
      && (h1.lfe_type                           == h2.lfe_type)
      && (h1.audio_channels                     == h2.audio_channels)
      && (h1.get_packet_length_in_nanoseconds() == h2.get_packet_length_in_nanoseconds())
    ;
}

bool
operator!=(header_t const &h1,
           header_t const &h2) {
  if (   (h1.crc_present                         != h2.crc_present)
      || (h1.num_pcm_sample_blocks               != h2.num_pcm_sample_blocks)
      || ((h1.frame_byte_size - h1.hd_part_size) != (h2.frame_byte_size - h2.hd_part_size))
      || (h1.audio_channels                      != h2.audio_channels)
      || (h1.core_sampling_frequency             != h2.core_sampling_frequency)
      || (h1.transmission_bitrate                != h2.transmission_bitrate)
      || (h1.embedded_down_mix                   != h2.embedded_down_mix)
      || (h1.embedded_dynamic_range              != h2.embedded_dynamic_range)
      || (h1.embedded_time_stamp                 != h2.embedded_time_stamp)
      || (h1.auxiliary_data                      != h2.auxiliary_data)
      || (h1.hdcd_master                         != h2.hdcd_master)
      || (h1.extension_audio_descriptor          != h2.extension_audio_descriptor)
      || (h1.extended_coding                     != h2.extended_coding)
      || (h1.audio_sync_word_in_sub_sub          != h2.audio_sync_word_in_sub_sub)
      || (h1.lfe_type                            != h2.lfe_type)
      || (h1.predictor_history_flag              != h2.predictor_history_flag)
      || (h1.multirate_interpolator              != h2.multirate_interpolator)
      || (h1.encoder_software_revision           != h2.encoder_software_revision)
      || (h1.copy_history                        != h2.copy_history)
      || (h1.source_pcm_resolution               != h2.source_pcm_resolution)
      || (h1.source_surround_in_es               != h2.source_surround_in_es)
      || (h1.front_sum_difference                != h2.front_sum_difference)
      || (h1.surround_sum_difference             != h2.surround_sum_difference)
      || (h1.dialog_normalization_gain           != h2.dialog_normalization_gain))
    return true;

  return false;
}

}}
