/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for TRUEHD data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

#include "common/ac3.h"
#include "common/byte_buffer.h"
#include "common/codec.h"

namespace mtx::truehd {

static constexpr uint32_t TRUEHD_SYNC_WORD = 0xf8726fba;
static constexpr uint32_t MLP_SYNC_WORD    = 0xf8726fbb;

struct frame_t {
  static int const ms_sampling_rates[16];
  static uint8_t const ms_mlp_channels[32];

  enum codec_e {
    truehd,
    mlp,
    ac3,
  };

  enum frame_type_e {
    invalid,
    normal,
    sync,
  };

  codec_e m_codec{truehd};
  frame_type_e m_type{invalid};
  int m_size{}, m_sampling_rate{}, m_channels{}, m_samples_per_frame{};
  unsigned int m_header_size{}, m_input_timing{};
  bool m_contains_atmos{};

  mtx::ac3::frame_c m_ac3_header;

  memory_cptr m_data;

  bool is_truehd() const {
    return truehd == m_codec;
  }

  bool is_mlp() const {
    return mlp == m_codec;
  }

  bool is_sync() const {
    return sync == m_type;
  }

  bool is_normal() const {
    return normal == m_type;
  }

  bool is_ac3() const {
    return ac3 == m_codec;
  }

  bool contains_atmos() const {
    return is_truehd() && m_contains_atmos;
  }

  codec_c codec() const {
    return is_ac3()         ? codec_c::look_up(codec_c::type_e::A_AC3)
         : is_mlp()         ? codec_c::look_up(codec_c::type_e::A_MLP)
         : contains_atmos() ? codec_c::look_up(codec_c::type_e::A_TRUEHD).specialize(codec_c::specialization_e::truehd_atmos)
         :                    codec_c::look_up(codec_c::type_e::A_TRUEHD);
  }

  bool parse_header(uint8_t const *data, std::size_t size);

protected:
  bool parse_ac3_header(uint8_t const *data, std::size_t size);
  bool parse_mlp_header(uint8_t const *data, std::size_t size);
  bool parse_truehd_header(uint8_t const *data, std::size_t size);

public:
  static int decode_channel_map(int channel_map);
  static unsigned int decode_rate_bits(unsigned int rate_bits);
};
using frame_cptr = std::shared_ptr<frame_t>;

class parser_c {
protected:
  enum {
    state_unsynced,
    state_synced,
  } m_sync_state{state_unsynced};

  mtx::bytes::buffer_c m_buffer;
  std::deque<frame_cptr> m_frames;
  frame_t::codec_e m_sync_codec{frame_t::truehd};

public:
  parser_c() = default;
  virtual ~parser_c() = default;

  virtual void add_data(const uint8_t *new_data, unsigned int new_size);
  virtual void parse(bool end_of_stream = false);
  virtual bool frame_available();
  virtual frame_cptr get_next_frame();

protected:
  virtual unsigned int resync(unsigned int offset);
};
using parser_cptr = std::shared_ptr<parser_c>;

void remove_dialog_normalization_gain(uint8_t *buf, std::size_t size);

}
