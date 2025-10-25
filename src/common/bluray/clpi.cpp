/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  Blu-ray clip info data handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bluray/clpi.h"
#include "common/mm_file_io.h"
#include "common/strings/formatting.h"

namespace mtx::bluray::clpi {

program_t::program_t()
  : spn_program_sequence_start(0)
  , program_map_pid(0)
  , num_streams(0)
  , num_groups(0)
{
}

void
program_t::dump() {
  mxinfo(fmt::format("Program dump:\n"
                     "  spn_program_sequence_start: {0}\n"
                     "  program_map_pid:            {1}\n"
                     "  num_streams:                {2}\n"
                     "  num_groups:                 {3}\n",
                     spn_program_sequence_start,program_map_pid, static_cast<unsigned int>(num_streams), static_cast<unsigned int>(num_groups)));

  for (auto &program_stream : program_streams)
    program_stream->dump();
}

program_stream_t::program_stream_t()
  : pid(0)
  , coding_type(0)
  , format(0)
  , rate(0)
  , aspect(0)
  , oc_flag(0)
  , char_code(0)
{
}

void
program_stream_t::dump() {
  mxinfo(fmt::format("Program stream dump:\n"
                     "  pid:         {0}\n"
                     "  coding_type: {1}\n"
                     "  format:      {2}\n"
                     "  rate:        {3}\n"
                     "  aspect:      {4}\n"
                     "  oc_flag:     {5}\n"
                     "  char_code:   {6}\n"
                     "  language:    {7}\n",
                     pid,
                     static_cast<unsigned int>(coding_type),
                     static_cast<unsigned int>(format),
                     static_cast<unsigned int>(rate),
                     static_cast<unsigned int>(aspect),
                     static_cast<unsigned int>(oc_flag),
                     static_cast<unsigned int>(char_code),
                     language));
}

void
ep_map_one_stream_t::dump(std::size_t idx)
  const {
  mxinfo(fmt::format("  EP map for one stream[{0}]:\n"
                     "    PID:               {1}\n"
                     "    type:              {2}\n"
                     "    num_coarse_points: {3}\n"
                     "    num_fine_points:   {4}\n"
                     "    address:           {5}\n",
                     idx, pid, type, num_coarse_points, num_fine_points, address));

  idx = 0;
  for (auto const &coarse_point : coarse_points)
    mxinfo(fmt::format("    coarse point[{0}]:\n"
                       "      ref_to_fine_id: {1}\n"
                       "      PTS:            {2}\n"
                       "      SPN:            {3}\n",
                       idx++, coarse_point.ref_to_fine_id, coarse_point.pts, coarse_point.spn));

  idx = 0;
  for (auto const &fine_point : fine_points)
    mxinfo(fmt::format("    fine point[{0}]:\n"
                       "      PTS: {1}\n"
                       "      SPN: {2}\n",
                       idx++, fine_point.pts, fine_point.spn));

  idx = 0;
  for (auto const &point : points)
    mxinfo(fmt::format("    calculated point[{0}]:\n"
                       "      PTS: {1}\n"
                       "      SPN: {2}\n",
                       idx++, point.pts, point.spn));
}

parser_c::parser_c(std::string file_name)
  : m_file_name{std::move(file_name)}
{
}

void
parser_c::dump() {
  mxinfo(fmt::format("Parser dump:\n"
                     "  sequence_info_start:             {0}\n"
                     "  program_info_start:              {1}\n"
                     "  characteristic_point_info_start: {2}\n",
                     m_sequence_info_start, m_program_info_start, m_characteristic_point_info_start));

  for (auto &program : m_programs)
    program->dump();

  mxinfo(fmt::format("Characteristic point info dump:\n"));

  auto idx = 0u;
  for (auto const &ep_map : m_ep_map)
    ep_map.dump(idx++);
}

bool
parser_c::parse() {
  try {
    mm_file_io_c m_file(m_file_name, libebml::MODE_READ);

    int64_t file_size   = m_file.get_size();
    memory_cptr content = memory_c::alloc(file_size);

    if (file_size != m_file.read(content, file_size))
      throw false;
    m_file.close();

    mtx::bits::reader_cptr bc(new mtx::bits::reader_c(content->get_buffer(), file_size));

    parse_header(*bc);
    parse_program_info(*bc);
    parse_characteristic_point_info(*bc);

    if (m_debug)
      dump();

    m_ok = true;

  } catch (...) {
    mxdebug_if(m_debug, "Parsing NOT OK\n");
  }

  return m_ok;
}

void
parser_c::parse_header(mtx::bits::reader_c &bc) {
  bc.set_bit_position(0);

  uint32_t magic = bc.get_bits(32);
  mxdebug_if(m_debug, fmt::format("File magic 1: 0x{0:08x}\n", magic));
  if (FILE_MAGIC != magic)
    throw false;

  magic = bc.get_bits(32);
  mxdebug_if(m_debug, fmt::format("File magic 2: 0x{0:08x}\n", magic));
  if ((FILE_MAGIC2A != magic) && (FILE_MAGIC2B != magic) && (FILE_MAGIC2C != magic))
    throw false;

  m_sequence_info_start             = bc.get_bits(32);
  m_program_info_start              = bc.get_bits(32);
  m_characteristic_point_info_start = bc.get_bits(32);
}

void
parser_c::parse_program_info(mtx::bits::reader_c &bc) {
  bc.set_bit_position(m_program_info_start * 8);

  bc.skip_bits(40);            // 32 bits length, 8 bits reserved
  size_t num_program_streams = bc.get_bits(8), program_idx, stream_idx;

  mxdebug_if(m_debug, fmt::format("num_program_streams: {0}\n", num_program_streams));

  for (program_idx = 0; program_idx < num_program_streams; ++program_idx) {
    program_cptr program(new program_t);
    m_programs.push_back(program);

    program->spn_program_sequence_start = bc.get_bits(32);
    program->program_map_pid            = bc.get_bits(16);
    program->num_streams                = bc.get_bits(8);
    program->num_groups                 = bc.get_bits(8);

    for (stream_idx = 0; stream_idx < program->num_streams; ++stream_idx)
      parse_program_stream(bc, *program);
  }
}

void
parser_c::parse_program_stream(mtx::bits::reader_c &bc,
                               program_t &program) {
  program_stream_cptr stream(new program_stream_t);
  program.program_streams.push_back(stream);

  stream->pid = bc.get_bits(16);

  if ((bc.get_bit_position() % 8) != 0) {
    mxdebug_if(m_debug, "Bit position not divisible by 8\n");
    throw false;
  }

  size_t length_in_bytes  = bc.get_bits(8);
  size_t position_in_bits = bc.get_bit_position();

  stream->coding_type     = bc.get_bits(8);

  char language[4];
  memset(language, 0, 4);

  switch (stream->coding_type) {
    case 0x01:
    case 0x02:
    case 0xea:
    case 0x1b:
      stream->format = bc.get_bits(4);
      stream->rate   = bc.get_bits(4);
      stream->aspect = bc.get_bits(4);
      bc.skip_bits(2);
      stream->oc_flag = bc.get_bits(1);
      bc.skip_bits(1);
      break;

    case 0x03:
    case 0x04:
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0xa1:
    case 0xa2:
      stream->format = bc.get_bits(4);
      stream->rate   = bc.get_bits(4);
      bc.get_bytes(reinterpret_cast<uint8_t *>(language), 3);
      break;

    case 0x90:
    case 0x91:
    case 0xa0:
      bc.get_bytes(reinterpret_cast<uint8_t *>(language), 3);
      break;

    case 0x92:
      stream->char_code = bc.get_bits(8);
      bc.get_bytes(reinterpret_cast<uint8_t *>(language), 3);
      break;

    default:
      mxdebug_if(m_debug, fmt::format("mtx::bluray::clpi::parser_c::parse_program_stream: Unknown coding type {0}\n", static_cast<unsigned int>(stream->coding_type)));
      break;
  }

  stream->language = mtx::bcp47::language_c::parse(language);

  bc.set_bit_position(position_in_bits + length_in_bytes * 8);
}

void
parser_c::parse_characteristic_point_info(mtx::bits::reader_c &bc) {
  bc.set_bit_position(m_characteristic_point_info_start * 8);

  auto length = bc.get_bits(32);
  if (length < 2)
    return;

  auto cpi_type = bc.skip_get_bits(12, 4);
  if (cpi_type == 1)            // 1 == EP_map
    parse_ep_map(bc);
}

void
parser_c::parse_ep_map(mtx::bits::reader_c &bc) {
  auto ep_map_start = bc.get_bit_position();

  bc.skip_bits(8);              // reserved for word align

  auto num_stream_pid_entries = bc.get_bits(8);
  for (auto idx = 0u; idx < num_stream_pid_entries; ++idx) {
    m_ep_map.emplace_back();
    auto &entry             = m_ep_map.back();

    entry.pid               = bc.get_bits(16);
    entry.type              = bc.skip_get_bits(10, 4); // reserved for word align
    entry.num_coarse_points = bc.get_bits(16);
    entry.num_fine_points   = bc.get_bits(18);
    entry.address           = bc.get_bits(32);
  }

  for (auto &entry : m_ep_map) {
    bc.set_bit_position(ep_map_start + entry.address * 8);
    parse_ep_map_for_one_stream_pid(bc, entry);
  }
}

void
parser_c::parse_ep_map_for_one_stream_pid(mtx::bits::reader_c &bc,
                                          ep_map_one_stream_t &map) {
  if (!map.num_coarse_points)
    return;

  auto start            = bc.get_bit_position();
  auto fine_table_start = bc.get_bits(32);

  for (auto idx = 0u; idx < map.num_coarse_points; ++idx) {
    map.coarse_points.emplace_back();
    auto &coarse_point          = map.coarse_points.back();

    coarse_point.ref_to_fine_id = bc.get_bits(18);
    coarse_point.pts            = bc.get_bits(14);
    coarse_point.spn            = bc.get_bits(32);
  }

  bc.set_bit_position(start + fine_table_start * 8);

  auto current_coarse_point = map.coarse_points.begin(),
    next_coarse_point       = current_coarse_point + 1,
    coarse_end              = map.coarse_points.end();

  for (auto idx = 0u; idx < map.num_fine_points; ++idx) {
    bc.skip_bits(1 + 3);        // is_angle_change_point, l_end_position_offset

    map.fine_points.emplace_back();
    auto &fine_point = map.fine_points.back();

    fine_point.pts   = bc.get_bits(11);
    fine_point.spn   = bc.get_bits(17);

    if (   (next_coarse_point < coarse_end)
        && (idx               >= next_coarse_point->ref_to_fine_id)) {
      ++current_coarse_point;
      ++next_coarse_point;
    }

    map.points.emplace_back();
    auto &point = map.points.back();

    point.pts = timestamp_c::mpeg(((current_coarse_point->pts & ~0x01) << 19) + (fine_point.pts << 9));
    point.spn = (current_coarse_point->spn & ~0x1ffff) + fine_point.spn;
  }
}

}
