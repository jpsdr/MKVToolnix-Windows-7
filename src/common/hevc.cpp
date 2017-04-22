/** MPEG video helper functions (MPEG 1, 2 and 4)

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

*/

#include "common/common_pch.h"

#include <cmath>
#include <unordered_map>

#include "common/bit_cursor.h"
#include "common/byte_buffer.h"
#include "common/checksums/base.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/mm_io.h"
#include "common/mpeg.h"
#include "common/hevc.h"
#include "common/strings/formatting.h"

namespace mtx { namespace hevc {

std::unordered_map<int, std::string> es_parser_c::ms_nalu_names_by_type;

hevcc_c::hevcc_c()
  : m_configuration_version{}
  , m_general_profile_space{}
  , m_general_tier_flag{}
  , m_general_profile_idc{}
  , m_general_profile_compatibility_flag{}
  , m_general_progressive_source_flag{}
  , m_general_interlace_source_flag{}
  , m_general_nonpacked_constraint_flag{}
  , m_general_frame_only_constraint_flag{}
  , m_general_level_idc{}
  , m_min_spatial_segmentation_idc{}
  , m_parallelism_type{}
  , m_chroma_format_idc{}
  , m_bit_depth_luma_minus8{}
  , m_bit_depth_chroma_minus8{}
  , m_max_sub_layers{}
  , m_temporal_id_nesting_flag{}
  , m_size_nalu_minus_one{}
  , m_nalu_size_length{}
{
}

hevcc_c::hevcc_c(unsigned int nalu_size_length,
                 std::vector<memory_cptr> const &vps_list,
                 std::vector<memory_cptr> const &sps_list,
                 std::vector<memory_cptr> const &pps_list,
                 user_data_t const &user_data,
                 codec_private_t const &codec_private)
  : m_configuration_version{}
  , m_general_profile_space{}
  , m_general_tier_flag{}
  , m_general_profile_idc{}
  , m_general_profile_compatibility_flag{}
  , m_general_progressive_source_flag{}
  , m_general_interlace_source_flag{}
  , m_general_nonpacked_constraint_flag{}
  , m_general_frame_only_constraint_flag{}
  , m_general_level_idc{}
  , m_min_spatial_segmentation_idc{}
  , m_parallelism_type{}
  , m_chroma_format_idc{}
  , m_bit_depth_luma_minus8{}
  , m_bit_depth_chroma_minus8{}
  , m_max_sub_layers{}
  , m_temporal_id_nesting_flag{}
  , m_size_nalu_minus_one{}
  , m_nalu_size_length{nalu_size_length}
  , m_vps_list{vps_list}
  , m_sps_list{sps_list}
  , m_pps_list{pps_list}
  , m_user_data{user_data}
  , m_codec_private{codec_private}
{
}

hevcc_c::operator bool()
  const {
  return m_nalu_size_length
      && !m_vps_list.empty()
      && !m_sps_list.empty()
      && !m_pps_list.empty()
      && (m_vps_info_list.empty() || (m_vps_info_list.size() == m_vps_list.size()))
      && (m_sps_info_list.empty() || (m_sps_info_list.size() == m_sps_list.size()))
      && (m_pps_info_list.empty() || (m_pps_info_list.size() == m_pps_list.size()));
}

bool
hevcc_c::parse_vps_list(bool ignore_errors) {
  if (m_vps_info_list.size() == m_vps_list.size())
    return true;

  m_vps_info_list.clear();
  for (auto const &vps: m_vps_list) {
    vps_info_t vps_info;
    auto vps_as_rbsp = mpeg::nalu_to_rbsp(vps);

    if (ignore_errors) {
      try {
        parse_vps(vps_as_rbsp, vps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_vps(vps_as_rbsp, vps_info))
      return false;

    m_vps_info_list.push_back(vps_info);
  }

  return true;
}

bool
hevcc_c::parse_sps_list(bool ignore_errors) {
  if (m_sps_info_list.size() == m_sps_list.size())
    return true;

  m_sps_info_list.clear();
  for (auto const &sps: m_sps_list) {
    sps_info_t sps_info;
    auto sps_as_rbsp = mpeg::nalu_to_rbsp(sps);

    if (ignore_errors) {
      try {
        parse_sps(sps_as_rbsp, sps_info, m_vps_info_list);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_sps(sps_as_rbsp, sps_info, m_vps_info_list))
      return false;

    m_sps_info_list.push_back(sps_info);
  }

  return true;
}

bool
hevcc_c::parse_pps_list(bool ignore_errors) {
  if (m_pps_info_list.size() == m_pps_list.size())
    return true;

  m_pps_info_list.clear();
  for (auto const &pps: m_pps_list) {
    pps_info_t pps_info;
    auto pps_as_rbsp = mpeg::nalu_to_rbsp(pps);

    if (ignore_errors) {
      try {
        parse_pps(pps_as_rbsp, pps_info);
      } catch (mtx::mm_io::end_of_file_x &) {
      }
    } else if (!parse_pps(pps_as_rbsp, pps_info))
      return false;

    m_pps_info_list.push_back(pps_info);
  }

  return true;
}

bool
hevcc_c::parse_sei_list() {
  m_sei_list.clear();

  int size = 100;

  user_data_t::const_iterator iter;
  for (iter = m_user_data.begin(); iter != m_user_data.end(); ++iter) {
    int payload_size = iter->second.size();
    size += payload_size;
    size += payload_size / 255 + 2;
  }

  if (size == 100)
    return true;

  auto mcptr_newsei = memory_c::alloc(size);
  auto newsei       = mcptr_newsei->get_buffer();
  bit_writer_c w(newsei, size);
  mm_mem_io_c byte_writer{newsei, static_cast<uint64_t>(size), 100};

  w.put_bits(1, 0); // forbidden_zero_bit
  w.put_bits(6, HEVC_NALU_TYPE_PREFIX_SEI); // nal_unit_type
  w.put_bits(6, 0); // nuh_reserved_zero_6bits
  w.put_bits(3, 1); // nuh_temporal_id_plus1

  byte_writer.skip(2); // skip the nalu header

  for (iter = m_user_data.begin(); iter != m_user_data.end(); ++iter) {
    int payload_size = iter->second.size();
    byte_writer.write_uint8(HEVC_SEI_USER_DATA_UNREGISTERED);
    while (payload_size >= 255) {
      byte_writer.write_uint8(255);
      payload_size -= 255;
    }
    byte_writer.write_uint8(payload_size);
    byte_writer.write(&iter->second[0], iter->second.size());
  }

  w.set_bit_position(byte_writer.getFilePointer() * 8);
  w.put_bit(1);
  w.byte_align();

  mcptr_newsei->set_size(w.get_bit_position() / 8);

  m_sei_list.push_back(mpeg::rbsp_to_nalu(mcptr_newsei));

  return true;
}

/* Codec Private Data

The format of the MKV CodecPrivate element for HEVC has been aligned with MP4 and GPAC/MP4Box.
The definition of MP4 for HEVC has not been finalized. The version of MP4Box appears to be
aligned with the latest version of the HEVC standard. The configuration_version field should be
kept 0 until CodecPrivate for HEVC have been finalized. Thereafter it shall have the required value of 1.
The CodecPrivate format is flexible and allows storage of arbitrary NAL units.
However it is restricted by MP4 to VPS, SPS and PPS headers and SEI messages that apply to the
whole stream as for example user data. The table below specifies the format:

Value                               Bits   Pos  Description
-----                               ----  ----  -----------
configuration_version                  8   0.0  The value should be 0 until the format has been finalized. Thereafter is should have the specified value (probably 1). This allows us to recognize (and ignore) non-standard CodecPrivate
general_profile_space                  2   1.0  Specifies the context for the interpretation of general_profile_idc and  general_profile_compatibility_flag
general_tier_flag                      1   1.2  Specifies the context for the interpretation of general_level_idc
general_profile_idc                    5   1.3  Defines the profile of the bitstream
general_profile_compatibility_flag    32   2.0  Defines profile compatibility, see [2] for interpretation
general_progressive_source_flag        1   6.0  Source is progressive, see [2] for interpretation.
general_interlace_source_flag          1   6.1  Source is interlaced, see [2] for interpretation.
general_nonpacked_constraint_flag      1   6.2  If 1 then no frame packing arrangement SEI messages, see [2] for more information
general_frame_only_constraint_flag     1   6.3  If 1 then no fields, see [2] for interpretation
reserved                              44   6.4  Reserved field, value TBD 0
general_level_idc                      8  12.0  Defines the level of the bitstream
reserved                               4  13.0  Reserved Field, value '1111'b
min_spatial_segmentation_idc          12  13.4  Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
reserved                               6  15.0  Reserved Field, value '111111'b
parallelism_type                       2  15.6  0=unknown, 1=slices, 2=tiles, 3=WPP
reserved                               6  16.0  Reserved field, value '111111'b
chroma_format_idc                      2  16.6  See table 6-1, HEVC
reserved                               5  17.0  Reserved Field, value '11111'b
bit_depth_luma_minus8                  3  17.5  Bit depth luma minus 8
reserved                               5  18.0  Reserved Field, value '11111'b
bit_depth_chroma_minus8                3  18.5  Bit depth chroma minus 8
reserved                              16  19.0  Reserved Field, value 0
reserved                               2  21.0  Reserved Field, value 0
max_sub_layers                         3  21.2  maximum number of temporal sub-layers
temporal_id_nesting_flag               1  21.5  Specifies whether inter prediction is additionally restricted. see [2] for interpretation.
size_nalu_minus_one                    2  21.6  Size of field NALU Length – 1
num_parameter_sets                     8  22.0  Number of parameter sets

for (i=0;i<num_parameter_sets;i++) {
  array_completeness                   1   0.0  1 when there is no duplicate parameter set with same id in the stream, 0 otherwise or unknown
  reserved                             1   0.1  Value '1'b
  nal_unit_type                        6   0.2  Nal unit type, restricted to VPS, SPS, PPS and SEI, SEI must be of declarative nature which applies to the whole stream such as user data sei.
  nal_unit_count                      16   1.0  Number of nal units
  for (j=0;j<nalu_unit_count;j+) {
    size                              16   3.0  Size of nal unit
    for(k=0;k<size;k++) {
      data[k]                          8   5.0  Nalu data
    }
  }
}

*/
memory_cptr
hevcc_c::pack() {
  parse_vps_list(true);
  parse_sps_list(true);
  parse_pps_list(true);
  parse_sei_list();

  if (!*this)
    return {};

  mm_mem_io_c buffer{nullptr, 1024, 1024};

  auto write_list = [&buffer](std::vector<memory_cptr> const &list, uint8 nal_unit_type) {
    buffer.write_uint8((0 << 7) | (0 << 6) | (nal_unit_type & 0x3F));
    buffer.write_uint16_be(list.size());

    for (auto &mem : list) {
      buffer.write_uint16_be(mem->get_size());
      buffer.write(mem);
    }
  };

  // configuration version
  buffer.write_uint8(1);
  // general parameters block
  // general_profile_space               2     Specifies the context for the interpretation of general_profile_idc and
  //                                           general_profile_compatibility_flag
  // general_tier_flag                   1     Specifies the context for the interpretation of general_level_idc
  // general_profile_idc                 5     Defines the profile of the bitstream
  buffer.write_uint8((   (m_codec_private.profile_space & 0x03) << 6)
                      | ((m_codec_private.tier_flag     & 0x01) << 5)
                      |  (m_codec_private.profile_idc   & 0x1F));
  // general_profile_compatibility_flag  32    Defines profile compatibility
  buffer.write_uint32_be(m_codec_private.profile_compatibility_flag);
  // general_progressive_source_flag     1     Source is progressive, see [2] for interpretation.
  // general_interlace_source_flag       1     Source is interlaced, see [2] for interpretation.
  // general_non-packed_constraint_flag  1     If 1 then no frame packing arrangement SEI messages, see [2] for more information
  // general_frame_only_constraint_flag  1     If 1 then no fields, see [2] for interpretation
  // reserved                            44    Reserved field, value TBD 0
  buffer.write_uint8(  ((m_codec_private.progressive_source_flag    & 0x01) << 7)
                     | ((m_codec_private.interlaced_source_flag     & 0x01) << 6)
                     | ((m_codec_private.non_packed_constraint_flag & 0x01) << 5)
                     | ((m_codec_private.frame_only_constraint_flag & 0x01) << 4));
  buffer.write_uint8(0);
  buffer.write_uint8(0);
  buffer.write_uint8(0);
  buffer.write_uint8(0);
  buffer.write_uint8(0);
  // general_level_idc                   8     Defines the level of the bitstream
  buffer.write_uint8(m_codec_private.level_idc & 0xFF);
  // reserved                            4     Reserved Field, value '1111'b
  // min_spatial_segmentation_idc        12    Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
  buffer.write_uint8(0xF0 | ((m_codec_private.min_spatial_segmentation_idc >> 8) & 0x0F));
  buffer.write_uint8(m_codec_private.min_spatial_segmentation_idc & 0XFF);
  // reserved                            6     Reserved Field, value '111111'b
  // parallelism_type                    2     0=unknown, 1=slices, 2=tiles, 3=WPP
  buffer.write_uint8(0xFC | m_codec_private.parallelism_type); //0x00 - unknown
  // reserved                            6     Reserved field, value '111111'b
  // chroma_format_idc                   2     See table 6-1, HEVC
  buffer.write_uint8(0xFC | (m_codec_private.chroma_format_idc & 0x03));
  // reserved                            5     Reserved Field, value '11111'b
  // bit_depth_luma_minus8               3     Bit depth luma minus 8
  buffer.write_uint8(0xF8 | ((m_codec_private.bit_depth_luma_minus8) & 0x07));
  // reserved                            5     Reserved Field, value '11111'b
  // bit_depth_chroma_minus8             3     Bit depth chroma minus 8
  buffer.write_uint8(0xF8 | ((m_codec_private.bit_depth_chroma_minus8) & 0x07));
  // reserved                            16    Reserved Field, value 0
  buffer.write_uint8(0);
  buffer.write_uint8(0);
  // reserved                            2     Reserved Field, value 0
  // max_sub_layers                      3     maximum number of temporal sub-layers
  // temporal_id_nesting_flag            1     Specifies whether inter prediction is additionally restricted. see [2] for interpretation.
  // size_nalu_minus_one                 2     Size of field NALU Length – 1
  buffer.write_uint8(  (((m_codec_private.max_sub_layers_minus1 + 1) & 0x03) << 6)
                     | ((m_codec_private.temporal_id_nesting_flag    & 0x01) << 2)
                     | ((m_nalu_size_length - 1)                     & 0x03));
  // num_arrays                          8     Number of arrays of parameter sets
  auto num_arrays = (m_vps_list.empty() ? 0 : 1)
                  + (m_sps_list.empty() ? 0 : 1)
                  + (m_pps_list.empty() ? 0 : 1)
                  + (m_sei_list.empty() ? 0 : 1);
  buffer.write_uint8(num_arrays);

  if (m_vps_list.size())
    write_list(m_vps_list, HEVC_NALU_TYPE_VIDEO_PARAM);
  if (m_sps_list.size())
    write_list(m_sps_list, HEVC_NALU_TYPE_SEQ_PARAM);
  if (m_pps_list.size())
    write_list(m_pps_list, HEVC_NALU_TYPE_PIC_PARAM);
  if (m_sei_list.size())
    write_list(m_sei_list, HEVC_NALU_TYPE_PREFIX_SEI);

  return std::make_shared<memory_c>(buffer.get_and_lock_buffer(), buffer.getFilePointer());
}

hevcc_c
hevcc_c::unpack(memory_cptr const &mem) {
  hevcc_c hevcc;

  if (!mem)
    return hevcc;

  try {
    bit_reader_c bit_reader(mem->get_buffer(), mem->get_size());
    mm_mem_io_c byte_reader{*mem};

    // configuration_version               8     The value should be 0 until the format has been finalized. Thereafter is should have the specified value
    //                                           (probably 1). This allows us to recognize (and ignore) non-standard CodecPrivate
    hevcc.m_configuration_version = bit_reader.get_bits(8);
    // general_profile_space               2     Specifies the context for the interpretation of general_profile_idc and
    //                                           general_profile_compatibility_flag
    hevcc.m_general_profile_space = bit_reader.get_bits(2);
    // general_tier_flag                   1     Specifies the context for the interpretation of general_level_idc
    hevcc.m_general_tier_flag = bit_reader.get_bits(1);
    // general_profile_idc                 5     Defines the profile of the bitstream
    hevcc.m_general_profile_idc = bit_reader.get_bits(5);
    // general_profile_compatibility_flag  32    Defines profile compatibility, see [2] for interpretation
    hevcc.m_general_profile_compatibility_flag = bit_reader.get_bits(32);
    // general_progressive_source_flag     1     Source is progressive, see [2] for interpretation.
    hevcc.m_general_progressive_source_flag = bit_reader.get_bits(1);
    // general_interlace_source_flag       1     Source is interlaced, see [2] for interpretation.
    hevcc.m_general_interlace_source_flag = bit_reader.get_bits(1);
    // general_nonpacked_constraint_flag  1     If 1 then no frame packing arrangement SEI messages, see [2] for more information
    hevcc.m_general_nonpacked_constraint_flag = bit_reader.get_bits(1);
    // general_frame_only_constraint_flag  1     If 1 then no fields, see [2] for interpretation
    hevcc.m_general_frame_only_constraint_flag = bit_reader.get_bits(1);
    // reserved                            44    Reserved field, value TBD 0
    bit_reader.skip_bits(44);
    // general_level_idc                   8     Defines the level of the bitstream
    hevcc.m_general_level_idc = bit_reader.get_bits(8);
    // reserved                            4     Reserved Field, value '1111'b
    bit_reader.skip_bits(4);
    // min_spatial_segmentation_idc        12    Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
    hevcc.m_min_spatial_segmentation_idc = bit_reader.get_bits(12);
    // reserved                            6     Reserved Field, value '111111'b
    bit_reader.skip_bits(6);
    // parallelism_type                    2     0=unknown, 1=slices, 2=tiles, 3=WPP
    hevcc.m_parallelism_type = bit_reader.get_bits(2);
    // reserved                            6     Reserved field, value '111111'b
    bit_reader.skip_bits(6);
    // chroma_format_idc                   2     See table 6-1, HEVC
    hevcc.m_chroma_format_idc = bit_reader.get_bits(2);
    // reserved                            5     Reserved Field, value '11111'b
    bit_reader.skip_bits(5);
    // bit_depth_luma_minus8               3     Bit depth luma minus 8
    hevcc.m_bit_depth_luma_minus8 = bit_reader.get_bits(3);
    // reserved                            5     Reserved Field, value '11111'b
    bit_reader.skip_bits(5);
    // bit_depth_chroma_minus8             3     Bit depth chroma minus 8
    hevcc.m_bit_depth_chroma_minus8 = bit_reader.get_bits(3);
    // reserved                            16    Reserved Field, value 0
    bit_reader.skip_bits(16);
    // reserved                            2     Reserved Field, value 0
    bit_reader.skip_bits(2);
    // max_sub_layers                      3     maximum number of temporal sub-layers
    hevcc.m_max_sub_layers = bit_reader.get_bits(3);
    // temporal_id_nesting_flag            1     Specifies whether inter prediction is additionally restricted. see [2] for interpretation.
    hevcc.m_temporal_id_nesting_flag = bit_reader.get_bits(1);
    // size_nalu_minus_one                 2     Size of field NALU Length – 1
    hevcc.m_size_nalu_minus_one = bit_reader.get_bits(2);

    unsigned int num_arrays = bit_reader.get_bits(8);

    // now skip over initial data and read in parameter sets, use byte reader
    byte_reader.skip(23);

    while (num_arrays > 1) {
      auto type           = byte_reader.read_uint8() & 0x3F;
      auto nal_unit_count = byte_reader.read_uint16_be();
      auto &list          = type == HEVC_NALU_TYPE_VIDEO_PARAM ? hevcc.m_vps_list
                          : type == HEVC_NALU_TYPE_SEQ_PARAM   ? hevcc.m_sps_list
                          : type == HEVC_NALU_TYPE_PIC_PARAM   ? hevcc.m_pps_list
                          :                                      hevcc.m_sei_list;

      while (nal_unit_count) {
        auto size = byte_reader.read_uint16_be();
        list.push_back(byte_reader.read(size));
        --nal_unit_count;
      }

      --num_arrays;
    }

    return hevcc;

  } catch (mtx::mm_io::exception &) {
    return hevcc_c{};
  }
}

static const struct {
  int numerator, denominator;
} s_predefined_pars[HEVC_NUM_PREDEFINED_PARS] = {
  {   0,  0 },
  {   1,  1 },
  {  12, 11 },
  {  10, 11 },
  {  16, 11 },
  {  40, 33 },
  {  24, 11 },
  {  20, 11 },
  {  32, 11 },
  {  80, 33 },
  {  18, 11 },
  {  15, 11 },
  {  64, 33 },
  { 160, 99 },
  {   4,  3 },
  {   3,  2 },
  {   2,  1 },
};

bool
par_extraction_t::is_valid()
  const {
  return successful && numerator && denominator;
}

static void
profile_tier_copy(bit_reader_c &r,
                  bit_writer_c &w,
                  vps_info_t &vps,
                  unsigned int maxNumSubLayersMinus1) {
  unsigned int i;
  std::vector<bool> sub_layer_profile_present_flag, sub_layer_level_present_flag;

  vps.profile_space = w.copy_bits(2, r);
  vps.tier_flag = w.copy_bits(1, r);
  vps.profile_idc = w.copy_bits(5, r);  // general_profile_idc
  vps.profile_compatibility_flag = w.copy_bits(32, r);
  vps.progressive_source_flag = w.copy_bits(1, r);
  vps.interlaced_source_flag = w.copy_bits(1, r);
  vps.non_packed_constraint_flag = w.copy_bits(1, r);
  vps.frame_only_constraint_flag = w.copy_bits(1, r);
  w.copy_bits(44, r);     // general_reserved_zero_44bits
  vps.level_idc = w.copy_bits(8, r);    // general_level_idc

  for (i = 0; i < maxNumSubLayersMinus1; i++) {
    sub_layer_profile_present_flag.push_back(w.copy_bits(1, r)); // sub_layer_profile_present_flag[i]
    sub_layer_level_present_flag.push_back(w.copy_bits(1, r));   // sub_layer_level_present_flag[i]
  }

  if (maxNumSubLayersMinus1 > 0)
    for (i = maxNumSubLayersMinus1; i < 8; i++)
      w.copy_bits(2, r);  // reserved_zero_2bits

  for (i = 0; i < maxNumSubLayersMinus1; i++) {
    if (sub_layer_profile_present_flag[i]) {
      w.copy_bits(2+1+5, r);  // sub_layer_profile_space[i], sub_layer_tier_flag[i], sub_layer_profile_idc[i]
      w.copy_bits(32, r);     // sub_layer_profile_compatibility_flag[i][]
      w.copy_bits(4, r);      // sub_layer_progressive_source_flag[i], sub_layer_interlaced_source_flag[i], sub_layer_non_packed_constraint_flag[i], sub_layer_frame_only_constraint_flag[i]
      w.copy_bits(44, r);     // sub_layer_reserved_zero_44bits[i]
    }
    if (sub_layer_level_present_flag[i]) {
      w.copy_bits(8, r);      // sub_layer_level_idc[i]
    }
  }
}

static void
sub_layer_hrd_parameters_copy(bit_reader_c &r,
                              bit_writer_c &w,
                              unsigned int CpbCnt,
                              bool sub_pic_cpb_params_present_flag) {
  unsigned int i;

  for (i = 0; i <= CpbCnt; i++) {
    w.copy_unsigned_golomb(r); // bit_rate_value_minus1[i]
    w.copy_unsigned_golomb(r); // cpb_size_value_minus1[i]

    if (sub_pic_cpb_params_present_flag) {
      w.copy_unsigned_golomb(r); // cpb_size_du_value_minus1[i]
      w.copy_unsigned_golomb(r); // bit_rate_du_value_minus1[i]
    }

    w.copy_bits(1, r); // cbr_flag[i]
  }
}

static void
hrd_parameters_copy(bit_reader_c &r,
                    bit_writer_c &w,
                    bool commonInfPresentFlag,
                    unsigned int maxNumSubLayersMinus1) {
  unsigned int i;

  bool nal_hrd_parameters_present_flag = false;
  bool vcl_hrd_parameters_present_flag = false;
  bool sub_pic_cpb_params_present_flag = false;

  if (commonInfPresentFlag) {
    nal_hrd_parameters_present_flag = w.copy_bits(1, r); // nal_hrd_parameters_present_flag
    vcl_hrd_parameters_present_flag = w.copy_bits(1, r); // vcl_hrd_parameters_present_flag

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
      sub_pic_cpb_params_present_flag = w.copy_bits(1, r); // sub_pic_cpb_params_present_flag
      if (sub_pic_cpb_params_present_flag) {
        w.copy_bits(8, r);  // tick_divisor_minus2
        w.copy_bits(5, r);  // du_cpb_removal_delay_increment_length_minus1
        w.copy_bits(1, r);  // sub_pic_cpb_params_in_pic_timing_sei_flag
        w.copy_bits(5, r);  // dpb_output_delay_du_length_minus1
      }

      w.copy_bits(4+4, r);  // bit_rate_scale, cpb_size_scale

      if (sub_pic_cpb_params_present_flag)
        w.copy_bits(4, r);  // cpb_size_du_scale

      w.copy_bits(5, r);  // initial_cpb_removal_delay_length_minus1
      w.copy_bits(5, r);  // au_cpb_removal_delay_length_minus1
      w.copy_bits(5, r);  // dpb_output_delay_length_minus1
    }
  }

  for (i = 0; i <= maxNumSubLayersMinus1; i++) {
    bool fixed_pic_rate_general_flag = w.copy_bits(1, r); // fixed_pic_rate_general_flag[i]
    bool fixed_pic_rate_within_cvs_flag = false;
    bool low_delay_hrd_flag = false;
    unsigned int CpbCnt = 0;

    if (!fixed_pic_rate_general_flag)
      fixed_pic_rate_within_cvs_flag = w.copy_bits(1, r); // fixed_pic_rate_within_cvs_flag[i]
    else {
      // E.2.2 - When fixed_pic_rate_general_flag[i] is equal to 1,
      // the value of fixed_pic_rate_within_cvs_flag[i] is inferred to be equal to 1.
      fixed_pic_rate_within_cvs_flag = true;
    }
    if (fixed_pic_rate_within_cvs_flag)
      w.copy_unsigned_golomb(r);                                       // elemental_duration_in_tc_minus1[i]
    else
      low_delay_hrd_flag = w.copy_bits(1, r);             // low_delay_hrd_flag[i]

    if (!low_delay_hrd_flag)
      CpbCnt = w.copy_unsigned_golomb(r);                              // cpb_cnt_minus1[i]

    if (nal_hrd_parameters_present_flag)
      sub_layer_hrd_parameters_copy(r, w, CpbCnt, sub_pic_cpb_params_present_flag);

    if (vcl_hrd_parameters_present_flag)
      sub_layer_hrd_parameters_copy(r, w, CpbCnt, sub_pic_cpb_params_present_flag);
  }
}

static void
scaling_list_data_copy(bit_reader_c &r,
                       bit_writer_c &w) {
  unsigned int i;
  unsigned int sizeId;
  for (sizeId = 0; sizeId < 4; sizeId++) {
    unsigned int matrixId;
    for (matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6); matrixId++) {
      if (w.copy_bits(1, r) == 0) {  // scaling_list_pred_mode_flag[sizeId][matrixId]
        w.copy_unsigned_golomb(r); // scaling_list_pred_matrix_id_delta[sizeId][matrixId]
      } else {
        unsigned int coefNum = std::min(64, (1 << (4 + (sizeId << 1))));

        if (sizeId > 1) {
          w.copy_signed_golomb(r);  // scaling_list_dc_coef_minus8[sizeId - 2][matrixId]
        }

        for (i = 0; i < coefNum; i++) {
          w.copy_signed_golomb(r);  // scaling_list_delta_coef
        }
      }
    }
  }
}

static void
short_term_ref_pic_set_copy(bit_reader_c &r,
                            bit_writer_c &w,
                            short_term_ref_pic_set_t *short_term_ref_pic_sets,
                            unsigned int idx_rps,
                            unsigned int num_short_term_ref_pic_sets) {
  if (idx_rps >= 64)
    throw false;

  short_term_ref_pic_set_t* ref_st_rp_set;
  short_term_ref_pic_set_t* cur_st_rp_set = short_term_ref_pic_sets + idx_rps;
  unsigned int inter_rps_pred_flag = cur_st_rp_set->inter_ref_pic_set_prediction_flag = 0;

  if (idx_rps > 0)
    inter_rps_pred_flag = cur_st_rp_set->inter_ref_pic_set_prediction_flag = w.copy_bits(1, r); // inter_ref_pic_set_prediction_flag

  if (inter_rps_pred_flag) {
    int ref_idx;
    int delta_rps;
    int i;
    int k = 0;
    int k0 = 0;
    int k1 = 0;
    int code = 0;

    if (idx_rps == num_short_term_ref_pic_sets)
      code = w.copy_unsigned_golomb(r); // delta_idx_minus1

    cur_st_rp_set->delta_idx = code + 1;
    ref_idx                  = idx_rps - 1 - code;

    if (ref_idx >= 64)
      throw false;

    ref_st_rp_set = short_term_ref_pic_sets + ref_idx;

    cur_st_rp_set->delta_rps_sign = w.copy_bits(1, r);  // delta_rps_sign
    cur_st_rp_set->delta_idx = w.copy_unsigned_golomb(r) + 1;  // abs_delta_rps_minus1

    delta_rps = (1 - cur_st_rp_set->delta_rps_sign*2) * cur_st_rp_set->delta_idx;

    // num_pics is used for indexing delta_poc[], user[] and ref_id[],
    // each of which only has 17 entries.
    if ((0 > cur_st_rp_set->num_pics) || (16 < cur_st_rp_set->num_pics))
      throw false;

    for (i = 0; i <= ref_st_rp_set->num_pics; i++) {
      int ref_id = w.copy_bits(1, r); // used_by_curr_pic_flag
      if (ref_id == 0) {
        int bit = w.copy_bits(1, r);  // use_delta_flag
        ref_id = bit*2;
      }

      if (ref_id != 0) {
        int delta_POC = delta_rps + ((i < ref_st_rp_set->num_pics) ? ref_st_rp_set->delta_poc[i] : 0);
        cur_st_rp_set->delta_poc[k] = delta_POC;
        cur_st_rp_set->used[k] = (ref_id == 1) ? 1 : 0;

        k0 += delta_POC < 0;
        k1 += delta_POC >= 0;
        k++;
      }

      cur_st_rp_set->ref_id[i] = ref_id;
    }

    cur_st_rp_set->num_ref_id = ref_st_rp_set->num_pics + 1;
    cur_st_rp_set->num_pics = k;
    cur_st_rp_set->num_negative_pics = k0;
    cur_st_rp_set->num_positive_pics = k1;

    // Sort in increasing order (smallest first)
    for (i = 1; i < cur_st_rp_set->num_pics; i++ ) {
      int delta_POC = cur_st_rp_set->delta_poc[i];
      int used = cur_st_rp_set->used[i];
      for (k = i - 1; k >= 0; k--) {
        int temp = cur_st_rp_set->delta_poc[k];

        if (delta_POC < temp) {
          cur_st_rp_set->delta_poc[k + 1] = temp;
          cur_st_rp_set->used[k + 1] = cur_st_rp_set->used[k];
          cur_st_rp_set->delta_poc[k] = delta_POC;
          cur_st_rp_set->used[k] = used;
        }
      }
    }

    // Flip the negative values to largest first
    for (i = 0, k = cur_st_rp_set->num_negative_pics - 1; i < cur_st_rp_set->num_negative_pics >> 1; i++, k--) {
      int delta_POC = cur_st_rp_set->delta_poc[i];
      int used = cur_st_rp_set->used[i];
      cur_st_rp_set->delta_poc[i] = cur_st_rp_set->delta_poc[k];
      cur_st_rp_set->used[i] = cur_st_rp_set->used[k];
      cur_st_rp_set->delta_poc[k] = delta_POC;
      cur_st_rp_set->used[k] = used;
    }
  } else {
    int prev = 0;
    int poc;
    int i;

    cur_st_rp_set->num_negative_pics = w.copy_unsigned_golomb(r);  // num_negative_pics
    cur_st_rp_set->num_positive_pics = w.copy_unsigned_golomb(r);  // num_positive_pics

    // num_positive_pics and num_negative_pics are used for indexing
    // delta_poc[], user[] and ref_id[]. The second loop is counting
    // from num_negative_pics to num_pics; therefore the former must
    // not be bigger than the latter.
    if (    (cur_st_rp_set->num_positive_pics < 0) || (cur_st_rp_set->num_positive_pics > 16)
         || (cur_st_rp_set->num_negative_pics < 0) || (cur_st_rp_set->num_negative_pics > 16)
         || ((cur_st_rp_set->num_positive_pics + cur_st_rp_set->num_negative_pics) > 16))
      throw false;

    for (i = 0; i < cur_st_rp_set->num_negative_pics; i++) {
      int code = w.copy_unsigned_golomb(r); // delta_poc_s0_minus1
      poc = prev - code - 1;
      prev = poc;
      cur_st_rp_set->delta_poc[i] = poc;
      cur_st_rp_set->used[i] = w.copy_bits(1, r); // used_by_curr_pic_s0_flag
    }

    prev = 0;
    cur_st_rp_set->num_pics = cur_st_rp_set->num_negative_pics + cur_st_rp_set->num_positive_pics;
    for (i = cur_st_rp_set->num_negative_pics; i < cur_st_rp_set->num_pics; i++) {
      int code = w.copy_unsigned_golomb(r); // delta_poc_s1_minus1
      poc = prev + code + 1;
      prev = poc;
      cur_st_rp_set->delta_poc[i] = poc;
      cur_st_rp_set->used[i] = w.copy_bits(1, r); // used_by_curr_pic_s1_flag
    }
  }
}

static void
vui_parameters_copy(bit_reader_c &r,
                    bit_writer_c &w,
                    sps_info_t &sps,
                    bool keep_ar_info,
                    unsigned int max_sub_layers_minus1) {
  if (r.get_bit() == 1) {                   // aspect_ratio_info_present_flag
    unsigned int ar_type = r.get_bits(8);   // aspect_ratio_idc

    if (keep_ar_info) {
      w.put_bit(1);                         // aspect_ratio_info_present_flag
      w.put_bits(8, ar_type);               // aspect_ratio_idc
    } else
      w.put_bit(0);                         // aspect_ratio_info_present_flag

    sps.ar_found = true;

    if (HEVC_EXTENDED_SAR == ar_type) {
      sps.par_num = r.get_bits(16); // sar_width
      sps.par_den = r.get_bits(16); // sar_height

      if (keep_ar_info &&
          0xFF == ar_type) {
        w.put_bits(16, sps.par_num);
        w.put_bits(16, sps.par_den);
      }
    } else if (HEVC_NUM_PREDEFINED_PARS >= ar_type) {
      sps.par_num = s_predefined_pars[ar_type].numerator;
      sps.par_den = s_predefined_pars[ar_type].denominator;
    }

  } else {
    sps.ar_found = false;
    w.put_bit(0);
  }

  // copy the rest
  if (w.copy_bits(1, r) == 1)   // overscan_info_present_flag
    w.copy_bits(1, r);          // overscan_appropriate_flag
  if (w.copy_bits(1, r) == 1) { // video_signal_type_present_flag
    w.copy_bits(4, r);          // video_format, video_full_range_flag
    if (w.copy_bits(1, r) == 1) // color_desc_present_flag
      w.copy_bits(24, r);       // colour_primaries, transfer_characteristics, matrix_coefficients
  }
  if (w.copy_bits(1, r) == 1) { // chroma_loc_info_present_flag
    w.copy_unsigned_golomb(r);               // chroma_sample_loc_type_top_field
    w.copy_unsigned_golomb(r);               // chroma_sample_loc_type_bottom_field
  }
  w.copy_bits(3, r);            // neutral_chroma_indication_flag, field_seq_flag, frame_field_info_present_flag

  if (   (r.get_remaining_bits() >= 68)
      && (r.peek_bits(21) == 0x100000))
    w.put_bit(0);                    // invalid default display window, signal no default_display_window_flag
  else if (w.copy_bits(1, r) == 1) { // default_display_window_flag
    w.copy_unsigned_golomb(r);               // def_disp_win_left_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_right_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_top_offset
    w.copy_unsigned_golomb(r);               // def_disp_win_bottom_offset
  }
  sps.timing_info_present = w.copy_bits(1, r); // vui_timing_info_present_flag
  if (sps.timing_info_present) {
    sps.num_units_in_tick = w.copy_bits(32, r); // vui_num_units_in_tick
    sps.time_scale        = w.copy_bits(32, r); // vui_time_scale
    if (w.copy_bits(1, r) == 1) { // vui_poc_proportional_to_timing_flag
      w.copy_unsigned_golomb(r); // vui_num_ticks_poc_diff_one_minus1
    }
    if (w.copy_bits(1, r) == 1) { // vui_hrd_parameters_present_flag
      hrd_parameters_copy(r, w, 1, max_sub_layers_minus1); // hrd_parameters
    }
    if (w.copy_bits(1, r) == 1) { // bitstream_restriction_flag
      w.copy_bits(3, r);  // tiles_fixed_structure_flag, motion_vectors_over_pic_boundaries_flag, restricted_ref_pic_lists_flag
      sps.min_spatial_segmentation_idc = w.copy_unsigned_golomb(r); // min_spatial_segmentation_idc
      w.copy_unsigned_golomb(r); // max_bytes_per_pic_denom
      w.copy_unsigned_golomb(r); // max_bits_per_mincu_denom
      w.copy_unsigned_golomb(r); // log2_max_mv_length_horizontal
      w.copy_unsigned_golomb(r); // log2_max_mv_length_vertical
    }
  }

  if (w.copy_bits(1, r) == 1) { // bitstream_restriction_flag
    w.copy_bits(3, r);          // tiles_fixed_structure_flag, motion_vectors_over_pic_boundaries_flag, restricted_ref_pic_lists_flag
    w.copy_unsigned_golomb(r);  // min_spatial_segmentation_idc
    w.copy_unsigned_golomb(r);  // max_bytes_per_pic_denom
    w.copy_unsigned_golomb(r);  // max_bits_per_min_cu_denom
    w.copy_unsigned_golomb(r);  // log2_max_mv_length_horizontal
    w.copy_unsigned_golomb(r);  // log2_max_mv_length_vertical
  }
}

void
sps_info_t::dump() {
  mxinfo(boost::format("sps_info dump:\n"
                       "  id:                                    %1%\n"
                       "  log2_max_pic_order_cnt_lsb:            %2%\n"
                       "  vui_present:                           %3%\n"
                       "  ar_found:                              %4%\n"
                       "  par_num:                               %5%\n"
                       "  par_den:                               %6%\n"
                       "  timing_info_present:                   %7%\n"
                       "  num_units_in_tick:                     %8%\n"
                       "  time_scale:                            %9%\n"
                       "  width:                                 %10%\n"
                       "  height:                                %11%\n"
                       "  checksum:                              %|12$08x|\n")
         % id
         % log2_max_pic_order_cnt_lsb
         % vui_present
         % ar_found
         % par_num
         % par_den
         % timing_info_present
         % num_units_in_tick
         % time_scale
         % width
         % height
         % checksum);
}

bool
sps_info_t::timing_info_valid()
  const {
  return timing_info_present
    && (0 != num_units_in_tick)
    && (0 != time_scale);
}

int64_t
sps_info_t::default_duration()
  const {
  return 1000000000ll * num_units_in_tick / time_scale;
}

void
pps_info_t::dump() {
  mxinfo(boost::format("pps_info dump:\n"
                       "id: %1%\n"
                       "sps_id: %2%\n"
                       "checksum: %|3$08x|\n")
         % id
         % sps_id
         % checksum);
}

void
slice_info_t::dump()
  const {
  mxinfo(boost::format("slice_info dump:\n"
                       "  nalu_type:                  %1%\n"
                       "  type:                       %2%\n"
                       "  pps_id:                     %3%\n"
                       "  pic_order_cnt_lsb:          %4%\n"
                       "  sps:                        %5%\n"
                       "  pps:                        %6%\n")
         % static_cast<unsigned int>(nalu_type)
         % static_cast<unsigned int>(type)
         % static_cast<unsigned int>(pps_id)
         % pic_order_cnt_lsb
         % sps
         % pps);
}

bool
parse_vps(memory_cptr const &buffer,
          vps_info_t &vps) {
  auto size         = buffer->get_size();
  auto mcptr_newvps = memory_c::alloc(size + 100);
  auto newvps       = mcptr_newvps->get_buffer();
  bit_reader_c r(buffer->get_buffer(), size);
  bit_writer_c w(newvps, size + 100);
  unsigned int i, j;

  memset(&vps, 0, sizeof(vps));

  w.copy_bits(1, r);            // forbidden_zero_bit
  if (w.copy_bits(6, r) != HEVC_NALU_TYPE_VIDEO_PARAM)  // nal_unit_type
    return false;
  w.copy_bits(6, r);            // nuh_reserved_zero_6bits
  w.copy_bits(3, r);            // nuh_temporal_id_plus1

  vps.id = w.copy_bits(4, r);                       // vps_video_parameter_set_id
  w.copy_bits(2+6, r);                              // vps_reserved_three_2bits, vps_reserved_zero_6bits
  vps.max_sub_layers_minus1 = w.copy_bits(3, r);    // vps_max_sub_layers_minus1
  w.copy_bits(1+16, r);                             // vps_temporal_id_nesting_flag, vps_reserved_0xffff_16bits

  // At this point we are at newvps + 6 bytes, profile_tier_level follows
  profile_tier_copy(r, w, vps, vps.max_sub_layers_minus1);  // profile_tier_level(vps_max_sub_layers_minus1)

  bool vps_sub_layer_ordering_info_present_flag = w.copy_bits(1, r);  // vps_sub_layer_ordering_info_present_flag
  for (i = (vps_sub_layer_ordering_info_present_flag ? 0 : vps.max_sub_layers_minus1); i <= vps.max_sub_layers_minus1; i++) {
    w.copy_unsigned_golomb(r); // vps_max_dec_pic_buffering_minus1[i]
    w.copy_unsigned_golomb(r); // vps_max_num_reorder_pics[i]
    w.copy_unsigned_golomb(r); // vps_max_latency_increase[i]
  }

  unsigned int vps_max_nuh_reserved_zero_layer_id = w.copy_bits(6, r);  // vps_max_nuh_reserved_zero_layer_id
  bool vps_num_op_sets_minus1 = w.copy_unsigned_golomb(r);       // vps_num_op_sets_minus1
  for (i = 1; i <= vps_num_op_sets_minus1; i++) {
    for (j = 0; j <= vps_max_nuh_reserved_zero_layer_id; j++) { // operation_point_set(i)
      w.copy_bits(1, r);  // layer_id_included_flag
    }
  }

  if (w.copy_bits(1, r) == 1) { // vps_timing_info_present_flag
    w.copy_bits(32, r);         // vps_num_units_in_tick
    w.copy_bits(32, r);         // vps_time_scale
    if (w.copy_bits(1, r) == 1)  // vps_poc_proportional_to_timing_flag
      w.copy_unsigned_golomb(r);             // vps_num_ticks_poc_diff_one_minus1
    unsigned int vps_num_hrd_parameters = w.copy_unsigned_golomb(r); // vps_num_hrd_parameters
    for (i = 0; i < vps_num_hrd_parameters; i++) {
      bool cprms_present_flag = true; // 7.4.3.1 - cprms_present_flag[0] is inferred to be equal to 1.
      w.copy_unsigned_golomb(r);             // hrd_op_set_idx[i]
      if (i > 0)
        cprms_present_flag = w.copy_bits(1, r); // cprms_present_flag[i]
      hrd_parameters_copy(r, w, cprms_present_flag, vps.max_sub_layers_minus1);
    }
  }

  if (w.copy_bits(1, r) == 1)    // vps_extension_flag
    while (r.get_remaining_bits())
      w.copy_bits(1, r);        // vps_extension_data_flag

  w.put_bit(1);
  w.byte_align();

  // Given we don't change the NALU while writing to w,
  // then we don't need to replace buffer with the bits we've written into w.
  // Leaving this code as reference if we ever do change the NALU while writing to w.
  //buffer = mcptr_newvps;
  //buffer->set_size(w.get_bit_position() / 8);

  vps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffer);

  return true;
}

memory_cptr
parse_sps(memory_cptr const &buffer,
          sps_info_t &sps,
          std::vector<vps_info_t> &m_vps_info_list,
          bool keep_ar_info) {
  auto size         = buffer->get_size();
  auto mcptr_newsps = memory_c::alloc(size + 100);
  auto newsps       = mcptr_newsps->get_buffer();
  bit_reader_c r(buffer->get_buffer(), size);
  bit_writer_c w(newsps, size + 100);
  unsigned int i;

  keep_ar_info = !hack_engaged(ENGAGE_REMOVE_BITSTREAM_AR_INFO);

  memset(&sps, 0, sizeof(sps));

  sps.par_num  = 1;
  sps.par_den  = 1;
  sps.ar_found = false;

  w.copy_bits(1, r);            // forbidden_zero_bit
  if (w.copy_bits(6, r) != HEVC_NALU_TYPE_SEQ_PARAM)  // nal_unit_type
    return {};
  w.copy_bits(6, r);            // nuh_reserved_zero_6bits
  w.copy_bits(3, r);            // nuh_temporal_id_plus1

  sps.vps_id                   = w.copy_bits(4, r); // sps_video_parameter_set_id
  sps.max_sub_layers_minus1    = w.copy_bits(3, r); // sps_max_sub_layers_minus1
  sps.temporal_id_nesting_flag = w.copy_bits(1, r); // sps_temporal_id_nesting_flag

  size_t vps_idx;
  for (vps_idx = 0; m_vps_info_list.size() > vps_idx; ++vps_idx)
    if (m_vps_info_list[vps_idx].id == sps.vps_id)
      break;
  if (m_vps_info_list.size() == vps_idx)
    return {};

  sps.vps = vps_idx;

  auto &vps = m_vps_info_list[vps_idx];

  profile_tier_copy(r, w, vps, sps.max_sub_layers_minus1);  // profile_tier_level(sps_max_sub_layers_minus1)

  sps.id = w.copy_unsigned_golomb(r);  // sps_seq_parameter_set_id

  if ((sps.chroma_format_idc = w.copy_unsigned_golomb(r)) == 3) // chroma_format_idc
    sps.separate_colour_plane_flag = w.copy_bits(1, r);    // separate_colour_plane_flag

  sps.width                   = w.copy_unsigned_golomb(r); // pic_width_in_luma_samples
  sps.height                  = w.copy_unsigned_golomb(r); // pic_height_in_luma_samples
  sps.conformance_window_flag = w.copy_bits(1, r);

  if (sps.conformance_window_flag) {
    sps.conf_win_left_offset   = w.copy_unsigned_golomb(r); // conf_win_left_offset
    sps.conf_win_right_offset  = w.copy_unsigned_golomb(r); // conf_win_right_offset
    sps.conf_win_top_offset    = w.copy_unsigned_golomb(r); // conf_win_top_offset
    sps.conf_win_bottom_offset = w.copy_unsigned_golomb(r); // conf_win_bottom_offset
  }

  sps.bit_depth_luma_minus8                     = w.copy_unsigned_golomb(r);     // bit_depth_luma_minus8
  sps.bit_depth_chroma_minus8                   = w.copy_unsigned_golomb(r);     // bit_depth_chroma_minus8
  sps.log2_max_pic_order_cnt_lsb                = w.copy_unsigned_golomb(r) + 4; // log2_max_pic_order_cnt_lsb_minus4
  auto sps_sub_layer_ordering_info_present_flag = w.copy_bits(1, r);             // sps_sub_layer_ordering_info_present_flag

  for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps.max_sub_layers_minus1); i <= sps.max_sub_layers_minus1; i++) {
    w.copy_unsigned_golomb(r); // sps_max_dec_pic_buffering_minus1[i]
    w.copy_unsigned_golomb(r); // sps_max_num_reorder_pics[i]
    w.copy_unsigned_golomb(r); // sps_max_latency_increase[i]
  }

