/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AV1 parser code

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/av1.h"
#include "common/bit_reader.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/list_utils.h"

namespace mtx { namespace av1 {

class parser_private_c {
  friend class parser_c;

  mtx::bits::reader_c r;

  bool sequence_header_found{};

  unsigned int obu_type{};
  bool obu_extension_flag{}, obu_has_size_field{};
  boost::optional<unsigned int> temporal_id, spatial_id;
  unsigned int operating_point_idc{};

  debugging_option_c debug_is_keyframe{"av1|av1_is_keyframe"};
};

parser_c::parser_c()
  : p{std::make_unique<parser_private_c>()}
{
}

parser_c::~parser_c() = default;

char const *
parser_c::get_obu_type_name(unsigned int obu_type) {
  static std::unordered_map<unsigned int, char const *> s_type_names;

  if (s_type_names.empty()) {
    s_type_names[OBU_SEQUENCE_HEADER]        = "sequence_header";
    s_type_names[OBU_TD]                     = "td";
    s_type_names[OBU_FRAME_HEADER]           = "frame_header";
    s_type_names[OBU_TITLE_GROUP]            = "title_group";
    s_type_names[OBU_METADATA]               = "metadata";
    s_type_names[OBU_FRAME]                  = "frame";
    s_type_names[OBU_REDUNDANT_FRAME_HEADER] = "redundant_frame_header";
    s_type_names[OBU_PADDING]                = "padding";
  }

  auto itr = s_type_names.find(obu_type);
  return itr != s_type_names.end() ? itr->second : "unknown";
}

uint64_t
parser_c::read_leb128() {
  uint64_t value{};

  for (int idx = 0; idx < 8; ++idx) {
    auto byte  = p->r.get_bits(8);
    value     |= (byte & 0x7f) << (idx * 7);

    if ((byte & 0x80) == 0)
      break;
  }

  return value;
}

boost::optional<uint64_t>
parser_c::parse_obu_common_data(memory_c const &buffer) {
  return parse_obu_common_data(buffer.get_buffer(), buffer.get_size());
}

boost::optional<uint64_t>
parser_c::parse_obu_common_data(unsigned char const *buffer,
                                uint64_t buffer_size) {
  p->r.init(buffer, buffer_size);
  return parse_obu_common_data();
}

boost::optional<uint64_t>
parser_c::parse_obu_common_data() {
  auto &r = p->r;

  // obu_header
  if (r.get_bit())              // obu_forbidden_bit
    throw obu_invalid_structure_x{};

  p->obu_type           = r.get_bits(4);
  p->obu_extension_flag = r.get_bit();
  p->obu_has_size_field = r.get_bit();
  r.skip_bits(1);               // obu_reserved_bit

  if (p->obu_extension_flag) {
    // obu_extension_header
    p->temporal_id = r.get_bits(3);
    p->spatial_id  = r.get_bits(2);
    r.skip_bits(3);             // extension_header_reserved_3bits

  } else {
    p->temporal_id = boost::none;
    p->spatial_id  = boost::none;
  }

  if (p->obu_has_size_field)
    return read_leb128();

  return {};
}

bool
parser_c::parse_sequence_header_obu() {
  return false;
}

bool
parser_c::parse_obu(unsigned char const *buffer,
                    uint64_t buffer_size,
                    uint64_t unit_size) {
  auto &r = p->r;

  r.init(buffer, buffer_size);

  auto obu_size = parse_obu_common_data();

  if (!obu_size && !unit_size)
    throw obu_without_size_unsupported_x{};

  if (!obu_size)
    obu_size = unit_size - 1 - (p->obu_extension_flag ? 1 : 0);

  if (   (p->obu_type            != OBU_SEQUENCE_HEADER)
      && (p->operating_point_idc != 0)
      && p->obu_extension_flag) {
    auto in_temporal_layer = !!((p->operating_point_idc >> *p->temporal_id)      & 1);
    auto in_spatial_layer  = !!((p->operating_point_idc >> (*p->spatial_id + 8)) & 1);

    if (!in_temporal_layer || !in_spatial_layer)
      return false;
  }

  if (p->obu_type == OBU_PADDING)
    return true;

  if (p->obu_type == OBU_SEQUENCE_HEADER) {
    auto result = parse_sequence_header_obu();
    if (result)
      p->sequence_header_found = true;
    return result;
  }

  return false;
}

bool
parser_c::is_keyframe(memory_c const &buffer) {
  return is_keyframe(buffer.get_buffer(), buffer.get_size());
}

bool
parser_c::is_keyframe(unsigned char const *buffer,
                      uint64_t buffer_size) {
  mxdebug_if(p->debug_is_keyframe, boost::format("is_keyframe: start on size %1%\n") % buffer_size);

  p->r.init(buffer, buffer_size);

  try {
    while (true) {
      auto position = p->r.get_bit_position() / 8;
      auto obu_size = parse_obu_common_data();

      mxdebug_if(p->debug_is_keyframe,
                 boost::format("is_keyframe:   at %1% type %2% (%3%) size %4%\n")
                 % position
                 % p->obu_type
                 % get_obu_type_name(p->obu_type)
                 % (obu_size ? static_cast<int>(*obu_size) : -1));

      if (!mtx::included_in(p->obu_type, OBU_FRAME, OBU_FRAME_HEADER)) {
        if (!obu_size) {
          mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to OBU with unknown size\n");
          return false;
        }

        p->r.skip_bits(*obu_size * 8);

        continue;
      }

      if (p->r.get_bit()) {     // show_existing_frame
        mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to show_existing_frame == 1\n");
        return false;
      }

      auto frame_type = p->r.get_bits(2);
      auto result     = (frame_type == FRAME_TYPE_KEY); // || (frame_type == FRAME_TYPE_INTRA_ONLY);

      mxdebug_if(p->debug_is_keyframe,
                 boost::format("is_keyframe:   %1% due to frame_type == %2% (%3%)\n")
                 % (result ? "true" : "false")
                 % frame_type
                 % (  frame_type == FRAME_TYPE_KEY        ? "key"
                    : frame_type == FRAME_TYPE_INTER      ? "inter"
                    : frame_type == FRAME_TYPE_INTRA_ONLY ? "intra-only"
                    :                                       "switch"));

      return result;
    }

  } catch (mtx::mm_io::exception &) {
    mxdebug_if(p->debug_is_keyframe, "is_keyframe:   false due to I/O exception\n");
  }

  return false;
}

}}
