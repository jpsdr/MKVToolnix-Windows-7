/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions and helper functions for TRUEHD data

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_TRUEHD_COMMON_H
#define MTX_COMMON_TRUEHD_COMMON_H

#include "common/common_pch.h"

#include <deque>

#include "common/ac3.h"
#include "common/byte_buffer.h"

#define TRUEHD_SYNC_WORD 0xf8726fba
#define MLP_SYNC_WORD    0xf8726fbb

struct truehd_frame_t {
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

  ac3::frame_c m_ac3_header;

  memory_cptr m_data;

  bool is_truehd() const {
    return truehd == m_codec;
  }

  bool is_sync() const {
    return sync == m_type;
  }

  bool is_normal() const {
    return sync == m_type;
  }

  bool is_ac3() const {
    return ac3 == m_codec;
  }
};
using truehd_frame_cptr = std::shared_ptr<truehd_frame_t>;

class truehd_parser_c {
protected:
  enum {
    state_unsynced,
    state_synced,
  } m_sync_state;

  byte_buffer_c m_buffer;
  std::deque<truehd_frame_cptr> m_frames;

public:
  truehd_parser_c();
  virtual ~truehd_parser_c();

  virtual void add_data(const unsigned char *new_data, unsigned int new_size);
  virtual void parse(bool end_of_stream = false);
  virtual bool frame_available();
  virtual truehd_frame_cptr get_next_frame();

protected:
  virtual unsigned int resync(unsigned int offset);
  virtual int decode_channel_map(int channel_map);
};
using truehd_parser_cptr = std::shared_ptr<truehd_parser_c>;

#endif // MTX_COMMON_TRUEHD_COMMON_H