  sps.log2_min_luma_coding_block_size_minus3   = w.copy_unsigned_golomb(r); // log2_min_luma_coding_block_size_minus3
  sps.log2_diff_max_min_luma_coding_block_size = w.copy_unsigned_golomb(r); // log2_diff_max_min_luma_coding_block_size

  w.copy_unsigned_golomb(r); // log2_min_transform_block_size_minus2
  w.copy_unsigned_golomb(r); // log2_diff_max_min_transform_block_size
  w.copy_unsigned_golomb(r); // max_transform_hierarchy_depth_inter
  w.copy_unsigned_golomb(r); // max_transform_hierarchy_depth_intra

  if (w.copy_bits(1, r) == 1)   // scaling_list_enabled_flag
    if (w.copy_bits(1, r) == 1) // sps_scaling_list_data_present_flag
      scaling_list_data_copy(r, w);

  w.copy_bits(1, r);  // amp_enabled_flag
  w.copy_bits(1, r);  // sample_adaptive_offset_enabled_flag

  if (w.copy_bits(1, r) == 1) { // pcm_enabled_flag
    w.copy_bits(4, r);          // pcm_sample_bit_depth_luma_minus1
    w.copy_bits(4, r);          // pcm_sample_bit_depth_chroma_minus1
    w.copy_unsigned_golomb(r);  // log2_min_pcm_luma_coding_block_size_minus3
    w.copy_unsigned_golomb(r);  // log2_diff_max_min_pcm_luma_coding_block_size
    w.copy_bits(1, r);          // pcm_loop_filter_disable_flag
  }

