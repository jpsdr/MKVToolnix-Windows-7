/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   CoreAudio reader module

   Written by Moritz Bunkus <mo@bunkus.online>.
   Initial DTS support by Peter Niemayer <niemayer@isg.de> and
     modified by Moritz Bunkus.
*/

#include "common/common_pch.h"

#include "common/alac.h"
#include "common/endian.h"
#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/strings/formatting.h"
#include "input/r_coreaudio.h"
#include "merge/input_x.h"
#include "output/p_alac.h"

bool
coreaudio_reader_c::probe_file() {
  std::string magic;
  return (4 == m_in->read(magic, 4)) && (balg::to_lower_copy(magic) == "caff");
}

void
coreaudio_reader_c::identify() {
  if (!m_supported) {
    id_result_container_unsupported(m_in->get_file_name(), fmt::format("CoreAudio ({0})", m_codec_name));
    return;
  }

  auto info = mtx::id::info_c{};
  info.add(mtx::id::audio_channels,           m_channels);
  info.add(mtx::id::audio_sampling_frequency, static_cast<int64_t>(m_sample_rate));
  info.add(mtx::id::audio_bits_per_sample,    m_bits_per_sample);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, m_codec.get_name(m_codec_name), info.get());
}

void
coreaudio_reader_c::read_headers() {
  // Skip "caff" magic, version and flags
  m_in->setFilePointer(8);

  scan_chunks();

  parse_desc_chunk();
  parse_pakt_chunk();
  parse_kuki_chunk();

  if (m_debug_headers)
    dump_headers();
}

void
coreaudio_reader_c::dump_headers()
  const {
  mxdebug(fmt::format("File '{0}' CoreAudio header dump\n"
                      "  'desc' chunk:\n"
                      "    codec:             {1}\n"
                      "    supported:         {2}\n"
                      "    sample_rate:       {3}\n"
                      "    flags:             {4}\n"
                      "    bytes_per_packet:  {5}\n"
                      "    frames_per_packet: {6}\n"
                      "    channels:          {7}\n"
                      "    bits_per_sample:   {8}\n"
                      "  'kuki' chunk:\n"
                      "    magic cookie:      {9}\n",
                      m_ti.m_fname,
                      m_codec_name, m_supported, m_sample_rate, m_flags, m_bytes_per_packet, m_frames_per_packet, m_channels, m_bits_per_sample,
                      m_magic_cookie ? fmt::format("present, size {0}", m_magic_cookie->get_size()) : "not present"s));

  if (m_codec.is(codec_c::type_e::A_ALAC) && m_magic_cookie && (m_magic_cookie->get_size() >= sizeof(mtx::alac::codec_config_t))) {
    auto cfg = reinterpret_cast<mtx::alac::codec_config_t *>(m_magic_cookie->get_buffer());

    mxdebug(fmt::format("ALAC magic cookie dump:\n"
                        "  frame length:         {0}\n"
                        "  compatible version:   {1}\n"
                        "  bit depth:            {2}\n"
                        "  rice history mult:    {3}\n"
                        "  rice initial history: {4}\n"
                        "  rice limit:           {5}\n"
                        "  num channels:         {6}\n"
                        "  max run:              {7}\n"
                        "  max frame bytes:      {8}\n"
                        "  avg bit rate:         {9}\n"
                        "  sample rate:          {10}\n",
                        get_uint32_be(&cfg->frame_length),
                        static_cast<unsigned int>(cfg->compatible_version),
                        static_cast<unsigned int>(cfg->bit_depth),
                        static_cast<unsigned int>(cfg->rice_history_mult),
                        static_cast<unsigned int>(cfg->rice_initial_history),
                        static_cast<unsigned int>(cfg->rice_limit),
                        static_cast<unsigned int>(cfg->num_channels),
                        get_uint16_be(&cfg->max_run),
                        get_uint32_be(&cfg->max_frame_bytes),
                        get_uint32_be(&cfg->avg_bit_rate),
                        get_uint32_be(&cfg->sample_rate)));
  }
}

void
coreaudio_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || !m_reader_packetizers.empty() || ! m_supported)
    return;

  if (m_codec.is(codec_c::type_e::A_ALAC))
    add_packetizer(create_alac_packetizer());
  else
    assert(false);

  show_packetizer_info(0, ptzr(0));
}

generic_packetizer_c *
coreaudio_reader_c::create_alac_packetizer() {
  return new alac_packetizer_c(this, m_ti, m_magic_cookie, m_sample_rate, m_channels);
}

