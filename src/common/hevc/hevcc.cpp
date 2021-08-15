/** MPEG-h HEVCC helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bit_writer.h"
#include "common/mm_mem_io.h"
#include "common/mpeg.h"
#include "common/hevc/util.h"
#include "common/hevc/hevcc.h"

namespace mtx::hevc {

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
                 std::vector<memory_cptr> vps_list,
                 std::vector<memory_cptr> sps_list,
                 std::vector<memory_cptr> pps_list,
                 user_data_t user_data,
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
  , m_vps_list{std::move(vps_list)}
  , m_sps_list{std::move(sps_list)}
  , m_pps_list{std::move(pps_list)}
  , m_user_data{std::move(user_data)}
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

  mtx::bits::writer_c w{};

  w.put_bits(1, 0); // forbidden_zero_bit
  w.put_bits(6, NALU_TYPE_PREFIX_SEI); // nal_unit_type
  w.put_bits(6, 0); // nuh_reserved_zero_6bits
  w.put_bits(3, 1); // nuh_temporal_id_plus1

  for (iter = m_user_data.begin(); iter != m_user_data.end(); ++iter) {
    int payload_size = iter->second.size();
    w.put_bits(8, SEI_USER_DATA_UNREGISTERED);
    while (payload_size >= 255) {
      w.put_bits(8, 255);
      payload_size -= 255;
    }
    w.put_bits(8, payload_size);

    auto data    = &iter->second[0];
    payload_size = iter->second.size();
    for (int idx = 0; idx < payload_size; ++idx)
      w.put_bits(8, data[idx]);
  }

  w.put_bit(true);
  w.byte_align();

  m_sei_list.push_back(mpeg::rbsp_to_nalu(w.get_buffer()));

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

  mtx::bits::writer_c w;

  auto write_list = [&w](std::vector<memory_cptr> const &list, uint8_t nal_unit_type) {
    w.put_bits(1, 1);
    w.put_bits(1, 0);
    w.put_bits(6, nal_unit_type);
    w.put_bits(16, list.size());

    for (auto &mem : list) {
      auto num_bytes = mem->get_size();
      mtx::bits::reader_c r{mem->get_buffer(), num_bytes};

      w.put_bits(16, num_bytes);

      while (num_bytes--)
        w.copy_bits(8, r);
    }
  };

  // configuration version
  w.put_bits(8, 1);
  // general parameters block
  // general_profile_space               2     Specifies the context for the interpretation of general_profile_idc and
  //                                           general_profile_compatibility_flag
  // general_tier_flag                   1     Specifies the context for the interpretation of general_level_idc
  // general_profile_idc                 5     Defines the profile of the bitstream
  w.put_bits(2, m_codec_private.profile_space);
  w.put_bits(1, m_codec_private.tier_flag);
  w.put_bits(5, m_codec_private.profile_idc);
  // general_profile_compatibility_flag  32    Defines profile compatibility
  w.put_bits(32, m_codec_private.profile_compatibility_flag);
  // general_progressive_source_flag     1     Source is progressive, see [2] for interpretation.
  // general_interlace_source_flag       1     Source is interlaced, see [2] for interpretation.
  // general_non-packed_constraint_flag  1     If 1 then no frame packing arrangement SEI messages, see [2] for more information
  // general_frame_only_constraint_flag  1     If 1 then no fields, see [2] for interpretation
  // reserved                            44    Reserved field, value TBD 0
  w.put_bits(1, m_codec_private.progressive_source_flag);
  w.put_bits(1, m_codec_private.interlaced_source_flag);
  w.put_bits(1, m_codec_private.non_packed_constraint_flag);
  w.put_bits(1, m_codec_private.frame_only_constraint_flag);
  w.put_bits(44, 0);
  // general_level_idc                   8     Defines the level of the bitstream
  w.put_bits(8, m_codec_private.level_idc & 0xFF);
  // reserved                            4     Reserved Field, value '1111'b
  // min_spatial_segmentation_idc        12    Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
  w.put_bits(4,  0x0f);
  w.put_bits(12, m_codec_private.min_spatial_segmentation_idc);
  // reserved                            6     Reserved Field, value '111111'b
  // parallelism_type                    2     0=unknown, 1=slices, 2=tiles, 3=WPP
  w.put_bits(6, 0x3f);
  w.put_bits(2, m_codec_private.parallelism_type);
  // reserved                            6     Reserved field, value '111111'b
  // chroma_format_idc                   2     See table 6-1, HEVC
  w.put_bits(6, 0x3f);
  w.put_bits(2, m_codec_private.chroma_format_idc);
  // reserved                            5     Reserved Field, value '11111'b
  // bit_depth_luma_minus8               3     Bit depth luma minus 8
  w.put_bits(5, 0x1f);
  w.put_bits(3, m_codec_private.bit_depth_luma_minus8);
  // reserved                            5     Reserved Field, value '11111'b
  // bit_depth_chroma_minus8             3     Bit depth chroma minus 8
  w.put_bits(5, 0x1f);
  w.put_bits(3,m_codec_private.bit_depth_chroma_minus8);
  // reserved                            16    Reserved Field, value 0
  w.put_bits(16, 0);
  // reserved                            2     Reserved Field, value 0
  // max_sub_layers                      3     maximum number of temporal sub-layers
  // temporal_id_nesting_flag            1     Specifies whether inter prediction is additionally restricted. see [2] for interpretation.
  // size_nalu_minus_one                 2     Size of field NALU Length – 1
  w.put_bits(2, 0);
  w.put_bits(3, m_codec_private.max_sub_layers_minus1 + 1);
  w.put_bits(1, m_codec_private.temporal_id_nesting_flag);
  w.put_bits(2, m_nalu_size_length - 1);
  // num_arrays                          8     Number of arrays of parameter sets
  auto num_arrays = (m_vps_list.empty() ? 0 : 1)
                  + (m_sps_list.empty() ? 0 : 1)
                  + (m_pps_list.empty() ? 0 : 1)
                  + (m_sei_list.empty() ? 0 : 1);
  w.put_bits(8, num_arrays);

  if (m_vps_list.size())
    write_list(m_vps_list, NALU_TYPE_VIDEO_PARAM);
  if (m_sps_list.size())
    write_list(m_sps_list, NALU_TYPE_SEQ_PARAM);
  if (m_pps_list.size())
    write_list(m_pps_list, NALU_TYPE_PIC_PARAM);
  if (m_sei_list.size())
    write_list(m_sei_list, NALU_TYPE_PREFIX_SEI);

  return w.get_buffer();
}

hevcc_c
hevcc_c::unpack(memory_cptr const &mem) {
  hevcc_c hevcc;

  if (!mem)
    return hevcc;

  try {
    mtx::bits::reader_c bit_reader(mem->get_buffer(), mem->get_size());
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

    while (num_arrays > 0) {
      auto type           = byte_reader.read_uint8() & 0x3F;
      auto nal_unit_count = byte_reader.read_uint16_be();
      auto &list          = type == NALU_TYPE_VIDEO_PARAM ? hevcc.m_vps_list
                          : type == NALU_TYPE_SEQ_PARAM   ? hevcc.m_sps_list
                          : type == NALU_TYPE_PIC_PARAM   ? hevcc.m_pps_list
                          :                                 hevcc.m_sei_list;

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

}                              // namespace mtx::hevc
