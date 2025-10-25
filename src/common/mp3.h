/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions for MP3 data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/codec.h"

struct mp3_header_t {
  int version;
  int layer;
  int protection;
  int bitrate_index, bitrate;
  int sampling_frequency;
  int padding, is_private;
  int channel_mode, channels;
  int mode_extension;
  int copyright, original, emphasis;
  size_t framesize;
  int samples_per_channel;
  bool is_tag;

  codec_c get_codec() const;
};

int find_mp3_header(const uint8_t *buf, int size);
int find_consecutive_mp3_headers(const uint8_t *buf, int size, int num, mp3_header_t *header_found = nullptr);

bool decode_mp3_header(const uint8_t *buf, mp3_header_t *h);