file_status_e
coreaudio_reader_c::read(generic_packetizer_c *,
                         bool) {
  if (!m_supported || (m_current_packet == m_packets.end()))
    return FILE_STATUS_DONE;

  try {
    m_in->setFilePointer(m_current_packet->m_position);
    auto mem = memory_c::alloc(m_current_packet->m_size);
    if (m_in->read(mem, m_current_packet->m_size) != m_current_packet->m_size)
      throw false;

    ptzr(0).process(std::make_shared<packet_t>(mem, m_current_packet->m_timestamp * m_frames_to_timestamp, m_current_packet->m_duration * m_frames_to_timestamp));

    ++m_current_packet;

  } catch (...) {
    m_current_packet = m_packets.end();
  }

  return m_current_packet == m_packets.end() ? FILE_STATUS_DONE : FILE_STATUS_MOREDATA;
}

void
coreaudio_reader_c::scan_chunks() {
  try {
    auto file_size = m_in->get_size();

    while (true) {
      coreaudio_chunk_t chunk;
      chunk.m_position = m_in->getFilePointer();

      if (m_in->read(chunk.m_type, 4) != 4)
        return;

      chunk.m_size          = m_in->read_uint64_be();
      chunk.m_size          = std::min<uint64_t>(chunk.m_size ? chunk.m_size : std::numeric_limits<uint64_t>::max(), file_size);
      chunk.m_data_position = m_in->getFilePointer();

      mxdebug_if(m_debug_chunks,fmt::format("scan_chunks() new chunk at {0} data position {3} type '{1}' size {2}\n", chunk.m_position, mtx::string::get_displayable(chunk.m_type), chunk.m_size, chunk.m_data_position));

      m_chunks.push_back(chunk);
      m_in->setFilePointer(chunk.m_size, libebml::seek_current);

    }
  } catch (...) {
  }
}

void
coreaudio_reader_c::debug_error_and_throw(std::string const &message)
  const {
  mxdebug_if(m_debug_headers, fmt::format("{0}\n", message));
  throw mtx::input::header_parsing_x{};
}

coreaudio_chunk_itr
coreaudio_reader_c::find_chunk(std::string const &type,
                               bool throw_on_error,
                               coreaudio_chunk_itr start) {
  auto chunk = std::find_if(start, m_chunks.end(), [&type](auto const &candidate) { return candidate.m_type == type; });

  if (throw_on_error && (chunk == m_chunks.end()))
    debug_error_and_throw(fmt::format("Chunk type '{0}' not found", type));

  return chunk;
}

memory_cptr
coreaudio_reader_c::read_chunk(std::string const &type,
                               bool throw_on_error) {
  try {
    auto chunk = find_chunk(type, throw_on_error);
    if (chunk == m_chunks.end())
      return nullptr;

    if (!chunk->m_size)
      debug_error_and_throw(fmt::format("Chunk '{0}' at {1} has zero size", type, chunk->m_position));

    m_in->setFilePointer(chunk->m_data_position);

    memory_cptr mem{memory_c::alloc(chunk->m_size)};
    if (chunk->m_size != m_in->read(mem, chunk->m_size))
      throw mtx::mm_io::end_of_file_x{};

    return mem;

  } catch (mtx::mm_io::exception &ex) {
    debug_error_and_throw(fmt::format("I/O error reading the '{0}' chunk: {1}", type, ex));

  } catch (mtx::input::header_parsing_x &) {
    throw;

  } catch (...) {
    debug_error_and_throw(fmt::format("Generic error reading the '{0}' chunk", type));
  }

  return nullptr;
}

void
coreaudio_reader_c::parse_desc_chunk() {
  auto mem = read_chunk("desc");
  mm_mem_io_c chunk{*mem};

  try {
    m_sample_rate = chunk.read_double();
    m_frames_to_timestamp.set(1000000000, m_sample_rate);

    if (4 != chunk.read(m_codec_name, 4)) {
      m_codec_name.clear();
      throw mtx::mm_io::end_of_file_x{};
    }

    balg::to_upper(m_codec_name);
    m_codec             = codec_c::look_up(m_codec_name);
    m_flags             = chunk.read_uint32_be();
    m_bytes_per_packet  = chunk.read_uint32_be();
    m_frames_per_packet = chunk.read_uint32_be();
    m_channels          = chunk.read_uint32_be();
    m_bits_per_sample   = chunk.read_uint32_be();

    if (m_codec.is(codec_c::type_e::A_ALAC))
      m_supported = true;

  } catch (mtx::mm_io::exception &ex) {
    debug_error_and_throw(fmt::format("I/O exception during 'desc' parsing: {0}", ex.what()));
  } catch (...) {
    debug_error_and_throw(fmt::format("Unknown exception during 'desc' parsing"));
  }
}