  auto num_short_term_ref_pic_sets = w.copy_unsigned_golomb(r);  // num_short_term_ref_pic_sets

  for (i = 0; i < num_short_term_ref_pic_sets; i++) {
    short_term_ref_pic_set_copy(r, w, sps.short_term_ref_pic_sets, i, num_short_term_ref_pic_sets); // short_term_ref_pic_set(i)
  }

  if (w.copy_bits(1, r) == 1) { // long_term_ref_pics_present_flag
    auto num_long_term_ref_pic_sets = w.copy_unsigned_golomb(r); // num_long_term_ref_pic_sets

    for (i = 0; i < num_long_term_ref_pic_sets; i++) {
      w.copy_bits(sps.log2_max_pic_order_cnt_lsb, r);  // lt_ref_pic_poc_lsb_sps[i]
      w.copy_bits(1, r);                               // used_by_curr_pic_lt_sps_flag[i]
    }
  }

  w.copy_bits(1, r);                   // sps_temporal_mvp_enabled_flag
  w.copy_bits(1, r);                   // strong_intra_smoothing_enabled_flag
  sps.vui_present = w.copy_bits(1, r); // vui_parameters_present_flag

  if (sps.vui_present == 1) {
    vui_parameters_copy(r, w, sps, keep_ar_info, sps.max_sub_layers_minus1);  //vui_parameters()
  }

