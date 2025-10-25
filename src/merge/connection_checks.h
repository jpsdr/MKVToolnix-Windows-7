/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   connection_check* macros

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#define connect_check_a_samplerate(a, b)                                                                        \
  if ((a) != (b)) {                                                                                             \
    error_message = fmt::format(FY("The sample rate of the two audio tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                           \
  }
#define connect_check_a_channels(a, b)                                                                                 \
  if ((a) != (b)) {                                                                                                    \
    error_message = fmt::format(FY("The number of channels of the two audio tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                                  \
  }
#define connect_check_a_bitdepth(a, b)                                                                                        \
  if ((a) != (b)) {                                                                                                           \
    error_message = fmt::format(FY("The number of bits per sample of the two audio tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                                         \
  }
#define connect_check_v_width(a, b)                                                                 \
  if ((a) != (b)) {                                                                                 \
    error_message = fmt::format(FY("The width of the two tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                               \
  }
#define connect_check_v_height(a, b)                                                                 \
  if ((a) != (b)) {                                                                                  \
    error_message = fmt::format(FY("The height of the two tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                \
  }
#define connect_check_v_dwidth(a, b)                                                                        \
  if ((a) != (b)) {                                                                                         \
    error_message = fmt::format(FY("The display width of the two tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                       \
  }
#define connect_check_v_dheight(a, b)                                                                        \
  if ((a) != (b)) {                                                                                          \
    error_message = fmt::format(FY("The display height of the two tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                        \
  }
#define connect_check_codec_id(a, b)                                                                  \
  if ((a) != (b)) {                                                                                   \
    error_message = fmt::format(FY("The CodecID of the two tracks is different: {0} and {1}"), a, b); \
    return CAN_CONNECT_NO_PARAMETERS;                                                                 \
  }
#define connect_check_codec_private(b)                                                                                                                                      \
  if (                                   (!!this->m_ti.m_private_data             != !!b->m_ti.m_private_data)                                                              \
      || (  this->m_ti.m_private_data && (  this->m_ti.m_private_data->get_size() !=   b->m_ti.m_private_data->get_size()))                                                 \
      || (  this->m_ti.m_private_data &&   memcmp(this->m_ti.m_private_data->get_buffer(), b->m_ti.m_private_data->get_buffer(), this->m_ti.m_private_data->get_size()))) { \
    error_message = fmt::format(FY("The codec's private data does not match (lengths: {0} and {1}).")                                                                       \
                     , this->m_ti.m_private_data ? this->m_ti.m_private_data->get_size() : 0u                                                                               \
                     , b->m_ti.m_private_data    ? b->m_ti.m_private_data->get_size()    : 0u);                                                                             \
    return CAN_CONNECT_MAYBE_CODECPRIVATE;                                                                                                                                  \
  }