void
coreaudio_reader_c::parse_pakt_chunk() {
  try {
    auto chunk = read_chunk("pakt");
    mm_mem_io_c mem{*chunk};

    auto num_packets  = mem.read_uint64_be();
    auto num_frames   = mem.read_uint64_be()  // valid frames
                      + mem.read_uint32_be()  // priming frames
                      + mem.read_uint32_be(); // remainder frames
    auto data_chunk   = find_chunk("data");
    auto position     = data_chunk->m_data_position + 4; // skip the "edit count" field
    uint64_t timestamp = 0;

    mxdebug_if(m_debug_headers, fmt::format("Number of packets: {0}, number of frames: {1}\n", num_packets, num_frames));

    for (auto packet_num = 0u; packet_num < num_packets; ++packet_num) {
      coreaudio_packet_t packet;

      packet.m_position  = position;
      packet.m_size      = m_bytes_per_packet  ? m_bytes_per_packet  : mem.read_mp4_descriptor_len();
      packet.m_duration  = m_frames_per_packet ? m_frames_per_packet : mem.read_mp4_descriptor_len();
      packet.m_timestamp  = timestamp;
      position          += packet.m_size;
      timestamp          += packet.m_duration;

      m_packets.push_back(packet);

      mxdebug_if(m_debug_packets,
                 fmt::format("  {3}) position {0} size {1} raw duration {2} raw timestamp {5} duration {6} timestamp {7} position in 'pakt' {4}\n",
                             packet.m_position, packet.m_size, packet.m_duration, packet_num, mem.getFilePointer(), packet.m_timestamp,
                             mtx::string::format_timestamp(packet.m_duration * m_frames_to_timestamp), mtx::string::format_timestamp(packet.m_timestamp * m_frames_to_timestamp)));
    }

    mxdebug_if(m_debug_headers, fmt::format("Final value for 'position': {0} data chunk end: {1}\n", position, data_chunk->m_position + 12 + data_chunk->m_size));

  } catch (mtx::mm_io::exception &ex) {
    debug_error_and_throw(fmt::format("I/O exception during 'pakt' parsing: {0}", ex.what()));
  } catch (...) {
    debug_error_and_throw(fmt::format("Unknown exception during 'pakt' parsing"));
  }

  m_current_packet = m_packets.begin();
}

void
coreaudio_reader_c::parse_kuki_chunk() {
  if (!m_supported)
    return;

  auto chunk = read_chunk("kuki", false);
  if (!chunk)
    return;

  if (m_codec.is(codec_c::type_e::A_ALAC))
    handle_alac_magic_cookie(chunk);
}

void
coreaudio_reader_c::handle_alac_magic_cookie(memory_cptr const &chunk) {
  if (chunk->get_size() < sizeof(mtx::alac::codec_config_t))
    debug_error_and_throw(fmt::format("Invalid ALAC magic cookie; size: {0} < {1}", chunk->get_size(), sizeof(mtx::alac::codec_config_t)));

  // Convert old-style magic cookie
  if (!memcmp(chunk->get_buffer() + 4, "frmaalac", 8)) {
    mxdebug_if(m_debug_headers, fmt::format("Converting old-style ALAC magic cookie with size {0} to new style\n", chunk->get_size()));

    auto const old_style_min_size
      = 12                            // format atom
      + 12                            // ALAC specific info (size, ID, version, flags)
      + sizeof(mtx::alac::codec_config_t); // the magic cookie itself

    if (chunk->get_size() < old_style_min_size)
      debug_error_and_throw(fmt::format("Invalid old-style ALAC magic cookie; size: {0} < {1}", chunk->get_size(), old_style_min_size));

    m_magic_cookie = memory_c::clone(chunk->get_buffer() + 12 + 12, sizeof(mtx::alac::codec_config_t));

  } else
    m_magic_cookie = chunk;


  mxdebug_if(m_debug_headers, fmt::format("Magic cookie size: {0}\n", m_magic_cookie->get_size()));
}