  if (w.copy_bits(1, r) == 1) // sps_extension_flag
    while (r.get_remaining_bits())
      w.copy_bits(1, r);  // sps_extension_data_flag

  w.put_bit(1);
  w.byte_align();

  // We potentially changed the NALU data with regards to the handling of keep_ar_info.
  // Therefore, we replace buffer with the changed NALU that exists in w.
  mcptr_newsps->resize(w.get_bit_position() / 8);

  sps.checksum = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *mcptr_newsps);

  // See ITU-T H.265 section 7.4.3.2 for the width/height calculation
  // formula.
  if (sps.conformance_window_flag) {
    auto sub_width_c  = ((1 == sps.chroma_format_idc) || (2 == sps.chroma_format_idc)) && (0 == sps.separate_colour_plane_flag) ? 2 : 1;
    auto sub_height_c =  (1 == sps.chroma_format_idc)                                  && (0 == sps.separate_colour_plane_flag) ? 2 : 1;

    sps.width        -= std::min<unsigned int>((sub_width_c  * sps.conf_win_right_offset)  + (sub_width_c  * sps.conf_win_left_offset), sps.width);
    sps.height       -= std::min<unsigned int>((sub_height_c * sps.conf_win_bottom_offset) + (sub_height_c * sps.conf_win_top_offset),  sps.height);
  }

  return mcptr_newsps;
}

bool
parse_pps(memory_cptr const &buffer,
          pps_info_t &pps) {
  try {
    bit_reader_c r(buffer->get_buffer(), buffer->get_size());

    memset(&pps, 0, sizeof(pps));

    r.skip_bits(1);                                // forbidden_zero_bit
    if (r.get_bits(6) != HEVC_NALU_TYPE_PIC_PARAM) // nal_unit_type
      return false;
    r.skip_bits(6);             // nuh_reserved_zero_6bits
    r.skip_bits(3);             // nuh_temporal_id_plus1

    pps.id                                    = r.get_unsigned_golomb(); // pps_pic_parameter_set_id
    pps.sps_id                                = r.get_unsigned_golomb(); // pps_seq_parameter_set_id
    pps.dependent_slice_segments_enabled_flag = r.get_bits(1);           // dependent_slice_segments_enabled_flag
    pps.output_flag_present_flag              = r.get_bits(1);           // output_flag_present_flag
    pps.num_extra_slice_header_bits           = r.get_bits(3);           // num_extra_slice_header_bits

    pps.checksum                              = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *buffer);

    return true;
  } catch (...) {
    return false;
  }
}

// HEVC spec, 7.3.2.4
bool
parse_sei(memory_cptr const &buffer,
          user_data_t &user_data) {
  try {
    bit_reader_c r(buffer->get_buffer(), buffer->get_size());
    mm_mem_io_c byte_reader{*buffer};

    unsigned int bytes_read   = 0;
    unsigned int buffer_size  = buffer->get_size();
    unsigned int payload_type = 0;
    unsigned int payload_size = 0;

    r.skip_bits(1);                                 // forbidden_zero_bit
    if (r.get_bits(6) != HEVC_NALU_TYPE_PREFIX_SEI) // nal_unit_type
      return false;
    r.skip_bits(6);             // nuh_reserved_zero_6bits
    r.skip_bits(3);             // nuh_temporal_id_plus1

    byte_reader.skip(2);        // skip the nalu header
    bytes_read += 2;

    while(bytes_read < buffer_size-2) {
      payload_type = 0;

      unsigned int payload_type_byte = byte_reader.read_uint8();
      bytes_read++;

      while(payload_type_byte == 0xFF) {
        payload_type     += 255;
        payload_type_byte = byte_reader.read_uint8();
        bytes_read++;
      }

      payload_type += payload_type_byte;
      payload_size  = 0;

      unsigned int payload_size_byte = byte_reader.read_uint8();
      bytes_read++;

      while(payload_size_byte == 0xFF) {
        payload_size     += 255;
        payload_size_byte = byte_reader.read_uint8();
        bytes_read++;
      }
      payload_size += payload_size_byte;

      handle_sei_payload(byte_reader, payload_type, payload_size, user_data);

      bytes_read += payload_size;
    }

    return true;
  } catch (...) {
    return false;
  }
}

bool
handle_sei_payload(mm_mem_io_c &byte_reader,
                   unsigned int sei_payload_type,
                   unsigned int sei_payload_size,
                   user_data_t &user_data) {
  std::vector<unsigned char> uuid;
  uint64 file_pos = byte_reader.getFilePointer();

  uuid.resize(16);
  if (sei_payload_type == HEVC_SEI_USER_DATA_UNREGISTERED) {
    if (sei_payload_size >= 16) {
      byte_reader.read(&uuid[0], 16); // uuid

      if (user_data.find(uuid) == user_data.end()) {
        std::vector<unsigned char> &payload = user_data[uuid];

        payload.resize(sei_payload_size);
        memcpy(&payload[0], &uuid[0], 16);
        byte_reader.read(&payload[16], sei_payload_size - 16);
      }
    }
  }

  // Go to end of SEI data by going to its beginning and then skipping over it.
  byte_reader.setFilePointer(file_pos);
  byte_reader.skip(sei_payload_size);

  return true;
}

/** Extract the pixel aspect ratio from the HEVC codec data

   This function searches a buffer containing the HEVC
   codec initialization for the pixel aspectc ratio. If it is found
   then the numerator and the denominator are extracted, and the
   aspect ratio information is removed from the buffer. A structure
   containing the new buffer, the numerator/denominator and the
   success status is returned.

   \param buffer The buffer containing the HEVC codec data.

   \return A \c par_extraction_t structure.
*/
par_extraction_t
extract_par(memory_cptr const &buffer) {
  static debugging_option_c s_debug_ar{"extract_par|hevc_sps|sps_aspect_ratio"};

  try {
    auto hevcc     = hevcc_c::unpack(buffer);
    auto new_hevcc = hevcc;
    bool ar_found = false;
    unsigned int par_num = 1;
    unsigned int par_den = 1;

    new_hevcc.m_sps_list.clear();

    for (auto &nalu : hevcc.m_sps_list) {
      if (!ar_found) {
        try {
          sps_info_t sps_info;
          auto parsed_nalu = parse_sps(mpeg::nalu_to_rbsp(nalu), sps_info, new_hevcc.m_vps_info_list);

          if (parsed_nalu) {
            if (s_debug_ar)
              sps_info.dump();

            ar_found = sps_info.ar_found;
            if (ar_found) {
              par_num = sps_info.par_num;
              par_den = sps_info.par_den;
            }

            nalu = mpeg::rbsp_to_nalu(parsed_nalu);
          }
        } catch (mtx::mm_io::end_of_file_x &) {
        }
      }

      new_hevcc.m_sps_list.push_back(nalu);
    }

    if (!new_hevcc)
      return par_extraction_t{buffer, 0, 0, false};

    return par_extraction_t{new_hevcc.pack(), ar_found ? par_num : 0, ar_found ? par_den : 0, true};

  } catch(...) {
    return par_extraction_t{buffer, 0, 0, false};
  }
}

bool
is_fourcc(const char *fourcc) {
  return balg::to_lower_copy(std::string{fourcc, 4}) == "hevc";
}

memory_cptr
hevcc_to_nalus(const unsigned char *buffer,
               size_t size) {
  try {
    if (6 > size)
      throw false;

    uint32_t marker = get_uint32_be(buffer);
    if (((marker & 0xffffff00) == 0x00000100) || (0x00000001 == marker))
      return memory_c::clone(buffer, size);

    mm_mem_io_c mem(buffer, size);
    byte_buffer_c nalus(size * 2);

    if (0x01 != mem.read_uint8())
      throw false;

    mem.setFilePointer(4, seek_beginning);
    size_t nal_size_size = 1 + (mem.read_uint8() & 3);
    if (2 > nal_size_size)
      throw false;

    size_t sps_or_pps;
    for (sps_or_pps = 0; 2 > sps_or_pps; ++sps_or_pps) {
      unsigned int num = mem.read_uint8();
      if (0 == sps_or_pps)
        num &= 0x1f;

      size_t i;
      for (i = 0; num > i; ++i) {
        uint16_t element_size   = mem.read_uint16_be();
        memory_cptr copy_buffer = memory_c::alloc(element_size + 4);
        if (element_size != mem.read(copy_buffer->get_buffer() + 4, element_size))
          throw false;

        put_uint32_be(copy_buffer->get_buffer(), NALU_START_CODE);
        nalus.add(copy_buffer->get_buffer(), element_size + 4);
      }
    }

    if (mem.getFilePointer() == size)
      return memory_c::clone(nalus.get_buffer(), nalus.get_size());

  } catch (...) {
  }

  return memory_cptr{};
}

es_parser_c::es_parser_c()
  : m_nalu_size_length(4)
  , m_keep_ar_info(true)
  , m_hevcc_ready(false)
  , m_hevcc_changed(false)
  , m_stream_default_duration(-1)
  , m_forced_default_duration(-1)
  , m_container_default_duration(-1)
  , m_frame_number(0)
  , m_num_skipped_frames(0)
  , m_first_keyframe_found(false)
  , m_recovery_point_valid(false)
  , m_b_frames_since_keyframe(false)
  , m_first_cleanup{true}
  , m_par_found(false)
  , m_max_timecode(0)
  , m_stream_position(0)
  , m_parsed_position(0)
  , m_have_incomplete_frame(false)
  , m_simple_picture_order{}
  , m_ignore_nalu_size_length_errors(false)
  , m_discard_actual_frames(false)
  , m_debug_keyframe_detection(debugging_c::requested("hevc_parser|hevc_keyframe_detection"))
  , m_debug_nalu_types(        debugging_c::requested("hevc_parser|hevc_nalu_types"))
  , m_debug_timecodes(         debugging_c::requested("hevc_parser|hevc_timecodes"))
  , m_debug_sps_info(          debugging_c::requested("hevc_parser|hevc_sps|hevc_sps_info"))
{
}

es_parser_c::~es_parser_c() {
  mxdebug_if(debugging_c::requested("hevc_statistics"),
             boost::format("HEVC statistics: #frames: out %1% discarded %2% #timecodes: in %3% generated %4% discarded %5% num_fields: %6% num_frames: %7%\n")
             % m_stats.num_frames_out % m_stats.num_frames_discarded % m_stats.num_timecodes_in % m_stats.num_timecodes_generated % m_stats.num_timecodes_discarded
             % m_stats.num_field_slices % m_stats.num_frame_slices);

  mxdebug_if(m_debug_timecodes, boost::format("stream_position %1% parsed_position %2%\n") % m_stream_position % m_parsed_position);

  if (!debugging_c::requested("hevc_num_slices_by_type"))
    return;

  static const char *s_type_names[] = {
    "B",  "P",  "I", "unknown"
  };

  int i;
  mxdebug("hevc: Number of slices by type:\n");
  for (i = 0; 2 >= i; ++i)
    if (0 != m_stats.num_slices_by_type[i])
      mxdebug(boost::format("  %1%: %2%\n") % s_type_names[i] % m_stats.num_slices_by_type[i]);
}

bool
es_parser_c::headers_parsed()
  const {
  return m_hevcc_ready
      && !m_sps_info_list.empty()
      && (m_sps_info_list.front().width  > 0)
      && (m_sps_info_list.front().height > 0);
}

void
es_parser_c::discard_actual_frames(bool discard) {
  m_discard_actual_frames = discard;
}

void
es_parser_c::add_bytes(unsigned char *buffer,
                       size_t size) {
  memory_slice_cursor_c cursor;
  int marker_size              = 0;
  int previous_marker_size     = 0;
  int previous_pos             = -1;
  uint64_t previous_parsed_pos = m_parsed_position;

  if (m_unparsed_buffer && (0 != m_unparsed_buffer->get_size()))
    cursor.add_slice(m_unparsed_buffer);
  cursor.add_slice(buffer, size);

  if (3 <= cursor.get_remaining_size()) {
    uint32_t marker =                               1 << 24
                    | (unsigned int)cursor.get_char() << 16
                    | (unsigned int)cursor.get_char() <<  8
                    | (unsigned int)cursor.get_char();

    while (1) {
      if (NALU_START_CODE == marker)
        marker_size = 4;
      else if (NALU_START_CODE == (marker & 0x00ffffff))
        marker_size = 3;

      if (0 != marker_size) {
        if (-1 != previous_pos) {
          auto new_size = cursor.get_position() - marker_size - previous_pos - previous_marker_size;
          auto nalu     = memory_c::alloc(new_size);
          cursor.copy(nalu->get_buffer(), previous_pos + previous_marker_size, new_size);
          m_parsed_position = previous_parsed_pos + previous_pos;

          mtx::mpeg::remove_trailing_zero_bytes(*nalu);
          if (nalu->get_size())
            handle_nalu(nalu, m_parsed_position);
        }
        previous_pos         = cursor.get_position() - marker_size;
        previous_marker_size = marker_size;
        marker_size          = 0;
      }

      if (!cursor.char_available())
        break;

      marker <<= 8;
      marker  |= (unsigned int)cursor.get_char();
    }
  }

  if (-1 == previous_pos)
    previous_pos = 0;

  m_stream_position += size;
  m_parsed_position  = previous_parsed_pos + previous_pos;

  int new_size = cursor.get_size() - previous_pos;
  if (0 != new_size) {
    m_unparsed_buffer = memory_c::alloc(new_size);
    cursor.copy(m_unparsed_buffer->get_buffer(), previous_pos, new_size);

  } else
    m_unparsed_buffer.reset();
}

void
es_parser_c::flush() {
  if (m_unparsed_buffer && (5 <= m_unparsed_buffer->get_size())) {
    m_parsed_position += m_unparsed_buffer->get_size();
    auto marker_size   = get_uint32_be(m_unparsed_buffer->get_buffer()) == NALU_START_CODE ? 4 : 3;
    auto nalu_size     = m_unparsed_buffer->get_size() - marker_size;
    handle_nalu(memory_c::clone(m_unparsed_buffer->get_buffer() + marker_size, nalu_size), m_parsed_position - nalu_size);
  }

  m_unparsed_buffer.reset();
  if (m_have_incomplete_frame) {
    m_frames.push_back(m_incomplete_frame);
    m_have_incomplete_frame = false;
  }

  cleanup();
}

void
es_parser_c::add_timecode(int64_t timecode) {
  m_provided_timestamps.emplace_back(timecode, m_stream_position);
  ++m_stats.num_timecodes_in;
}

void
es_parser_c::flush_incomplete_frame() {
  if (!m_have_incomplete_frame || !m_hevcc_ready)
    return;

  m_frames.push_back(m_incomplete_frame);
  m_incomplete_frame.clear();
  m_have_incomplete_frame = false;
}

void
es_parser_c::flush_unhandled_nalus() {
  for (auto const &nalu_with_pos : m_unhandled_nalus)
    handle_nalu(nalu_with_pos.first, nalu_with_pos.second);

  m_unhandled_nalus.clear();
}

void
es_parser_c::handle_slice_nalu(memory_cptr const &nalu,
                               uint64_t nalu_pos) {
  if (!m_hevcc_ready) {
    m_unhandled_nalus.emplace_back(nalu, nalu_pos);
    return;
  }

  slice_info_t si;
  if (!parse_slice(mpeg::nalu_to_rbsp(nalu), si))
    return;

  if (m_have_incomplete_frame && si.first_slice_segment_in_pic_flag)
    flush_incomplete_frame();

  if (m_have_incomplete_frame) {
    memory_c &mem = *(m_incomplete_frame.m_data.get());
    int offset    = mem.get_size();
    mem.resize(offset + m_nalu_size_length + nalu->get_size());
    mtx::mpeg::write_nalu_size(mem.get_buffer() + offset, nalu->get_size(), m_nalu_size_length, m_ignore_nalu_size_length_errors);
    memcpy(mem.get_buffer() + offset + m_nalu_size_length, nalu->get_buffer(), nalu->get_size());

    return;
  }

  bool is_i_slice =  (HEVC_SLICE_TYPE_I == si.type);
  bool is_b_slice =  (HEVC_SLICE_TYPE_B == si.type);

  m_incomplete_frame.m_si       =  si;
  m_incomplete_frame.m_keyframe =  m_recovery_point_valid
                                || (   is_i_slice
                                    && (   (m_debug_keyframe_detection && !m_b_frames_since_keyframe)
                                        || (HEVC_NALU_TYPE_IDR_W_RADL == si.nalu_type)
                                        || (HEVC_NALU_TYPE_IDR_N_LP   == si.nalu_type)
                                        || (HEVC_NALU_TYPE_CRA_NUT    == si.nalu_type)));
  m_incomplete_frame.m_position = nalu_pos;
  m_recovery_point_valid        = false;

  if (m_incomplete_frame.m_keyframe) {
    m_first_keyframe_found    = true;
    m_b_frames_since_keyframe = false;
    cleanup();

  } else
    m_b_frames_since_keyframe |= is_b_slice;

  m_incomplete_frame.m_data = create_nalu_with_size(nalu, true);
  m_have_incomplete_frame   = true;

  ++m_frame_number;
}

void
es_parser_c::handle_vps_nalu(memory_cptr const &nalu) {
  vps_info_t vps_info;

  if (!parse_vps(mpeg::nalu_to_rbsp(nalu), vps_info))
    return;

  size_t i;
  for (i = 0; m_vps_info_list.size() > i; ++i)
    if (m_vps_info_list[i].id == vps_info.id)
      break;

  if (m_vps_info_list.size() == i) {
    m_vps_list.push_back(nalu);
    m_vps_info_list.push_back(vps_info);
    m_hevcc_changed = true;

  } else if (m_vps_info_list[i].checksum != vps_info.checksum) {
    mxverb(2, boost::format("hevc: VPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % vps_info.id % m_vps_info_list[i].checksum % vps_info.checksum);

    m_vps_info_list[i] = vps_info;
    m_vps_list[i]      = nalu;
    m_hevcc_changed    = true;

    // Update codec private if needed
    if (m_codec_private.vps_data_id == (int) vps_info.id) {
      m_codec_private.profile_space              = vps_info.profile_space;
      m_codec_private.tier_flag                  = vps_info.tier_flag;
      m_codec_private.profile_idc                = vps_info.profile_idc;
      m_codec_private.profile_compatibility_flag = vps_info.profile_compatibility_flag;
      m_codec_private.progressive_source_flag    = vps_info.progressive_source_flag;
      m_codec_private.interlaced_source_flag     = vps_info.interlaced_source_flag;
      m_codec_private.non_packed_constraint_flag = vps_info.non_packed_constraint_flag;
      m_codec_private.frame_only_constraint_flag = vps_info.frame_only_constraint_flag;
      m_codec_private.level_idc                  = vps_info.level_idc;
      m_codec_private.vps_data_id                = vps_info.id;
    }
  }

  // Update codec private if needed
  if (-1 == m_codec_private.vps_data_id) {
    m_codec_private.profile_space              = vps_info.profile_space;
    m_codec_private.tier_flag                  = vps_info.tier_flag;
    m_codec_private.profile_idc                = vps_info.profile_idc;
    m_codec_private.profile_compatibility_flag = vps_info.profile_compatibility_flag;
    m_codec_private.progressive_source_flag    = vps_info.progressive_source_flag;
    m_codec_private.interlaced_source_flag     = vps_info.interlaced_source_flag;
    m_codec_private.non_packed_constraint_flag = vps_info.non_packed_constraint_flag;
    m_codec_private.frame_only_constraint_flag = vps_info.frame_only_constraint_flag;
    m_codec_private.level_idc                  = vps_info.level_idc;
    m_codec_private.vps_data_id                = vps_info.id;
  }

  m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_sps_nalu(memory_cptr const &nalu) {
  sps_info_t sps_info;

  auto parsed_nalu = parse_sps(mpeg::nalu_to_rbsp(nalu), sps_info, m_vps_info_list, m_keep_ar_info);
  if (!parsed_nalu)
    return;

  parsed_nalu = mpeg::rbsp_to_nalu(parsed_nalu);

  size_t i;
  for (i = 0; m_sps_info_list.size() > i; ++i)
    if (m_sps_info_list[i].id == sps_info.id)
      break;

  bool use_sps_info = true;
  if (m_sps_info_list.size() == i) {
    m_sps_list.push_back(parsed_nalu);
    m_sps_info_list.push_back(sps_info);
    m_hevcc_changed = true;

  } else if (m_sps_info_list[i].checksum != sps_info.checksum) {
    mxverb(2, boost::format("hevc: SPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % sps_info.id % m_sps_info_list[i].checksum % sps_info.checksum);

    m_sps_info_list[i] = sps_info;
    m_sps_list[i]      = parsed_nalu;
    m_hevcc_changed    = true;

    // Update codec private if needed
    if (m_codec_private.sps_data_id == (int) sps_info.id) {
      m_codec_private.min_spatial_segmentation_idc = sps_info.min_spatial_segmentation_idc;
      m_codec_private.chroma_format_idc = sps_info.chroma_format_idc;
      m_codec_private.bit_depth_luma_minus8 = sps_info.bit_depth_luma_minus8;
      m_codec_private.bit_depth_chroma_minus8 = sps_info.bit_depth_chroma_minus8;
      m_codec_private.max_sub_layers_minus1 = sps_info.max_sub_layers_minus1;
      m_codec_private.temporal_id_nesting_flag = sps_info.temporal_id_nesting_flag;
    }
  } else
    use_sps_info = false;

  m_extra_data.push_back(create_nalu_with_size(parsed_nalu));

  // Update codec private if needed
  if (-1 == m_codec_private.sps_data_id) {
    m_codec_private.min_spatial_segmentation_idc = sps_info.min_spatial_segmentation_idc;
    m_codec_private.chroma_format_idc = sps_info.chroma_format_idc;
    m_codec_private.bit_depth_luma_minus8 = sps_info.bit_depth_luma_minus8;
    m_codec_private.bit_depth_chroma_minus8 = sps_info.bit_depth_chroma_minus8;
    m_codec_private.max_sub_layers_minus1 = sps_info.max_sub_layers_minus1;
    m_codec_private.temporal_id_nesting_flag = sps_info.temporal_id_nesting_flag;
    m_codec_private.sps_data_id = sps_info.id;
  }

  if (use_sps_info && m_debug_sps_info)
    sps_info.dump();

  if (!use_sps_info)
    return;

  if (!has_stream_default_duration()
      && sps_info.timing_info_valid()) {
    m_stream_default_duration = sps_info.default_duration();
    mxdebug_if(m_debug_timecodes, boost::format("Stream default duration: %1%\n") % m_stream_default_duration);
  }

  if (!m_par_found
      && sps_info.ar_found
      && (0 != sps_info.par_den)) {
    m_par_found = true;
    m_par       = int64_rational_c(sps_info.par_num, sps_info.par_den);
  }
}

void
es_parser_c::handle_pps_nalu(memory_cptr const &nalu) {
  pps_info_t pps_info;

  if (!parse_pps(mpeg::nalu_to_rbsp(nalu), pps_info))
    return;

  size_t i;
  for (i = 0; m_pps_info_list.size() > i; ++i)
    if (m_pps_info_list[i].id == pps_info.id)
      break;

  if (m_pps_info_list.size() == i) {
    m_pps_list.push_back(nalu);
    m_pps_info_list.push_back(pps_info);
    m_hevcc_changed = true;

  } else if (m_pps_info_list[i].checksum != pps_info.checksum) {
    mxverb(2, boost::format("hevc: PPS ID %|1$04x| changed; checksum old %|2$04x| new %|3$04x|\n") % pps_info.id % m_pps_info_list[i].checksum % pps_info.checksum);

    m_pps_info_list[i] = pps_info;
    m_pps_list[i]      = nalu;
    m_hevcc_changed     = true;
  }

  m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_sei_nalu(memory_cptr const &nalu) {
  if (parse_sei(mpeg::nalu_to_rbsp(nalu), m_user_data))
    m_extra_data.push_back(create_nalu_with_size(nalu));
}

void
es_parser_c::handle_nalu(memory_cptr const &nalu,
                         uint64_t nalu_pos) {
  if (1 > nalu->get_size())
    return;

  int type = (*(nalu->get_buffer()) >> 1) & 0x3F;

  mxdebug_if(m_debug_nalu_types, boost::format("NALU type 0x%|1$02x| (%2%) size %3%\n") % type % get_nalu_type_name(type) % nalu->get_size());

  switch (type) {
    case HEVC_NALU_TYPE_VIDEO_PARAM:
      flush_incomplete_frame();
      handle_vps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_SEQ_PARAM:
      flush_incomplete_frame();
      handle_sps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_PIC_PARAM:
      flush_incomplete_frame();
      handle_pps_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_PREFIX_SEI:
      flush_incomplete_frame();
      handle_sei_nalu(nalu);
      break;

    case HEVC_NALU_TYPE_END_OF_SEQ:
    case HEVC_NALU_TYPE_END_OF_STREAM:
    case HEVC_NALU_TYPE_ACCESS_UNIT:
      flush_incomplete_frame();
      break;

    case HEVC_NALU_TYPE_FILLER_DATA:
      // Skip these.
      break;

    case HEVC_NALU_TYPE_TRAIL_N:
    case HEVC_NALU_TYPE_TRAIL_R:
    case HEVC_NALU_TYPE_TSA_N:
    case HEVC_NALU_TYPE_TSA_R:
    case HEVC_NALU_TYPE_STSA_N:
    case HEVC_NALU_TYPE_STSA_R:
    case HEVC_NALU_TYPE_RADL_N:
    case HEVC_NALU_TYPE_RADL_R:
    case HEVC_NALU_TYPE_RASL_N:
    case HEVC_NALU_TYPE_RASL_R:
    case HEVC_NALU_TYPE_BLA_W_LP:
    case HEVC_NALU_TYPE_BLA_W_RADL:
    case HEVC_NALU_TYPE_BLA_N_LP:
    case HEVC_NALU_TYPE_IDR_W_RADL:
    case HEVC_NALU_TYPE_IDR_N_LP:
    case HEVC_NALU_TYPE_CRA_NUT:
      if (!m_hevcc_ready && !m_vps_info_list.empty() && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_hevcc_ready = true;
        flush_unhandled_nalus();
      }
      handle_slice_nalu(nalu, nalu_pos);
      break;

    default:
      flush_incomplete_frame();
      if (!m_hevcc_ready && !m_vps_info_list.empty() && !m_sps_info_list.empty() && !m_pps_info_list.empty()) {
        m_hevcc_ready = true;
        flush_unhandled_nalus();
      }
      m_extra_data.push_back(create_nalu_with_size(nalu));

      break;
  }
}

bool
es_parser_c::parse_slice(memory_cptr const &buffer,
                         slice_info_t &si) {
  try {
    bit_reader_c r(buffer->get_buffer(), buffer->get_size());
    unsigned int i;

    memset(&si, 0, sizeof(si));

    r.get_bits(1);                // forbidden_zero_bit
    si.nalu_type = r.get_bits(6); // nal_unit_type
    r.get_bits(6);                // nuh_reserved_zero_6bits
    r.get_bits(3);                // nuh_temporal_id_plus1

    bool RapPicFlag = (si.nalu_type >= 16 && si.nalu_type <= 23); // RapPicFlag
    si.first_slice_segment_in_pic_flag = r.get_bits(1); // first_slice_segment_in_pic_flag

    if (RapPicFlag)
      r.get_bits(1);  // no_output_of_prior_pics_flag

    si.pps_id = r.get_unsigned_golomb();  // slice_pic_parameter_set_id

    size_t pps_idx;
    for (pps_idx = 0; m_pps_info_list.size() > pps_idx; ++pps_idx)
      if (m_pps_info_list[pps_idx].id == si.pps_id)
        break;
    if (m_pps_info_list.size() == pps_idx) {
      mxverb(3, boost::format("slice parser error: PPS not found: %1%\n") % si.pps_id);
      return false;
    }

    pps_info_t &pps = m_pps_info_list[pps_idx];
    size_t sps_idx;
    for (sps_idx = 0; m_sps_info_list.size() > sps_idx; ++sps_idx)
      if (m_sps_info_list[sps_idx].id == pps.sps_id)
        break;
    if (m_sps_info_list.size() == sps_idx)
      return false;

    si.sps = sps_idx;
    si.pps = pps_idx;

    sps_info_t &sps = m_sps_info_list[sps_idx];

    bool dependent_slice_segment_flag = false;
    if (!si.first_slice_segment_in_pic_flag) {
      if (pps.dependent_slice_segments_enabled_flag)
        dependent_slice_segment_flag = r.get_bits(1); // dependent_slice_segment_flag

      auto log2_min_cb_size_y   = sps.log2_min_luma_coding_block_size_minus3 + 3;
      auto log2_ctb_size_y      = log2_min_cb_size_y + sps.log2_diff_max_min_luma_coding_block_size;
      auto ctb_size_y           = 1 << log2_ctb_size_y;
      auto pic_width_in_ctbs_y  = ceil(static_cast<double>(sps.width  / ctb_size_y));
      auto pic_height_in_ctbs_y = ceil(static_cast<double>(sps.height / ctb_size_y));
      auto pic_size_in_ctbs_y   = pic_width_in_ctbs_y * pic_height_in_ctbs_y;
      auto v                    = mtx::math::int_log2(pic_size_in_ctbs_y);

      r.get_bits(v);  // slice_segment_address
    }

    if (!dependent_slice_segment_flag) {
      for (i = 0; i < pps.num_extra_slice_header_bits; i++)
        r.get_bits(1);  // slice_reserved_undetermined_flag[i]

      si.type = r.get_unsigned_golomb();  // slice_type

      if (pps.output_flag_present_flag)
        r.get_bits(1);    // pic_output_flag

      if (sps.separate_colour_plane_flag == 1)
        r.get_bits(1);    // colour_plane_id

      if ( (si.nalu_type != HEVC_NALU_TYPE_IDR_W_RADL) && (si.nalu_type != HEVC_NALU_TYPE_IDR_N_LP) ) {
        si.pic_order_cnt_lsb = r.get_bits(sps.log2_max_pic_order_cnt_lsb); // slice_pic_order_cnt_lsb
      }

      ++m_stats.num_slices_by_type[1 < si.type ? 2 : si.type];
    }

    return true;
  } catch (...) {
    return false;
  }
}

int64_t
es_parser_c::duration_for(slice_info_t const &si)
  const {
  int64_t duration = -1 != m_forced_default_duration                                                  ? m_forced_default_duration * 2
                   : (m_sps_info_list.size() > si.sps) && m_sps_info_list[si.sps].timing_info_valid() ? m_sps_info_list[si.sps].default_duration()
                   : -1 != m_stream_default_duration                                                  ? m_stream_default_duration * 2
                   : -1 != m_container_default_duration                                               ? m_container_default_duration * 2
                   :                                                                                    20000000 * 2;
  return duration;
}

int64_t
es_parser_c::get_most_often_used_duration()
  const {
  int64_t const s_common_default_durations[] = {
    1000000000ll / 50,
    1000000000ll / 25,
    1000000000ll / 60,
    1000000000ll / 30,
    1000000000ll * 1001 / 48000,
    1000000000ll * 1001 / 24000,
    1000000000ll * 1001 / 60000,
    1000000000ll * 1001 / 30000,
  };

  auto most_often = m_duration_frequency.begin();
  for (auto current = m_duration_frequency.begin(); m_duration_frequency.end() != current; ++current)
    if (current->second > most_often->second)
      most_often = current;

  // No duration at all!? No frame?
  if (m_duration_frequency.end() == most_often) {
    mxdebug_if(m_debug_timecodes, boost::format("Duration frequency: none found, using 25 FPS\n"));
    return 1000000000ll / 25;
  }

  auto best = std::make_pair(most_often->first, std::numeric_limits<uint64_t>::max());

  for (auto common_default_duration : s_common_default_durations) {
    uint64_t diff = std::abs(most_often->first - common_default_duration);
    if ((diff < 20000) && (diff < best.second))
      best = std::make_pair(common_default_duration, diff);
  }

  mxdebug_if(m_debug_timecodes, boost::format("Duration frequency. Result: %1%, diff %2%. Best before adjustment: %3%. All: %4%\n")
             % best.first % best.second % most_often->first
             % boost::accumulate(m_duration_frequency, std::string(""), [](std::string const &accu, std::pair<int64_t, int64_t> const &pair) {
                 return accu + (boost::format(" <%1% %2%>") % pair.first % pair.second).str();
               }));

  return best.first;
}

void
es_parser_c::calculate_frame_order() {
  auto frames_begin           = m_frames.begin();
  auto frames_end             = m_frames.end();
  auto frame_itr              = frames_begin;

  auto &idr                   = frame_itr->m_si;
  auto &sps                   = m_sps_info_list[idr.sps];

  auto idx                    = 0u;
  auto prev_pic_order_cnt_msb = 0u;
  auto prev_pic_order_cnt_lsb = 0u;

  m_simple_picture_order      = false;

  while (frames_end != frame_itr) {
    auto &si = frame_itr->m_si;

    if (si.sps != idr.sps) {
      m_simple_picture_order = true;
      break;
    }

    if ((HEVC_NALU_TYPE_IDR_W_RADL == idr.type) || (HEVC_NALU_TYPE_IDR_N_LP == idr.type)) {
      frame_itr->m_presentation_order = 0;
      prev_pic_order_cnt_lsb = prev_pic_order_cnt_msb = 0;
    } else {
      unsigned int poc_msb;
      auto max_poc_lsb = 1u << (sps.log2_max_pic_order_cnt_lsb);
      auto poc_lsb     = si.pic_order_cnt_lsb;

      if (poc_lsb < prev_pic_order_cnt_lsb && (prev_pic_order_cnt_lsb - poc_lsb) >= (max_poc_lsb / 2))
        poc_msb = prev_pic_order_cnt_msb + max_poc_lsb;
      else if (poc_lsb > prev_pic_order_cnt_lsb && (poc_lsb - prev_pic_order_cnt_lsb) > (max_poc_lsb / 2))
        poc_msb = prev_pic_order_cnt_msb - max_poc_lsb;
      else
        poc_msb = prev_pic_order_cnt_msb;

      frame_itr->m_presentation_order = poc_lsb + poc_msb;

      if ((HEVC_NALU_TYPE_RADL_N != idr.type) && (HEVC_NALU_TYPE_RADL_R != idr.type) && (HEVC_NALU_TYPE_RASL_N != idr.type) && (HEVC_NALU_TYPE_RASL_R != idr.type)) {
        prev_pic_order_cnt_lsb = poc_lsb;
        prev_pic_order_cnt_msb = poc_msb;
      }
    }

    frame_itr->m_decode_order = idx;

    ++frame_itr;
    ++idx;
  }
}

std::vector<int64_t>
es_parser_c::calculate_provided_timestamps_to_use() {
  auto frame_idx                     = 0u;
  auto provided_timestamps_idx       = 0u;
  auto const num_frames              = m_frames.size();
  auto const num_provided_timestamps = m_provided_timestamps.size();

  std::vector<int64_t> provided_timestamps_to_use;
  provided_timestamps_to_use.reserve(num_frames);

  while (   (frame_idx               < num_frames)
         && (provided_timestamps_idx < num_provided_timestamps)) {
    timestamp_c timestamp_to_use;
    auto &frame = m_frames[frame_idx];

    while (   (provided_timestamps_idx < num_provided_timestamps)
           && (frame.m_position >= m_provided_timestamps[provided_timestamps_idx].second)) {
      timestamp_to_use = timestamp_c::ns(m_provided_timestamps[provided_timestamps_idx].first);
      ++provided_timestamps_idx;
    }

    if (timestamp_to_use.valid()) {
      provided_timestamps_to_use.emplace_back(timestamp_to_use.to_ns());
      frame.m_has_provided_timecode = true;
    }

    ++frame_idx;
  }

  mxdebug_if(m_debug_timecodes,
             boost::format("cleanup; num frames %1% num provided timestamps available %2% num provided timestamps to use %3%\n"
                           "  frames:\n%4%"
                           "  provided timestamps (available):\n%5%"
                           "  provided timestamps (to use):\n%6%")
             % num_frames % num_provided_timestamps % provided_timestamps_to_use.size()
             % boost::accumulate(m_frames, std::string{}, [](auto const &str, auto const &frame) {
                 return str + (boost::format("    pos %1% size %2% key? %3%\n") % frame.m_position % frame.m_data->get_size() % frame.m_keyframe).str();
               })
             % boost::accumulate(m_provided_timestamps, std::string{}, [](auto const &str, auto const &provided_timestamp) {
                 return str + (boost::format("    pos %1% timestamp %2%\n") % provided_timestamp.second % format_timestamp(provided_timestamp.first)).str();
               })
             % boost::accumulate(provided_timestamps_to_use, std::string{}, [](auto const &str, auto const &provided_timestamp) {
                 return str + (boost::format("    timestamp %1%\n") % format_timestamp(provided_timestamp)).str();
               }));

  m_provided_timestamps.erase(m_provided_timestamps.begin(), m_provided_timestamps.begin() + provided_timestamps_idx);

  brng::sort(provided_timestamps_to_use);

  return provided_timestamps_to_use;
}

void
es_parser_c::calculate_frame_timestamps() {
  auto provided_timestamps_to_use = calculate_provided_timestamps_to_use();

  if (!m_simple_picture_order)
    brng::sort(m_frames, [](const frame_t &f1, const frame_t &f2) { return f1.m_presentation_order < f2.m_presentation_order; });

  auto frames_begin          = m_frames.begin();
  auto frames_end            = m_frames.end();
  auto previous_frame_itr    = frames_begin;
  auto provided_timecode_itr = provided_timestamps_to_use.begin();

  for (auto frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    if (frame_itr->m_has_provided_timecode) {
      frame_itr->m_start = *provided_timecode_itr;
      ++provided_timecode_itr;

      if (frames_begin != frame_itr)
        previous_frame_itr->m_end = frame_itr->m_start;

    } else {
      frame_itr->m_start = frames_begin == frame_itr ? m_max_timecode : previous_frame_itr->m_end;
      ++m_stats.num_timecodes_generated;
    }

    frame_itr->m_end = frame_itr->m_start + duration_for(frame_itr->m_si);

    previous_frame_itr = frame_itr;
  }

  m_max_timecode = m_frames.back().m_end;

  mxdebug_if(m_debug_timecodes, boost::format("CLEANUP frames <pres_ord dec_ord has_prov_tc tc dur>: %1%\n")
             % boost::accumulate(m_frames, std::string(""), [](std::string const &accu, frame_t const &frame) {
                 return accu + (boost::format(" <%1% %2% %3% %4% %5%>") % frame.m_presentation_order % frame.m_decode_order % frame.m_has_provided_timecode % frame.m_start % (frame.m_end - frame.m_start)).str();
               }));

  if (!m_simple_picture_order)
    brng::sort(m_frames, [](const frame_t &f1, const frame_t &f2) { return f1.m_decode_order < f2.m_decode_order; });
}

void
es_parser_c::calculate_frame_references_and_update_stats() {
  auto frames_begin       = m_frames.begin();
  auto frames_end         = m_frames.end();
  auto previous_frame_itr = frames_begin;

  for (auto frame_itr = frames_begin; frames_end != frame_itr; ++frame_itr) {
    if (frames_begin != frame_itr)
      frame_itr->m_ref1 = previous_frame_itr->m_start - frame_itr->m_start;

    previous_frame_itr = frame_itr;
    m_duration_frequency[frame_itr->m_end - frame_itr->m_start]++;

    ++m_stats.num_field_slices;
  }
}

void
es_parser_c::cleanup() {
  if (m_frames.empty())
    return;

  if (m_discard_actual_frames) {
    m_stats.num_frames_discarded    += m_frames.size();
    m_stats.num_timecodes_discarded += m_provided_timestamps.size();

    m_frames.clear();
    m_provided_timestamps.clear();

    return;
  }

  calculate_frame_order();
  calculate_frame_timestamps();
  calculate_frame_references_and_update_stats();

  if (m_first_cleanup && !m_frames.front().m_keyframe) {
    // Drop all frames before the first key frames as they cannot be
    // decoded anyway.
    m_stats.num_frames_discarded += m_frames.size();
    m_frames.clear();

    return;
  }

  m_first_cleanup = false;

  m_stats.num_frames_out += m_frames.size();
  m_frames_out.insert(m_frames_out.end(), m_frames.begin(), m_frames.end());
  m_frames.clear();
}

memory_cptr
es_parser_c::create_nalu_with_size(const memory_cptr &src,
                                   bool add_extra_data) {
  auto nalu = mtx::mpeg::create_nalu_with_size(src, m_nalu_size_length, add_extra_data ? m_extra_data : std::vector<memory_cptr>{} );

  if (add_extra_data)
    m_extra_data.clear();

  return nalu;
}

memory_cptr
es_parser_c::get_hevcc()
  const {
  return hevcc_c{static_cast<unsigned int>(m_nalu_size_length), m_vps_list, m_sps_list, m_pps_list, m_user_data, m_codec_private}.pack();
}

bool
es_parser_c::has_par_been_found()
  const {
  assert(m_hevcc_ready);
  return m_par_found;
}

int64_rational_c const &
es_parser_c::get_par()
  const {
  assert(m_hevcc_ready && m_par_found);
  return m_par;
}

std::pair<int64_t, int64_t> const
es_parser_c::get_display_dimensions(int width,
                                    int height)
  const {
  assert(m_hevcc_ready && m_par_found);

  if (0 >= width)
    width = get_width();
  if (0 >= height)
    height = get_height();

  return std::make_pair<int64_t, int64_t>(1 <= m_par ? std::llround(width * boost::rational_cast<double>(m_par)) : width,
                                          1 <= m_par ? height                                                    : std::llround(height / boost::rational_cast<double>(m_par)));
}

size_t
es_parser_c::get_num_field_slices()
  const {
  return m_stats.num_field_slices;
}

size_t
es_parser_c::get_num_frame_slices()
  const {
  return m_stats.num_frame_slices;
}

void
es_parser_c::dump_info()
  const {
  mxinfo("Dumping m_frames_out:\n");
  for (auto &frame : m_frames_out) {
    mxinfo(boost::format("size %1% key %2% start %3% end %4% ref1 %5% adler32 0x%|6$08x|\n")
           % frame.m_data->get_size()
           % frame.m_keyframe
           % format_timestamp(frame.m_start)
           % format_timestamp(frame.m_end)
           % format_timestamp(frame.m_ref1)
           % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, *frame.m_data));
  }
}

std::string
es_parser_c::get_nalu_type_name(int type) {
  init_nalu_names();
  auto name = ms_nalu_names_by_type.find(type);
  return name == ms_nalu_names_by_type.end() ? "unknown" : name->second;
}

void
es_parser_c::init_nalu_names() {
  if (!ms_nalu_names_by_type.empty())
    return;

  ms_nalu_names_by_type = std::unordered_map<int, std::string>{
    { HEVC_NALU_TYPE_TRAIL_N,       "trail_n"       },
    { HEVC_NALU_TYPE_TRAIL_R,       "trail_r"       },
    { HEVC_NALU_TYPE_TSA_N,         "tsa_n"         },
    { HEVC_NALU_TYPE_TSA_R,         "tsa_r"         },
    { HEVC_NALU_TYPE_STSA_N,        "stsa_n"        },
    { HEVC_NALU_TYPE_STSA_R,        "stsa_r"        },
    { HEVC_NALU_TYPE_RADL_N,        "radl_n"        },
    { HEVC_NALU_TYPE_RADL_R,        "radl_r"        },
    { HEVC_NALU_TYPE_RASL_N,        "rasl_n"        },
    { HEVC_NALU_TYPE_RASL_R,        "rasl_r"        },
    { HEVC_NALU_TYPE_RSV_VCL_N10,   "rsv_vcl_n10"   },
    { HEVC_NALU_TYPE_RSV_VCL_N12,   "rsv_vcl_n12"   },
    { HEVC_NALU_TYPE_RSV_VCL_N14,   "rsv_vcl_n14"   },
    { HEVC_NALU_TYPE_RSV_VCL_R11,   "rsv_vcl_r11"   },
    { HEVC_NALU_TYPE_RSV_VCL_R13,   "rsv_vcl_r13"   },
    { HEVC_NALU_TYPE_RSV_VCL_R15,   "rsv_vcl_r15"   },
    { HEVC_NALU_TYPE_BLA_W_LP,      "bla_w_lp"      },
    { HEVC_NALU_TYPE_BLA_W_RADL,    "bla_w_radl"    },
    { HEVC_NALU_TYPE_BLA_N_LP,      "bla_n_lp"      },
    { HEVC_NALU_TYPE_IDR_W_RADL,    "idr_w_radl"    },
    { HEVC_NALU_TYPE_IDR_N_LP,      "idr_n_lp"      },
    { HEVC_NALU_TYPE_CRA_NUT,       "cra_nut"       },
    { HEVC_NALU_TYPE_RSV_RAP_VCL22, "rsv_rap_vcl22" },
    { HEVC_NALU_TYPE_RSV_RAP_VCL23, "rsv_rap_vcl23" },
    { HEVC_NALU_TYPE_RSV_VCL24,     "rsv_vcl24"     },
    { HEVC_NALU_TYPE_RSV_VCL25,     "rsv_vcl25"     },
    { HEVC_NALU_TYPE_RSV_VCL26,     "rsv_vcl26"     },
    { HEVC_NALU_TYPE_RSV_VCL27,     "rsv_vcl27"     },
    { HEVC_NALU_TYPE_RSV_VCL28,     "rsv_vcl28"     },
    { HEVC_NALU_TYPE_RSV_VCL29,     "rsv_vcl29"     },
    { HEVC_NALU_TYPE_RSV_VCL30,     "rsv_vcl30"     },
    { HEVC_NALU_TYPE_RSV_VCL31,     "rsv_vcl31"     },
    { HEVC_NALU_TYPE_VIDEO_PARAM,   "video_param"   },
    { HEVC_NALU_TYPE_SEQ_PARAM,     "seq_param"     },
    { HEVC_NALU_TYPE_PIC_PARAM,     "pic_param"     },
    { HEVC_NALU_TYPE_ACCESS_UNIT,   "access_unit"   },
    { HEVC_NALU_TYPE_END_OF_SEQ,    "end_of_seq"    },
    { HEVC_NALU_TYPE_END_OF_STREAM, "end_of_stream" },
    { HEVC_NALU_TYPE_FILLER_DATA,   "filler_data"   },
    { HEVC_NALU_TYPE_PREFIX_SEI,    "prefix_sei"    },
    { HEVC_NALU_TYPE_SUFFIX_SEI,    "suffix_sei"    },
    { HEVC_NALU_TYPE_RSV_NVCL41,    "rsv_nvcl41"    },
    { HEVC_NALU_TYPE_RSV_NVCL42,    "rsv_nvcl42"    },
    { HEVC_NALU_TYPE_RSV_NVCL43,    "rsv_nvcl43"    },
    { HEVC_NALU_TYPE_RSV_NVCL44,    "rsv_nvcl44"    },
    { HEVC_NALU_TYPE_RSV_NVCL45,    "rsv_nvcl45"    },
    { HEVC_NALU_TYPE_RSV_NVCL46,    "rsv_nvcl46"    },
    { HEVC_NALU_TYPE_RSV_NVCL47,    "rsv_nvcl47"    },
    { HEVC_NALU_TYPE_UNSPEC48,      "unspec48"      },
    { HEVC_NALU_TYPE_UNSPEC49,      "unspec49"      },
    { HEVC_NALU_TYPE_UNSPEC50,      "unspec50"      },
    { HEVC_NALU_TYPE_UNSPEC51,      "unspec51"      },
    { HEVC_NALU_TYPE_UNSPEC52,      "unspec52"      },
    { HEVC_NALU_TYPE_UNSPEC53,      "unspec53"      },
    { HEVC_NALU_TYPE_UNSPEC54,      "unspec54"      },
    { HEVC_NALU_TYPE_UNSPEC55,      "unspec55"      },
    { HEVC_NALU_TYPE_UNSPEC56,      "unspec56"      },
    { HEVC_NALU_TYPE_UNSPEC57,      "unspec57"      },
    { HEVC_NALU_TYPE_UNSPEC58,      "unspec58"      },
    { HEVC_NALU_TYPE_UNSPEC59,      "unspec59"      },
    { HEVC_NALU_TYPE_UNSPEC60,      "unspec60"      },
    { HEVC_NALU_TYPE_UNSPEC61,      "unspec61"      },
    { HEVC_NALU_TYPE_UNSPEC62,      "unspec62"      },
    { HEVC_NALU_TYPE_UNSPEC63,      "unspec63"      },
  };
}

}}                              // namespace mtx::hevc
