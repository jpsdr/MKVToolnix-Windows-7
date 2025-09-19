/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Quicktime and MP4 reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
   The second half of the parse_headers() function after the
     "// process chunkmap:" comment was taken from mplayer's
     demux_mov.c file which is distributed under the GPL as well. Thanks to
     the original authors.
*/

#include "common/common_pch.h"

#include <cstring>
#include <unordered_map>

#include <boost/multiprecision/gmp.hpp>

#include <QRegularExpression>

#include "avilib.h"
#include "common/aac.h"
#include "common/alac.h"
#include "common/at_scope_exit.h"
#include "common/avc/es_parser.h"
#include "common/chapters/chapters.h"
#include "common/codec.h"
#include "common/construct.h"
#include "common/endian.h"
#include "common/frame_timing.h"
#include "common/hacks.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mm_mem_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/mp3.h"
#include "common/mp4.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/tags/tags.h"
#include "common/vobsub.h"
#include "input/r_qtmp4.h"
#include "input/timed_text_to_text_utf8_converter.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_alac.h"
#include "output/p_av1.h"
#include "output/p_avc.h"
#include "output/p_dts.h"
#include "output/p_flac.h"
#include "output/p_hevc.h"
#include "output/p_hevc_es.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_mpeg4_p2.h"
#include "output/p_opus.h"
#include "output/p_passthrough.h"
#include "output/p_pcm.h"
#include "output/p_prores.h"
#include "output/p_quicktime.h"
#include "output/p_textsubs.h"
#include "output/p_video_for_windows.h"
#include "output/p_vobsub.h"
#include "output/p_vorbis.h"
#include "output/p_vpx.h"

constexpr auto MAX_INTERLEAVING_BADNESS = 0.4;

namespace mtx {

class atom_chunk_size_x: public exception {
private:
  uint64_t m_size, m_pos;

public:
  atom_chunk_size_x(uint64_t size, uint64_t pos)
    : mtx::exception()
    , m_size{size}
    , m_pos{pos}
  {
  }

  virtual const char *what() const throw() {
    return "invalid atom chunk size";
  }

  virtual std::string error() const throw() {
    return fmt::format("invalid atom chunk size {0} at {1}", m_size, m_pos);
  }
};

}

static void
print_atom_too_small_error(std::string const &name,
                           qt_atom_t const &atom,
                           std::size_t actual_size) {
  mxerror(fmt::format(FY("Quicktime/MP4 reader: '{0}' atom is too small. Expected size: >= {1}. Actual size: {2}.\n"), name, actual_size, atom.size));
}

static std::string
space(int num) {
  return std::string(num, ' ');
}

static qt_atom_t
read_qtmp4_atom(mm_io_c *read_from,
                bool exit_on_error = true) {
  qt_atom_t a;

  a.pos    = read_from->getFilePointer();
  a.size   = read_from->read_uint32_be();
  a.fourcc = fourcc_c(read_from);
  a.hsize  = 8;

  if (1 == a.size) {
    a.size   = read_from->read_uint64_be();
    a.hsize += 8;

  } else if (0 == a.size)
    a.size   = read_from->get_size() - read_from->getFilePointer() + 8;

  if (a.size < a.hsize) {
    if (exit_on_error)
      mxerror(fmt::format(FY("Quicktime/MP4 reader: Invalid chunk size {0} at {1}.\n"), a.size, a.pos));
    else
      throw mtx::atom_chunk_size_x{a.size, a.pos};
  }

  return a;
}

static std::string
displayable_esds_tag_name(uint8_t tag) {
  return mtx::mp4::DESCRIPTOR_TYPE_O                 == tag ? "O"
       : mtx::mp4::DESCRIPTOR_TYPE_IO                == tag ? "IO"
       : mtx::mp4::DESCRIPTOR_TYPE_ES                == tag ? "ES"
       : mtx::mp4::DESCRIPTOR_TYPE_DEC_CONFIG        == tag ? "DEC_CONFIG"
       : mtx::mp4::DESCRIPTOR_TYPE_DEC_SPECIFIC      == tag ? "DEC_SPECIFIC"
       : mtx::mp4::DESCRIPTOR_TYPE_SL_CONFIG         == tag ? "SL_CONFIG"
       : mtx::mp4::DESCRIPTOR_TYPE_CONTENT_ID        == tag ? "CONTENT_ID"
       : mtx::mp4::DESCRIPTOR_TYPE_SUPPL_CONTENT_ID  == tag ? "SUPPL_CONTENT_ID"
       : mtx::mp4::DESCRIPTOR_TYPE_IP_PTR            == tag ? "IP_PTR"
       : mtx::mp4::DESCRIPTOR_TYPE_IPMP_PTR          == tag ? "IPMP_PTR"
       : mtx::mp4::DESCRIPTOR_TYPE_IPMP              == tag ? "IPMP"
       : mtx::mp4::DESCRIPTOR_TYPE_REGISTRATION      == tag ? "REGISTRATION"
       : mtx::mp4::DESCRIPTOR_TYPE_ESID_INC          == tag ? "ESID_INC"
       : mtx::mp4::DESCRIPTOR_TYPE_ESID_REF          == tag ? "ESID_REF"
       : mtx::mp4::DESCRIPTOR_TYPE_FILE_IO           == tag ? "FILE_IO"
       : mtx::mp4::DESCRIPTOR_TYPE_FILE_O            == tag ? "FILE_O"
       : mtx::mp4::DESCRIPTOR_TYPE_EXT_PROFILE_LEVEL == tag ? "EXT_PROFILE_LEVEL"
       : mtx::mp4::DESCRIPTOR_TYPE_TAGS_START        == tag ? "TAGS_START"
       : mtx::mp4::DESCRIPTOR_TYPE_TAGS_END          == tag ? "TAGS_END"
       :                                  "unknown";
}

bool
qtmp4_reader_c::probe_file() {
  while (true) {
    uint64_t atom_pos  = m_in->getFilePointer();
    uint64_t atom_size = m_in->read_uint32_be();
    fourcc_c atom(*m_in);

    if (1 == atom_size)
      atom_size = m_in->read_uint64_be();

    if (   (atom == "moov")
        || (atom == "ftyp")
        || (atom == "mdat")
        || (atom == "pnot"))
      return true;

    if ((atom != "wide") && (atom != "skip"))
      return false;

    m_in->setFilePointer(atom_pos + atom_size);
  }
}

void
qtmp4_reader_c::read_headers() {
  try {
    show_demuxer_info();

    parse_headers();

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }
}

void
qtmp4_reader_c::calculate_num_bytes_to_process() {
  for (auto const &dmx : m_demuxers)
    if (demuxing_requested(dmx->type, dmx->id, dmx->language))
      m_bytes_to_process += std::accumulate(dmx->m_index.begin(), dmx->m_index.end(), 0ull, [](int64_t num, auto const &entry) { return num + entry.size; });
}

qt_atom_t
qtmp4_reader_c::read_atom(mm_io_c *read_from,
                          bool exit_on_error) {
  return read_qtmp4_atom(read_from ? read_from : m_in.get(), exit_on_error);
}

bool
qtmp4_reader_c::resync_to_top_level_atom(uint64_t start_pos) {
  static std::vector<std::string> const s_top_level_atoms{ "ftyp", "pdin", "moov", "moof", "mfra", "mdat", "free", "skip" };
  static auto test_atom_at = [this](uint64_t atom_pos, uint64_t expected_hsize, fourcc_c const &expected_fourcc) -> bool {
    m_in->setFilePointer(atom_pos);
    auto test_atom = read_atom(nullptr, false);
    mxdebug_if(m_debug_resync, fmt::format("Test for {0}bit offset atom: {1}\n", 8 == expected_hsize ? 32 : 64, test_atom));

    if ((test_atom.fourcc == expected_fourcc) && (test_atom.hsize == expected_hsize) && ((test_atom.pos + test_atom.size) <= m_size)) {
      mxdebug_if(m_debug_resync, fmt::format("{0}bit offset atom looks good\n", 8 == expected_hsize ? 32 : 64));
      m_in->setFilePointer(atom_pos);
      return true;
    }

    return false;
  };

  try {
    m_in->setFilePointer(start_pos);
    fourcc_c fourcc{m_in};
    auto next_pos = start_pos;

    mxdebug_if(m_debug_resync, fmt::format("Starting resync at {0}, FourCC {1}\n", start_pos, fourcc));
    while (true) {
      m_in->setFilePointer(next_pos);
      fourcc.shift_read(m_in);
      next_pos = m_in->getFilePointer();

      if (!fourcc.human_readable() || !fourcc.equiv(s_top_level_atoms))
        continue;

      auto fourcc_pos = m_in->getFilePointer() - 4;
      mxdebug_if(m_debug_resync, fmt::format("Human readable at {0}: {1}\n", fourcc_pos, fourcc));

      if (test_atom_at(fourcc_pos - 12, 16, fourcc) || test_atom_at(fourcc_pos - 4, 8, fourcc))
        return true;
    }

  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug_resync, fmt::format("I/O exception during resync: {0}\n", ex.what()));
  } catch (mtx::atom_chunk_size_x &ex) {
    mxdebug_if(m_debug_resync, fmt::format("Atom exception during resync: {0}\n", ex.error()));
  }

  return false;
}

void
qtmp4_reader_c::parse_headers() {
  unsigned int idx;

  m_in->setFilePointer(0);

  bool headers_parsed = false;
  bool mdat_found     = false;

  try {
    while (!m_in->eof()) {
      qt_atom_t atom = read_atom();
      mxdebug_if(m_debug_headers, fmt::format("'{0}' atom, size {1}, at {2}–{3}, human readable? {4}\n", atom.fourcc, atom.size, atom.pos, atom.pos + atom.size, atom.fourcc.human_readable()));

      if (atom.fourcc == "ftyp") {
        auto tmp = fourcc_c{m_in};
        mxdebug_if(m_debug_headers, fmt::format("  File type major brand: {0}\n", tmp));
        tmp = fourcc_c{m_in};
        mxdebug_if(m_debug_headers, fmt::format("  File type minor brand: {0}\n", tmp));

        for (idx = 0; idx < ((atom.size - 16) / 4); ++idx) {
          tmp = fourcc_c{m_in};
          mxdebug_if(m_debug_headers, fmt::format("  File type compatible brands #{0}: {1}\n", idx, tmp));
        }

      } else if (atom.fourcc == "moov") {
        if (!headers_parsed)
          handle_moov_atom(atom.to_parent(), 0);
        else
          m_in->setFilePointer(atom.pos + atom.size);
        headers_parsed = true;

      } else if (atom.fourcc == "mdat") {
        m_in->setFilePointer(atom.pos + atom.size);
        mdat_found = true;

      } else if (atom.fourcc == "moof") {
        handle_moof_atom(atom.to_parent(), 0, atom);

      } else if (atom.fourcc.human_readable())
        m_in->setFilePointer(atom.pos + atom.size);

      else if (!resync_to_top_level_atom(atom.pos))
        break;
    }
  } catch (mtx::mm_io::exception &) {
  }

  if (!headers_parsed)
    mxerror(Y("Quicktime/MP4 reader: Have not found any header atoms.\n"));

  if (!mdat_found)
    mxerror(Y("Quicktime/MP4 reader: Have not found the 'mdat' atom. No movie data found.\n"));

  verify_track_parameters_and_update_indexes();

  read_chapter_track();

  m_demuxers.erase(std::remove_if(m_demuxers.begin(), m_demuxers.end(), [](qtmp4_demuxer_cptr const &dmx) { return !dmx->ok || dmx->is_chapters(); }), m_demuxers.end());

  detect_interleaving();

  if (!g_identifying) {
    calculate_timestamps();
    calculate_num_bytes_to_process();
  }

  mxdebug_if(m_debug_headers, fmt::format("Number of valid tracks found: {0}\n", m_demuxers.size()));

  create_global_tags_from_meta_data();
}

void
qtmp4_reader_c::verify_track_parameters_and_update_indexes() {
  for (auto &dmx : m_demuxers) {
    if (m_chapter_track_ids[dmx->container_id])
      dmx->type = 'C';

    else if (!demuxing_requested(dmx->type, dmx->id, dmx->language))
      continue;

    else if (dmx->is_audio() && !dmx->verify_audio_parameters())
      continue;

    else if (dmx->is_video() && !dmx->verify_video_parameters())
      continue;

    else if (dmx->is_subtitles() && !dmx->verify_subtitles_parameters())
      continue;

    else if (dmx->is_unknown())
      continue;

    dmx->ok = dmx->update_tables();
  }
}

std::optional<int64_t>
qtmp4_reader_c::calculate_global_min_timestamp()
  const {
  std::optional<int64_t> global_min_timestamp;

  for (auto const &dmx : m_demuxers) {
    if (!demuxing_requested(dmx->type, dmx->id, dmx->language)) {
      continue;
    }

    auto dmx_min_timestamp = dmx->min_timestamp();
    if (dmx_min_timestamp && (!global_min_timestamp || (dmx_min_timestamp < *global_min_timestamp))) {
      global_min_timestamp = *dmx_min_timestamp;
    }

    mxdebug_if(m_debug_headers, fmt::format("Minimum timestamp of track {0}: {1}\n", dmx->id, dmx_min_timestamp ? mtx::string::format_timestamp(*dmx_min_timestamp) : "none"));
  }

  mxdebug_if(m_debug_headers, fmt::format("Minimum global timestamp: {0}\n", global_min_timestamp ? mtx::string::format_timestamp(*global_min_timestamp) : "none"));

  return global_min_timestamp;
}

void
qtmp4_reader_c::calculate_timestamps() {
  if (m_timestamps_calculated)
    return;

  for (auto &dmx : m_demuxers) {
    dmx->calculate_frame_rate();
    dmx->calculate_timestamps();
  }

  auto min_timestamp = calculate_global_min_timestamp();

  if (min_timestamp && (0 != *min_timestamp)) {
    for (auto &dmx : m_demuxers) {
      dmx->adjust_timestamps(-*min_timestamp);
    }
  }

  m_timestamps_calculated = true;
}

void
qtmp4_reader_c::handle_audio_encoder_delay(qtmp4_demuxer_c &dmx) {
  if ((0 == m_audio_encoder_delay_samples) || (0 == dmx.a_samplerate) || (-1 == dmx.ptzr))
    return;

  ptzr(dmx.ptzr).m_ti.m_tcsync.displacement -= (m_audio_encoder_delay_samples * 1000000000ll) / dmx.a_samplerate;
  m_audio_encoder_delay_samples               = 0;
}

void
qtmp4_reader_c::process_atom(qt_atom_t const &parent,
                             int level,
                             std::function<void(qt_atom_t const &)> const &handler) {
  auto parent_size = parent.size;

  while (8 <= parent_size) {
    auto atom = read_atom();
    mxdebug_if(m_debug_headers, fmt::format("{0}'{1}' atom, size {2}, at {3}–{4}\n", space(2 * level + 1), atom.fourcc, atom.size, atom.pos, atom.pos + atom.size));

    if (atom.size > parent_size)
      break;

    handler(atom);

    m_in->setFilePointer(atom.pos + atom.size);
    parent_size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_cmov_atom(qt_atom_t parent,
                                 int level) {
  m_compression_algorithm = fourcc_c{};

  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "dcom")
      handle_dcom_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "cmvd")
      handle_cmvd_atom(atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_cmvd_atom(qt_atom_t atom,
                                 int level) {
  uint32_t moov_size = m_in->read_uint32_be();
  mxdebug_if(m_debug_headers, fmt::format("{0}Uncompressed size: {1}\n", space((level + 1) * 2 + 1), moov_size));

  if (m_compression_algorithm != "zlib")
    mxerror(fmt::format(FY("Quicktime/MP4 reader: This file uses compressed headers with an unknown "
                           "or unsupported compression algorithm '{0}'. Aborting.\n"), m_compression_algorithm));

  auto old_in      = m_in;
  auto cmov_size   = atom.size - atom.hsize;
  auto af_cmov_buf = memory_c::alloc(cmov_size);
  auto cmov_buf    = af_cmov_buf->get_buffer();
  auto af_moov_buf = memory_c::alloc(moov_size + 16);
  auto moov_buf    = af_moov_buf->get_buffer();

  if (m_in->read(cmov_buf, cmov_size) != cmov_size)
    throw mtx::input::header_parsing_x();

  z_stream zs;
  zs.zalloc    = static_cast<alloc_func>(nullptr);
  zs.zfree     = static_cast<free_func>(nullptr);
  zs.opaque    = static_cast<voidpf>(nullptr);
  zs.next_in   = cmov_buf;
  zs.avail_in  = cmov_size;
  zs.next_out  = moov_buf;
  zs.avail_out = moov_size;

  int zret = inflateInit(&zs);
  if (Z_OK != zret)
    mxerror(fmt::format(FY("Quicktime/MP4 reader: This file uses compressed headers, but the zlib library could not be initialized. "
                           "Error code from zlib: {0}. Aborting.\n"), zret));

  zret = inflate(&zs, Z_NO_FLUSH);
  if ((Z_OK != zret) && (Z_STREAM_END != zret))
    mxerror(fmt::format(FY("Quicktime/MP4 reader: This file uses compressed headers, but they could not be uncompressed. "
                           "Error code from zlib: {0}. Aborting.\n"), zret));

  if (moov_size != zs.total_out)
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: This file uses compressed headers, but the expected uncompressed size ({0}) "
                          "was not what is available after uncompressing ({1}).\n"), moov_size, zs.total_out));

  zret = inflateEnd(&zs);

  m_in = mm_io_cptr(new mm_mem_io_c(moov_buf, zs.total_out));
  while (!m_in->eof()) {
    qt_atom_t next_atom = read_atom();
    mxdebug_if(m_debug_headers, fmt::format("{0}'{1}' atom at {2}\n", space((level + 1) * 2 + 1), next_atom.fourcc, next_atom.pos));

    if (next_atom.fourcc == "moov")
      handle_moov_atom(next_atom.to_parent(), level + 1);

    m_in->setFilePointer(next_atom.pos + next_atom.size);
  }
  m_in = old_in;
}

void
qtmp4_reader_c::handle_ctts_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  auto version = m_in->read_uint8();
  m_in->skip(3);                // version & flags

  auto count = m_in->read_uint32_be();
  mxdebug_if(m_debug_headers, fmt::format("{0}Frame offset table v{2}: {1} raw entries\n", space(level * 2 + 1), count, static_cast<unsigned int>(version)));

  dmx.raw_frame_offset_table.reserve(dmx.raw_frame_offset_table.size() + count);

  size_t i;
  for (i = 0; i < count; ++i) {
    qt_frame_offset_t frame_offset;

    frame_offset.count  = m_in->read_uint32_be();
    frame_offset.offset = mtx::math::to_signed(m_in->read_uint32_be());
    dmx.raw_frame_offset_table.push_back(frame_offset);
  }

  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.raw_frame_offset_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: count {2} offset {3}\n", space((level + 1) * 2 + 1), idx, dmx.raw_frame_offset_table[idx].count, dmx.raw_frame_offset_table[idx].offset));
}

void
qtmp4_reader_c::handle_sgpd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  auto version = m_in->read_uint8();
  m_in->skip(3);                // flags

  auto grouping_type              = fourcc_c{m_in};
  auto default_description_length = version == 1 ? m_in->read_uint32_be() : uint32_t{};

  if (version >= 2)
    m_in->skip(4);              // default_sample_description_index

  auto count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample group description table: version {1}, type '{2}', {3} raw entries\n", space(level * 2 + 2), static_cast<unsigned int>(version), grouping_type, count));

  if (grouping_type != "rap ")
    return;

  dmx.random_access_point_table.reserve(dmx.random_access_point_table.size() + count);

  for (auto idx = 0u; idx < count; ++idx) {
    if ((version == 1) && (default_description_length == 0))
      m_in->skip(4);            // description_length

    auto byte                      = m_in->read_uint8();
    auto num_leading_samples_known = (byte & 0x80) == 0x80;
    auto num_leading_samples       = static_cast<unsigned int>(byte & 0x7f);

    dmx.random_access_point_table.emplace_back(num_leading_samples_known, num_leading_samples);
  }

  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.random_access_point_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: leading samples known {2} num {3}\n", space((level + 1) * 2 + 2), idx, dmx.random_access_point_table[idx].num_leading_samples_known, dmx.random_access_point_table[idx].num_leading_samples));
}

void
qtmp4_reader_c::handle_sbgp_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  auto version = m_in->read_uint8();
  m_in->skip(3);                // flags

  auto grouping_type = fourcc_c{m_in};

  if (version == 1)
    m_in->skip(4);              // grouping_type_parameter

  auto count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample to group table: version {1}, type '{2}', {3} raw entries\n", space(level * 2 + 2), static_cast<unsigned int>(version), grouping_type, count));

  auto &table = dmx.sample_to_group_tables[grouping_type.value()];
  table.reserve(table.size() + count);

  for (auto idx = 0u; idx < count; ++idx) {
    auto sample_count            = m_in->read_uint32_be();
    auto group_description_index = m_in->read_uint32_be();

    table.emplace_back(sample_count, group_description_index);
  }

  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: sample count {2} group description {3}\n", space((level + 1) * 2 + 2), idx, table[idx].sample_count, table[idx].group_description_index));
}

void
qtmp4_reader_c::handle_dcom_atom(qt_atom_t,
                                 int level) {
  m_compression_algorithm = fourcc_c{m_in->read_uint32_be()};
  mxdebug_if(m_debug_headers, fmt::format("{0}Compression algorithm: {1}\n", space(level * 2 + 1), m_compression_algorithm));
}

void
qtmp4_reader_c::handle_hdlr_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  hdlr_atom_t hdlr;

  if (sizeof(hdlr_atom_t) > atom.size)
    print_atom_too_small_error("hdlr", atom, sizeof(hdlr_atom_t));

  if (m_in->read(&hdlr, sizeof(hdlr_atom_t)) != sizeof(hdlr_atom_t))
    throw mtx::input::header_parsing_x();

  mxdebug_if(m_debug_headers, fmt::format("{0}Component type: {1:4s} subtype: {2:4s}\n", space(level * 2 + 1), reinterpret_cast<char const *>(&hdlr.type), reinterpret_cast<char const *>(&hdlr.subtype)));

  auto subtype = fourcc_c{reinterpret_cast<uint8_t const *>(&hdlr) + offsetof(hdlr_atom_t, subtype)};
  if (subtype == "soun")
    dmx.type = 'a';

  else if (subtype == "vide")
    dmx.type = 'v';

  else if (mtx::included_in(subtype, "text", "subp", "sbtl"))
    dmx.type = 's';
}

void
qtmp4_reader_c::handle_mdhd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  if (1 > atom.size)
    print_atom_too_small_error("mdhd", atom, sizeof(mdhd_atom_t));

  int version = m_in->read_uint8();

  if (0 == version) {
    mdhd_atom_t mdhd;

    if (sizeof(mdhd_atom_t) > atom.size)
      print_atom_too_small_error("mdhd", atom, sizeof(mdhd_atom_t));
    if (m_in->read(&mdhd.flags, sizeof(mdhd_atom_t) - 1) != (sizeof(mdhd_atom_t) - 1))
      throw mtx::input::header_parsing_x();

    dmx.time_scale      = get_uint32_be(&mdhd.time_scale);
    dmx.global_duration = get_uint32_be(&mdhd.duration);
    dmx.language        = decode_and_verify_language(get_uint16_be(&mdhd.language));

  } else if (1 == version) {
    mdhd64_atom_t mdhd;

    if (sizeof(mdhd64_atom_t) > atom.size)
      print_atom_too_small_error("mdhd", atom, sizeof(mdhd64_atom_t));
    if (m_in->read(&mdhd.flags, sizeof(mdhd64_atom_t) - 1) != (sizeof(mdhd64_atom_t) - 1))
      throw mtx::input::header_parsing_x();

    dmx.time_scale      = get_uint32_be(&mdhd.time_scale);
    dmx.global_duration = get_uint64_be(&mdhd.duration);
    dmx.language        = decode_and_verify_language(get_uint16_be(&mdhd.language));

  } else
    mxerror(fmt::format(FY("Quicktime/MP4 reader: The 'media header' atom ('mdhd') uses the unsupported version {0}.\n"), version));

  mxdebug_if(m_debug_headers, fmt::format("{0}Time scale: {1}, duration: {2}, language: {3}\n", space(level * 2 + 1), dmx.time_scale, dmx.global_duration, dmx.language));

  if (0 == dmx.time_scale)
    mxerror(Y("Quicktime/MP4 reader: The 'time scale' parameter was 0. This is not supported.\n"));
}

void
qtmp4_reader_c::handle_mdia_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "mdhd")
      handle_mdhd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "hdlr")
      handle_hdlr_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "minf")
      handle_minf_atom(dmx, atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_minf_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "hdlr")
      handle_hdlr_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stbl")
      handle_stbl_atom(dmx, atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_moov_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "cmov")
      handle_cmov_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "mvhd")
      handle_mvhd_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "udta")
      handle_udta_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "mvex")
      handle_mvex_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "trak") {
      auto dmx = std::make_shared<qtmp4_demuxer_c>(*this);
      dmx->id  = m_demuxers.size();

      handle_trak_atom(*dmx, atom.to_parent(), level + 1);

      if ((!dmx->is_unknown() && dmx->codec) || dmx->is_subtitles() || dmx->is_video() || dmx->is_audio())
        m_demuxers.push_back(dmx);
    }
  });
}

void
qtmp4_reader_c::handle_mvex_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "trex")
      handle_trex_atom(atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_trex_atom(qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);            // Version, flags

  auto track_id                  = m_in->read_uint32_be();
  auto &defaults                 = m_track_defaults[track_id];
  defaults.sample_description_id = m_in->read_uint32_be();
  defaults.sample_duration       = m_in->read_uint32_be();
  defaults.sample_size           = m_in->read_uint32_be();
  defaults.sample_flags          = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample defaults for track ID {1}: description idx {2} duration {3} size {4} flags {5}\n",
                         space(level * 2 + 1), track_id, defaults.sample_description_id, defaults.sample_duration, defaults.sample_size, defaults.sample_flags));
}

void
qtmp4_reader_c::handle_moof_atom(qt_atom_t parent,
                                 int level,
                                 qt_atom_t const &moof_atom) {
  m_moof_offset              = moof_atom.pos;
  m_fragment_implicit_offset = moof_atom.pos;

  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "traf")
      handle_traf_atom(atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_traf_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "tfhd")
      handle_tfhd_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "trun")
      handle_trun_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "edts") {
      if (m_track_for_fragment)
        handle_edts_atom(*m_track_for_fragment, atom.to_parent(), level + 1);
    }

  });

  m_fragment           = nullptr;
  m_track_for_fragment = nullptr;
}

void
qtmp4_reader_c::handle_tfhd_atom(qt_atom_t,
                                 int level) {
  m_in->skip(1);                // Version

  auto flags     = m_in->read_uint24_be();
  auto track_id  = m_in->read_uint32_be();
  auto track_itr = std::find_if(m_demuxers.begin(), m_demuxers.end(), [track_id](qtmp4_demuxer_cptr const &dmx) { return dmx->container_id == track_id; });

  if (!track_id || !mtx::includes(m_track_defaults, track_id) || (m_demuxers.end() == track_itr)) {
    mxdebug_if(m_debug_headers,
               fmt::format("{0}tfhd atom with track_id({1}) == 0, no entry in trex for it or no track({2}) found\n",
                           space(level * 2 + 1), track_id, static_cast<void *>(m_demuxers.end() == track_itr ? nullptr : track_itr->get())));
    return;
  }

  auto &defaults = m_track_defaults[track_id];
  auto &track    = **track_itr;

  track.m_fragments.emplace_back();
  auto &fragment = track.m_fragments.back();

  fragment.track_id              = track_id;
  fragment.moof_offset           = m_moof_offset;
  fragment.implicit_offset       = m_fragment_implicit_offset;
  fragment.base_data_offset      = flags & QTMP4_TFHD_BASE_DATA_OFFSET      ? m_in->read_uint64_be()
                                 : flags & QTMP4_TFHD_DEFAULT_BASE_IS_MOOF  ? fragment.moof_offset
                                 :                                            fragment.implicit_offset;
  fragment.sample_description_id = flags & QTMP4_TFHD_SAMPLE_DESCRIPTION_ID ? m_in->read_uint32_be() : defaults.sample_description_id;
  fragment.sample_duration       = flags & QTMP4_TFHD_DEFAULT_DURATION      ? m_in->read_uint32_be() : defaults.sample_duration;
  fragment.sample_size           = flags & QTMP4_TFHD_DEFAULT_SIZE          ? m_in->read_uint32_be() : defaults.sample_size;
  fragment.sample_flags          = flags & QTMP4_TFHD_DEFAULT_FLAGS         ? m_in->read_uint32_be() : defaults.sample_flags;

  m_fragment           = &fragment;
  m_track_for_fragment = &track;

  mxdebug_if(m_debug_headers,
             fmt::format("{0}Atom flags 0x{1:05x} track_id {2} fragment content: moof_off {3} implicit_off {4} base_data_off {5} stsd_id {6} sample_duration {7} sample_size {8} sample_flags {9}\n",
                         space(level * 2 + 1), flags, track_id,
                         fragment.moof_offset, fragment.implicit_offset, fragment.base_data_offset, fragment.sample_description_id, fragment.sample_duration, fragment.sample_size, fragment.sample_flags));
}

void
qtmp4_reader_c::handle_trun_atom(qt_atom_t,
                                 int level) {
  if (!m_fragment || !m_track_for_fragment) {
    mxdebug_if(m_debug_headers, fmt::format("{0}No current fragment ({1}) or track for that fragment ({2})\n", space(2 * level + 1), static_cast<void *>(m_fragment), static_cast<void *>(m_track_for_fragment)));
    return;
  }

  m_in->skip(1);                // Version
  auto flags   = m_in->read_uint24_be();
  auto entries = m_in->read_uint32_be();
  auto &track  = *m_track_for_fragment;

  if (track.raw_frame_offset_table.empty() && !track.sample_table.empty())
    track.raw_frame_offset_table.emplace_back(track.sample_table.size(), 0);

  auto data_offset        = flags & QTMP4_TRUN_DATA_OFFSET ? m_in->read_uint32_be() : 0;
  auto first_sample_flags = flags & QTMP4_TRUN_FIRST_SAMPLE_FLAGS ? m_in->read_uint32_be() : m_fragment->sample_flags;
  auto offset             = m_fragment->base_data_offset + data_offset;

  auto calc_reserve_size  = [entries](auto current_size) {
    auto constexpr reserve_chunk_size = 1024u;
    return (((current_size + entries) / reserve_chunk_size) + 1) * reserve_chunk_size;
  };

  track.durmap_table.reserve(calc_reserve_size(track.durmap_table.size()));
  track.sample_table.reserve(calc_reserve_size(track.sample_table.size()));
  track.chunk_table.reserve(calc_reserve_size(track.chunk_table.size()));
  track.raw_frame_offset_table.reserve(calc_reserve_size(track.raw_frame_offset_table.size()));
  track.keyframe_table.reserve(calc_reserve_size(track.keyframe_table.size()));

  std::vector<uint32_t> all_sample_flags;
  std::vector<bool> all_keyframe_flags;

  if (m_debug_tables) {
    all_sample_flags.reserve(entries);
    all_keyframe_flags.reserve(entries);
  }

  for (auto idx = 0u; idx < entries; ++idx) {
    auto sample_duration = flags & QTMP4_TRUN_SAMPLE_DURATION   ? m_in->read_uint32_be() : m_fragment->sample_duration;
    auto sample_size     = flags & QTMP4_TRUN_SAMPLE_SIZE       ? m_in->read_uint32_be() : m_fragment->sample_size;
    auto sample_flags    = flags & QTMP4_TRUN_SAMPLE_FLAGS      ? m_in->read_uint32_be() : idx > 0 ? m_fragment->sample_flags : first_sample_flags;
    auto ctts_duration   = flags & QTMP4_TRUN_SAMPLE_CTS_OFFSET ? m_in->read_uint32_be() : 0;
    auto keyframe        = !track.is_video()                    ? true                   : !(sample_flags & (QTMP4_FRAG_SAMPLE_FLAG_IS_NON_SYNC | QTMP4_FRAG_SAMPLE_FLAG_DEPENDS_YES));

    track.durmap_table.emplace_back(1, sample_duration);
    track.sample_table.emplace_back(sample_size);
    track.chunk_table.emplace_back(1, offset);
    track.raw_frame_offset_table.emplace_back(1, mtx::math::to_signed(ctts_duration));

    if (keyframe)
      track.keyframe_table.emplace_back(track.num_frames_from_trun + 1);

    offset += sample_size;

    track.num_frames_from_trun++;

    if (!m_debug_tables)
      continue;

    all_sample_flags.emplace_back(sample_flags);
    all_keyframe_flags.push_back(keyframe);
  }

  m_fragment->implicit_offset = offset;
  m_fragment_implicit_offset  = offset;

  mxdebug_if(m_debug_headers, fmt::format("{0}Number of entries: {1}\n", space((level + 1) * 2 + 1), entries));

  if (!m_debug_tables)
    return;

  auto spc                = space((level + 2) * 2 + 1);
  auto durmap_start       = track.durmap_table.size()           - entries;
  auto sample_start       = track.sample_table.size()           - entries;
  auto chunk_start        = track.chunk_table.size()            - entries;
  auto frame_offset_start = track.raw_frame_offset_table.size() - entries;
  auto end                = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), entries);

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: duration {2} size {3} data start {4} end {5} pts offset {6} key? {7} raw flags 0x{8:08x}\n",
                        spc, idx,
                        track.durmap_table[durmap_start + idx].duration,
                        track.sample_table[sample_start + idx].size,
                        track.chunk_table[chunk_start + idx].pos,
                        (track.sample_table[sample_start + idx].size + track.chunk_table[chunk_start + idx].pos),
                        track.raw_frame_offset_table[frame_offset_start + idx].offset,
                        static_cast<unsigned int>(all_keyframe_flags[idx]),
                        all_sample_flags[idx]));
}

void
qtmp4_reader_c::handle_mvhd_atom(qt_atom_t atom,
                                 int level) {
  mvhd_atom_t mvhd;

  if (sizeof(mvhd_atom_t) > (atom.size - atom.hsize))
    print_atom_too_small_error("mvhd", atom, sizeof(mvhd_atom_t));
  if (m_in->read(&mvhd, sizeof(mvhd_atom_t)) != sizeof(mvhd_atom_t))
    throw mtx::input::header_parsing_x();

  m_time_scale  = get_uint32_be(&mvhd.time_scale);
  auto duration = get_uint32_be(&mvhd.duration);

  for (auto i = 0; i < 3; ++i) {
    for (auto j = 0; j < 3; ++j)
      m_display_matrix[i][j] = std::bit_cast<int32_t>(get_uint32_be(&mvhd.display_matrix[i][j]));
  }

  if ((duration != std::numeric_limits<uint32_t>::max()) && (m_time_scale != 0))
    m_duration = mtx::to_uint(mtx::rational(duration, m_time_scale) * 1'000'000'000ull);

  mxdebug_if(m_debug_headers, fmt::format("{0}Time scale: {1} duration: {2}\n", space(level * 2 + 1), m_time_scale, m_duration ? mtx::string::format_timestamp(*m_duration) : "—"s));
}

void
qtmp4_reader_c::handle_udta_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "chpl")
      handle_chpl_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "meta")
      handle_meta_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == fourcc_c{0xa96e'616du} && atom.size - atom.hsize > 4) { // ©nam
      try {
        auto content = read_string_atom(atom, 4);
        mtx::string::strip(content, true);
        m_ti.m_title = content;
      } catch (mtx::exception const &ex) {
        mxdebug_if(m_debug_headers, fmt::format("{0}exception while reading title: {1}\n", space(level * 2 + 1), ex.what()));
      }
    }
  });
}

void
qtmp4_reader_c::handle_chpl_atom(qt_atom_t,
                                 int level) {
  if (m_ti.m_no_chapters || m_chapters)
    return;

  m_in->skip(1 + 3 + 4);          // Version, flags, zero

  int count = m_in->read_uint8();
  mxdebug_if(m_debug_chapters, fmt::format("{0}Chapter list: {1} entries\n", space(level * 2 + 1), count));

  if (0 == count)
    return;

  std::vector<qtmp4_chapter_entry_t> entries;

  entries.reserve(count);

  int i;
  for (i = 0; i < count; ++i) {
    uint64_t timestamp = m_in->read_uint64_be() * 100;
    memory_cptr buf   = memory_c::alloc(m_in->read_uint8() + 1);
    memset(buf->get_buffer(), 0, buf->get_size());

    if (m_in->read(buf->get_buffer(), buf->get_size() - 1) != (buf->get_size() - 1))
      break;

    entries.push_back(qtmp4_chapter_entry_t(std::string(reinterpret_cast<char *>(buf->get_buffer())), timestamp));
  }

  recode_chapter_entries(entries);
  process_chapter_entries(level, entries);
}

void
qtmp4_reader_c::handle_meta_atom(qt_atom_t parent,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags

  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "ilst")
      handle_ilst_atom(atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_ilst_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "----")
      handle_4dashes_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "covr")
      handle_covr_atom(atom.to_parent(), level + 1);

    else if (mtx::included_in(atom.fourcc,
                              fourcc_c{0xa96e'616du},  // ©nam
                              fourcc_c{0xa974'6f6fu},  // ©too
                              fourcc_c{0xa963'6d74u})) // ©cmt
      handle_ilst_metadata_atom(atom.to_parent(), level + 1, atom.fourcc);
  });
}

std::string
qtmp4_reader_c::read_string_atom(qt_atom_t atom,
                                 size_t num_skipped) {
  if ((num_skipped + atom.hsize) > atom.size)
    return "";

  std::string string;
  size_t length = atom.size - atom.hsize - num_skipped;

  m_in->skip(num_skipped);
  m_in->read(string, length);

  return string;
}

void
qtmp4_reader_c::handle_4dashes_atom(qt_atom_t parent,
                                    int level) {
  std::string name, mean, data;

  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "name")
      name = read_string_atom(atom, 4);

    else if (atom.fourcc == "mean")
      mean = read_string_atom(atom, 4);

    else if (atom.fourcc == "data")
      data = read_string_atom(atom, 8);
  });

  mxdebug_if(m_debug_headers, fmt::format("{0}'----' content: name={1} mean={2} data={3}\n", space(2 * level + 1), name, mean, data));

  if (name == "iTunSMPB")
    parse_itunsmpb(data);
}

void
qtmp4_reader_c::handle_covr_atom(qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [this, level](qt_atom_t const &atom) {
    if (atom.fourcc != "data")
      return;

    size_t data_size = atom.size - atom.hsize;
    if (data_size <= 8)
      return;

    try {
      auto type = m_in->read_uint32_be();
      if (!mtx::included_in<int>(type, mtx::mp4::ATOM_DATA_TYPE_BMP, mtx::mp4::ATOM_DATA_TYPE_JPEG, mtx::mp4::ATOM_DATA_TYPE_PNG))
        return;

      m_in->skip(4);

      data_size -= 8;

      auto attach_mode         = attachment_requested(m_attachment_id);

      auto attachment          = std::make_shared<attachment_t>();

      attachment->name         = fmt::format(fmt::runtime("cover.{}"), type == mtx::mp4::ATOM_DATA_TYPE_PNG ? "png" : type == mtx::mp4::ATOM_DATA_TYPE_BMP ? "bmp" : "jpg");
      attachment->mime_type    = fmt::format(fmt::runtime("image/{}"), type == mtx::mp4::ATOM_DATA_TYPE_PNG ? "png" : type == mtx::mp4::ATOM_DATA_TYPE_BMP ? "bmp" : "jpeg");
      attachment->data         = m_in->read(data_size);
      attachment->ui_id        = m_attachment_id++;
      attachment->to_all_files = ATTACH_MODE_TO_ALL_FILES == attach_mode;
      attachment->source_file  = m_ti.m_fname;

      if (ATTACH_MODE_SKIP != attach_mode)
        add_attachment(attachment);

    } catch (mtx::exception const &ex) {
      mxdebug_if(m_debug_headers, fmt::format(fmt::runtime("{0}exception while reading cover art: {1}\n"), space((level + 1) * 2 + 1), ex.what()));
    }
  });
}

void
qtmp4_reader_c::handle_ilst_metadata_atom(qt_atom_t parent,
                                          int level,
                                          fourcc_c const &fourcc) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc != "data")
      return;

    size_t data_size = atom.size - atom.hsize;
    if (data_size <= 8)
      return;

    try {
      auto content = read_string_atom(atom, 8);
      mtx::string::strip(content, true);

      if (fourcc == fourcc_c{0xa96e'616du}) // ©nam
        m_ti.m_title = content;

      else if (fourcc == fourcc_c{0xa974'6f6fu}) // ©too
        m_encoder = content;

      else if (fourcc == fourcc_c{0xa963'6d74u}) // ©cmt
        m_comment = content;

    } catch (mtx::exception const &ex) {
      mxdebug_if(m_debug_headers, fmt::format("{0}exception while reading title: {1}\n", space((level + 1) * 2 + 1), ex.what()));
    }
  });
}

void
qtmp4_reader_c::parse_itunsmpb(std::string data) {
  data = to_utf8(Q(data).replace(QRegularExpression{"[^0-9da-fA-F]+"}, {}));

  if (16 > data.length())
    return;

  try {
    m_audio_encoder_delay_samples = mtx::string::from_hex(data.substr(8, 8));
  } catch (std::bad_cast &) {
  }
}

void
qtmp4_reader_c::read_chapter_track() {
  if (m_ti.m_no_chapters || m_chapters)
    return;

  auto chapter_dmx_itr = std::find_if(m_demuxers.begin(), m_demuxers.end(), [](qtmp4_demuxer_cptr const &dmx) { return dmx->is_chapters(); });
  if (m_demuxers.end() == chapter_dmx_itr)
    return;

  if ((*chapter_dmx_itr)->sample_table.empty())
    return;

  std::vector<qtmp4_chapter_entry_t> entries;
  uint64_t pts_scale_gcd = std::gcd(static_cast<uint64_t>(1000000000ull), static_cast<uint64_t>((*chapter_dmx_itr)->time_scale));
  uint64_t pts_scale_num = 1000000000ull                                         / pts_scale_gcd;
  uint64_t pts_scale_den = static_cast<uint64_t>((*chapter_dmx_itr)->time_scale) / pts_scale_gcd;

  entries.reserve((*chapter_dmx_itr)->sample_table.size());

  for (auto &sample : (*chapter_dmx_itr)->sample_table) {
    if (2 >= sample.size)
      continue;

    m_in->setFilePointer(sample.pos);
    memory_cptr chunk(memory_c::alloc(sample.size));
    if (m_in->read(chunk->get_buffer(), sample.size) != sample.size)
      continue;

    unsigned int name_len = get_uint16_be(chunk->get_buffer());
    if ((name_len + 2) > sample.size)
      continue;

    entries.push_back(qtmp4_chapter_entry_t(std::string(reinterpret_cast<char *>(chunk->get_buffer()) + 2, name_len),
                                            sample.pts * pts_scale_num / pts_scale_den));
  }

  recode_chapter_entries(entries);
  process_chapter_entries(0, entries);
}

void
qtmp4_reader_c::process_chapter_entries(int level,
                                        std::vector<qtmp4_chapter_entry_t> &entries) {
  if (entries.empty())
    return;

  mxdebug_if(m_debug_chapters, fmt::format("{0}{1} chapter(s):\n", space((level + 1) * 2 + 1), entries.size()));

  std::stable_sort(entries.begin(), entries.end());

  auto af_out = std::make_shared<mm_mem_io_c>(nullptr, 0, 1000);
  auto &out   = *af_out;
  out.set_file_name(m_ti.m_fname);
  out.write_bom("UTF-8");

  unsigned int i = 0;
  for (; entries.size() > i; ++i) {
    qtmp4_chapter_entry_t &chapter = entries[i];

    mxdebug_if(m_debug_chapters, fmt::format("{0}{1}: start {3} name {2}\n", space((level + 1) * 2 + 1), i, chapter.m_name, mtx::string::format_timestamp(chapter.m_timestamp)));

    out.puts(fmt::format("CHAPTER{0:02}={1:02}:{2:02}:{3:02}.{4:03}\n"
                         "CHAPTER{0:02}NAME={5}\n",
                         i,
                          chapter.m_timestamp / 60 / 60 / 1'000'000'000,
                         (chapter.m_timestamp      / 60 / 1'000'000'000) %    60,
                         (chapter.m_timestamp           / 1'000'000'000) %    60,
                         (chapter.m_timestamp           /     1'000'000) % 1'000,
                         chapter.m_name));
  }

  mm_text_io_c text_out(af_out);
  try {
    m_chapters = mtx::chapters::parse(&text_out, 0, -1, 0, m_ti.m_chapter_language, "", true);
    mtx::chapters::align_uids(m_chapters.get());

    auto const &sync = mtx::includes(m_ti.m_timestamp_syncs, track_info_c::chapter_track_id) ? m_ti.m_timestamp_syncs[track_info_c::chapter_track_id]
                     : mtx::includes(m_ti.m_timestamp_syncs, track_info_c::all_tracks_id)    ? m_ti.m_timestamp_syncs[track_info_c::all_tracks_id]
                     :                                                                         timestamp_sync_t{};
    mtx::chapters::adjust_timestamps(*m_chapters, sync.displacement, sync.factor);

  } catch (mtx::chapters::parser_x &ex) {
    mxerror(fmt::format(FY("The MP4 file '{0}' contains chapters whose format was not recognized. This is often the case if the chapters are not encoded in UTF-8. Use the '--chapter-charset' option in order to specify the charset to use.\n"), m_ti.m_fname));
  }
}

void
qtmp4_reader_c::handle_stbl_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "stts")
      handle_stts_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stsd")
      handle_stsd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stss")
      handle_stss_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stsc")
      handle_stsc_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stsz")
      handle_stsz_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stco")
      handle_stco_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "co64")
      handle_co64_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "ctts")
      handle_ctts_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "sgpd")
      handle_sgpd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "sbgp")
      handle_sbgp_atom(dmx, atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_stco_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, fmt::format("{0}Chunk offset table: {1} entries\n", space(level * 2 + 1), count));

  dmx.chunk_table.reserve(dmx.chunk_table.size() + count);

  for (auto i = 0u; i < count; ++i)
    dmx.chunk_table.emplace_back(0, m_in->read_uint32_be());

  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunk_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}  {1}\n", space(level * 2 + 1), dmx.chunk_table[idx].pos));
}

void
qtmp4_reader_c::handle_co64_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, fmt::format("{0}64bit chunk offset table: {1} entries\n", space(level * 2 + 1), count));

  dmx.chunk_table.reserve(dmx.chunk_table.size() + count);

  for (auto i = 0u; i < count; ++i)
    dmx.chunk_table.emplace_back(0, m_in->read_uint64_be());

  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunk_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}  {1}\n", space(level * 2 + 1), dmx.chunk_table[idx].pos));
}

void
qtmp4_reader_c::handle_stsc_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();
  size_t i;

  dmx.chunkmap_table.reserve(dmx.chunkmap_table.size() + count);

  for (i = 0; i < count; ++i) {
    qt_chunkmap_t chunkmap;

    chunkmap.first_chunk           = m_in->read_uint32_be() - 1;
    chunkmap.samples_per_chunk     = m_in->read_uint32_be();
    chunkmap.sample_description_id = m_in->read_uint32_be();
    dmx.chunkmap_table.push_back(chunkmap);
  }

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample to chunk/chunkmap table: {1} entries\n", space(level * 2 + 1), count));
  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunkmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: first_chunk {2} samples_per_chunk {3} sample_description_id {4}\n",
                        space((level + 1) * 2 + 1), idx, dmx.chunkmap_table[idx].first_chunk, dmx.chunkmap_table[idx].samples_per_chunk, dmx.chunkmap_table[idx].sample_description_id));
}

void
qtmp4_reader_c::handle_stsd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  size_t i;
  for (i = 0; i < count; ++i) {
    int64_t pos   = m_in->getFilePointer();
    uint32_t size = m_in->read_uint32_be();

    if (4 > size)
      mxerror(fmt::format(FY("Quicktime/MP4 reader: The 'size' field is too small in the stream description atom for track ID {0}.\n"), dmx.id));

    dmx.stsd = memory_c::alloc(size);
    auto priv     = dmx.stsd->get_buffer();

    put_uint32_be(priv, size);
    if (m_in->read(priv + sizeof(uint32_t), size - sizeof(uint32_t)) != (size - sizeof(uint32_t)))
      mxerror(fmt::format(FY("Quicktime/MP4 reader: Could not read the stream description atom for track ID {0}.\n"), dmx.id));

    dmx.handle_stsd_atom(size, level);

    m_in->setFilePointer(pos + size);
  }
}

void
qtmp4_reader_c::handle_stss_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  dmx.keyframe_table.reserve(dmx.keyframe_table.size() + count);

  size_t i;
  for (i = 0; i < count; ++i)
    dmx.keyframe_table.push_back(m_in->read_uint32_be());

  std::sort(dmx.keyframe_table.begin(), dmx.keyframe_table.end());

  mxdebug_if(m_debug_headers, fmt::format("{0}Sync/keyframe table: {1} entries\n", space(level * 2 + 1), count));
  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.keyframe_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}keyframe at {1}\n", space((level + 1) * 2 + 1), dmx.keyframe_table[idx]));
}

void
qtmp4_reader_c::handle_stsz_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t sample_size = m_in->read_uint32_be();
  uint32_t count       = m_in->read_uint32_be();

  if (0 == sample_size) {
    dmx.sample_table.reserve(dmx.sample_table.size() + count);

    size_t i;
    for (i = 0; i < count; ++i) {
      qt_sample_t sample;

      sample.size = m_in->read_uint32_be();

      // This is a sanity check against damaged samples. I have one of
      // those in which one sample was suppposed to be > 2GB big.
      if (sample.size >= 100 * 1024 * 1024)
        sample.size = 0;

      dmx.sample_table.push_back(sample);
    }

    mxdebug_if(m_debug_headers, fmt::format("{0}Sample size table: {1} entries\n", space(level * 2 + 1), count));
    if (m_debug_tables) {
      auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.sample_table.size());

      for (auto idx = 0u; idx < end; ++idx)
        mxdebug(fmt::format("{0}{1}: size {2}\n", space((level + 1) * 2 + 1), idx, dmx.sample_table[idx].size));
    }

  } else {
    dmx.sample_size = sample_size;
    mxdebug_if(m_debug_headers, fmt::format("{0}Sample size table; global sample size: {1}\n", space(level * 2 + 1), sample_size));
  }
}

void
qtmp4_reader_c::handle_sttd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  dmx.durmap_table.reserve(dmx.durmap_table.size() + count);

  size_t i;
  for (i = 0; i < count; ++i) {
    qt_durmap_t durmap;

    durmap.number   = m_in->read_uint32_be();
    durmap.duration = m_in->read_uint32_be();
    dmx.durmap_table.push_back(durmap);
  }

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample duration table: {1} entries\n", space(level * 2 + 1), count));
  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.durmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: number {2} duration {3}\n", space((level + 1) * 2 + 1), idx, dmx.durmap_table[idx].number, dmx.durmap_table[idx].duration));
}

void
qtmp4_reader_c::handle_stts_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  dmx.durmap_table.reserve(dmx.durmap_table.size() + count);

  size_t i;
  for (i = 0; i < count; ++i) {
    qt_durmap_t durmap;

    durmap.number   = m_in->read_uint32_be();
    durmap.duration = m_in->read_uint32_be();
    dmx.durmap_table.push_back(durmap);
  }

  mxdebug_if(m_debug_headers, fmt::format("{0}Sample duration table: {1} entries\n", space(level * 2 + 1), count));
  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.durmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: number {2} duration {3}\n", space((level + 1) * 2 + 1), idx, dmx.durmap_table[idx].number, dmx.durmap_table[idx].duration));
}

void
qtmp4_reader_c::handle_edts_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "elst")
      handle_elst_atom(dmx, atom.to_parent(), level + 1);
  });
}

void
qtmp4_reader_c::handle_elst_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  uint8_t version = m_in->read_uint8();
  m_in->skip(3);                // flags
  uint32_t count  = m_in->read_uint32_be();
  dmx.editlist_table.resize(count);

  size_t i;
  for (i = 0; i < count; ++i) {
    auto &editlist = dmx.editlist_table[i];

    if (1 == version) {
      editlist.segment_duration = m_in->read_uint64_be();
      editlist.media_time       = static_cast<int64_t>(m_in->read_uint64_be());
    } else {
      editlist.segment_duration = m_in->read_uint32_be();
      editlist.media_time       = static_cast<int32_t>(m_in->read_uint32_be());
    }
    editlist.media_rate_integer  = m_in->read_uint16_be();
    editlist.media_rate_fraction = m_in->read_uint16_be();
  }

  mxdebug_if(m_debug_headers, fmt::format("{0}Edit list table: {1} entries\n", space(level * 2 + 1), count));
  if (!m_debug_tables)
    return;

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.editlist_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("{0}{1}: segment duration {2} media time {3} media rate {4}.{5}\n",
                        space((level + 1) * 2 + 1), idx, dmx.editlist_table[idx].segment_duration, dmx.editlist_table[idx].media_time, dmx.editlist_table[idx].media_rate_integer, dmx.editlist_table[idx].media_rate_fraction));
}

void
qtmp4_reader_c::handle_display_matrix(qtmp4_demuxer_c &dmx, int level) {
  dmx.yaw = dmx.roll = 0;

  int32_t display_matrix[3][3];
  for (auto i = 0; i < 3; ++i) {
    for (auto j = 0; j < 3; ++j)
      display_matrix[i][j] = std::bit_cast<int32_t>(m_in->read_uint32_be());
  }

  // Columns 1 and 2 are 16.16 fixed point, while column 3 is 2.30 fixed point.
  const int32_t shifts[3] = { 16, 16, 30 };
  int32_t matrix[3][3] = { { 0 } };
  // display matrix = track display matrix x movie display matrix
  for (auto i = 0; i < 3; ++i) {
    for (auto j = 0; j < 3; ++j) {
      for (auto k = 0; k < 3; ++k)
        matrix[i][j] += (static_cast<int64_t>(display_matrix[i][k]) * m_display_matrix[k][j]) >> shifts[k];
    }
  }

  bool ignored = true;
  mtx::at_scope_exit_c show_ignored_warning([&]() {
    if (ignored)
      mxdebug_if(m_debug_headers, fmt::format("{0}Track ID: {1} ignoring display matrix indicating non-orthogonal transformation\n", space(level * 2 + 1), dmx.container_id));
  });

  // Check whether this is an affine transformation.
  if (matrix[0][2] || matrix[1][2])
    return;

  // This together with the checks below test whether the upper-left 2x2 matrix is nonsingular.
  if (!matrix[0][0] && !matrix[0][1])
    return;

  // We ignore the translation part of the matrix (matrix[2][0] and matrix[2][1]) as well as any scaling, i.e. we only
  // look at the upper left 2x2 matrix.  We only accept matrices that are an exact multiple of an orthogonal one.  Apart
  // from the multiple, every such matrix can be obtained by potentially flipping in the x-direction (corresponding to
  // yaw = 180) followed by a rotation of (say) an angle phi in the counterclockwise direction. The upper-left 2x2
  // matrix then looks like this:
  //
  //         | (+/-)cos(phi) (-/+)sin(phi) |
  // scale * |                             |
  //         |      sin(phi)      cos(phi) |
  //
  // The first set of signs in the first row apply in case of no flipping, the second set applies in case of flipping.

  // The casts to int64_t are needed because -INT32_MIN doesn't fit in an int32_t.
  if (   (matrix[0][0] == matrix[1][1])
      && (-static_cast<int64_t>(matrix[0][1]) == matrix[1][0]))
    dmx.yaw = 0;
  else if (   (-static_cast<int64_t>(matrix[0][0]) == matrix[1][1])
           && (matrix[0][1] == matrix[1][0]))
    dmx.yaw = 180;
  else
    return;
  dmx.roll = 180 / M_PI * atan2(matrix[1][0], matrix[1][1]);
  ignored = false;

  if (!dmx.yaw && !dmx.roll)
    return;

  mxdebug_if(m_debug_headers, fmt::format("{0}Track ID: {1} yaw {2}, roll {3}\n", space(level * 2 + 1), dmx.container_id, dmx.yaw, dmx.roll));
}

void
qtmp4_reader_c::handle_tkhd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  if (atom.size < 1)
    print_atom_too_small_error("tkhd", atom, 1);

  auto version       = m_in->read_uint8();
  auto expected_size = 4u + 2 * (version == 1 ? 8 : 4) + 4 + 4 + (version == 1 ? 8 : 4) + 2 * 4 + 3 * 2 + 2 + 9 * 4 + 2 * 4;

  if (atom.size < expected_size)
    print_atom_too_small_error("tkhd", atom, expected_size);

  auto flags = m_in->read_uint24_be();

  m_in->skip(2 * (version == 1 ? 8 : 4)); // creation_time, modification_time

  dmx.container_id = m_in->read_uint32_be();

  m_in->skip(4                        // reserved
             + (version == 1 ? 8 : 4) // duration
             + 2 * 4                  // reserved
             + 3 * 2                  // layer, alternate_group, volume
             + 2);                    // reserved

  handle_display_matrix(dmx, level);

  dmx.m_enabled            = (flags & QTMP4_TKHD_FLAG_ENABLED) == QTMP4_TKHD_FLAG_ENABLED;
  dmx.v_display_width_flt  = m_in->read_uint32_be();
  dmx.v_display_height_flt = m_in->read_uint32_be();
  dmx.v_width              = dmx.v_display_width_flt  >> 16;
  dmx.v_height             = dmx.v_display_height_flt >> 16;

  mxdebug_if(m_debug_headers,
             fmt::format("{0}Track ID: {1} display width × height {2}.{3} × {4}.{5}\n",
                         space(level * 2 + 1), dmx.container_id, dmx.v_display_width_flt >> 16, dmx.v_display_width_flt & 0xffff, dmx.v_display_height_flt >> 16, dmx.v_display_height_flt & 0xffff));
}

void
qtmp4_reader_c::handle_tref_atom(qtmp4_demuxer_c &/* dmx */,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (12 > atom.size)
      return;

    std::vector<uint32_t> track_ids;
    for (auto idx = (atom.size - 4) / 8; 0 < idx; --idx)
      track_ids.push_back(m_in->read_uint32_be());

    if (atom.fourcc == "chap")
      for (auto track_id : track_ids)
        m_chapter_track_ids[track_id] = true;

    if (!m_debug_headers)
      return;

    std::string message;
    for (auto track_id : track_ids)
      message += fmt::format(" {0}", track_id);
    mxdebug(fmt::format("{0}Reference type: {1}; track IDs:{2}\n", space(level * 2 + 1), atom.fourcc, message));
  });
}

void
qtmp4_reader_c::handle_trak_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  process_atom(parent, level, [&](qt_atom_t const &atom) {
    if (atom.fourcc == "tkhd")
      handle_tkhd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "mdia")
      handle_mdia_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "edts")
      handle_edts_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "tref")
      handle_tref_atom(dmx, atom.to_parent(), level + 1);
  });

  // Only set these after parsing all sub atoms, because we only know if it's a video at the end.
  if (dmx.yaw || dmx.roll) {
    if (dmx.is_video()) {
      m_ti.m_projection_pose_yaw.set(dmx.yaw, option_source_e::container);
      m_ti.m_projection_pose_roll.set(dmx.roll, option_source_e::container);
    } else
      dmx.yaw = dmx.roll = 0;
  }

  dmx.determine_codec();
  mxdebug_if(m_debug_headers, fmt::format("{0}Codec determination result: {1} ok {2} type {3}\n", space(level * 2 + 1), dmx.codec.get_name(), dmx.ok, dmx.type));
}

file_status_e
qtmp4_reader_c::read(generic_packetizer_c *packetizer,
                     bool) {
  size_t dmx_idx;

  for (dmx_idx = 0; dmx_idx < m_demuxers.size(); ++dmx_idx) {
    auto &dmx = *m_demuxers[dmx_idx];

    if ((-1 == dmx.ptzr) || (&ptzr(dmx.ptzr) != packetizer))
      continue;

    if (dmx.pos < dmx.m_index.size())
      break;
  }

  if (m_demuxers.size() == dmx_idx)
    return finish();

  auto &dmx   = *m_demuxers[dmx_idx];
  auto &index = dmx.m_index[dmx.pos];

  m_in->setFilePointer(index.file_pos);

  int buffer_offset = 0;
  memory_cptr buffer;

  if (   dmx.is_video()
      && !dmx.pos
      && dmx.codec.is(codec_c::type_e::V_MPEG4_P2)
      && dmx.esds_parsed
      && (dmx.esds.decoder_config)) {
    buffer        = memory_c::alloc(index.size + dmx.esds.decoder_config->get_size());
    buffer_offset = dmx.esds.decoder_config->get_size();

    memcpy(buffer->get_buffer(), dmx.esds.decoder_config->get_buffer(), dmx.esds.decoder_config->get_size());

  } else {
    buffer = memory_c::alloc(index.size);
  }

  if (m_in->read(buffer->get_buffer() + buffer_offset, index.size) != static_cast<uint64_t>(index.size)) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Could not read chunk number {0}/{1} with size {2} from position {3}. Aborting.\n"),
                       dmx.pos, dmx.m_index.size(), index.size, index.file_pos));
    return finish();
  }

  auto duration = dmx.m_use_frame_rate_for_duration ? *dmx.m_use_frame_rate_for_duration : index.duration;
  auto packet   = std::make_shared<packet_t>(buffer, index.timestamp, duration, index.is_keyframe ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC, VFT_NOBFRAME);
  dmx.process(packet);

  ++dmx.pos;

  m_bytes_processed += index.size;

  if (dmx.pos < dmx.m_index.size())
    return FILE_STATUS_MOREDATA;

  return finish();
}

file_status_e
qtmp4_reader_c::finish() {
  for (auto const &dmx : m_demuxers)
    if (dmx->m_converter)
      dmx->m_converter->flush();

  return flush_packetizers();
}

memory_cptr
qtmp4_reader_c::create_bitmap_info_header(qtmp4_demuxer_c &dmx,
                                          const char *fourcc,
                                          size_t extra_size,
                                          const void *extra_data) {
  int full_size = sizeof(alBITMAPINFOHEADER) + extra_size;
  auto bih_p    = memory_c::alloc(full_size);
  auto bih      = reinterpret_cast<alBITMAPINFOHEADER *>(bih_p->get_buffer());

  memset(bih, 0, full_size);
  put_uint32_le(&bih->bi_size,       full_size);
  put_uint32_le(&bih->bi_width,      dmx.v_width);
  put_uint32_le(&bih->bi_height,     dmx.v_height);
  put_uint16_le(&bih->bi_planes,     1);
  put_uint16_le(&bih->bi_bit_count,  dmx.v_bitdepth);
  put_uint32_le(&bih->bi_size_image, dmx.v_width * dmx.v_height * 3);
  memcpy(&bih->bi_compression, fourcc, 4);

  if (0 != extra_size)
    memcpy(bih + 1, extra_data, extra_size);

  return bih_p;
}

bool
qtmp4_reader_c::create_audio_packetizer_ac3(qtmp4_demuxer_c &dmx) {
  auto buf = dmx.read_first_bytes(64);

  if (!buf || (-1 == dmx.m_ac3_header.find_in(buf))) {
    mxwarn_tid(m_ti.m_fname, dmx.id, Y("No AC-3 header found in first frame; track will be skipped.\n"));
    dmx.ok = false;

    return false;
  }

  dmx.ptzr = add_packetizer(new ac3_packetizer_c(this, m_ti, dmx.m_ac3_header.m_sample_rate, dmx.m_ac3_header.m_channels, dmx.m_ac3_header.m_bs_id));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));

  return true;
}

bool
qtmp4_reader_c::create_audio_packetizer_alac(qtmp4_demuxer_c &dmx) {
  auto magic_cookie = memory_c::clone(dmx.stsd->get_buffer() + dmx.stsd_non_priv_struct_size + 12, dmx.stsd->get_size() - dmx.stsd_non_priv_struct_size - 12);
  dmx.ptzr          = add_packetizer(new alac_packetizer_c(this, m_ti, magic_cookie, dmx.a_samplerate, dmx.a_channels));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));

  return true;
}

bool
qtmp4_reader_c::create_audio_packetizer_dts(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new dts_packetizer_c(this, m_ti, dmx.m_dts_header));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));

  return true;
}

void
qtmp4_reader_c::create_video_packetizer_svq1(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = create_bitmap_info_header(dmx, "SVQ1");
  dmx.ptzr            = add_packetizer(new video_for_windows_packetizer_c(this, m_ti, 0, dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpeg4_p2(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = create_bitmap_info_header(dmx, "DIVX");
  dmx.ptzr            = add_packetizer(new mpeg4_p2_video_packetizer_c(this, m_ti, 0, dmx.v_width, dmx.v_height, false));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpeg1_2(qtmp4_demuxer_c &dmx) {
  int version = !dmx.esds_parsed                                             ? (dmx.fourcc.value() & 0xff) - '0'
              : dmx.esds.object_type_id == mtx::mp4::OBJECT_TYPE_MPEG1Visual ? 1
              :                                                                2;
  dmx.ptzr    = add_packetizer(new mpeg1_2_video_packetizer_c(this, m_ti, version, -1, dmx.v_width, dmx.v_height, 0, 0, false));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_av1(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.empty() || (4 > dmx.priv[0]->get_size()) ? memory_cptr{} : dmx.priv[0];
  dmx.ptzr            = add_packetizer(new av1_video_packetizer_c(this, m_ti, dmx.v_width, dmx.v_height));

  if (dmx.frame_rate)
    ptzr(dmx.ptzr).set_track_default_duration(mtx::to_int(mtx_mp_rational_t{boost::multiprecision::denominator(dmx.frame_rate), boost::multiprecision::numerator(dmx.frame_rate)} * 1'000'000'000ll));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_avc(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() ? dmx.priv[0] : memory_cptr{};
  dmx.ptzr            = add_packetizer(new avc_video_packetizer_c(this, m_ti, dmx.frame_rate ? mtx::to_int(1'000'000'000 / dmx.frame_rate) : 0, dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpegh_p2_es(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() && dmx.priv[0]->get_size() ? dmx.priv[0] : memory_cptr{};
  dmx.ptzr            = add_packetizer(new hevc_es_video_packetizer_c(this, m_ti, dmx.v_width, dmx.v_height));

  if (boost::multiprecision::numerator(dmx.frame_rate)) {
    auto duration = mtx::to_int(mtx_mp_rational_t{boost::multiprecision::denominator(dmx.frame_rate), boost::multiprecision::numerator(dmx.frame_rate)} * 1'000'000'000ll);
    ptzr(dmx.ptzr).set_track_default_duration(duration);
  }

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpegh_p2(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() && dmx.priv[0]->get_size() ? dmx.priv[0] : memory_cptr{};
  auto packetizer     = new hevc_video_packetizer_c(this, m_ti, 0, dmx.v_width, dmx.v_height);
  dmx.ptzr            = add_packetizer(packetizer);

  if (boost::multiprecision::numerator(dmx.frame_rate)) {
    auto duration = mtx::to_int(mtx_mp_rational_t{boost::multiprecision::denominator(dmx.frame_rate), boost::multiprecision::numerator(dmx.frame_rate)} * 1'000'000'000ll);
    packetizer->set_track_default_duration(duration);
  }

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_prores(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = memory_c::clone(dmx.fourcc.str());
  dmx.ptzr            = add_packetizer(new prores_video_packetizer_c(this, m_ti, mtx::to_int(1'000'000'000.0 / dmx.frame_rate), dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_standard(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.stsd;
  dmx.ptzr            = add_packetizer(new quicktime_video_packetizer_c(this, m_ti, dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_vpx(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() && dmx.priv[0]->get_size() ? dmx.priv[0] : memory_cptr{};
  dmx.ptzr            = add_packetizer(new vpx_video_packetizer_c{this, m_ti, dmx.codec.get_type()});

  ptzr(dmx.ptzr).set_video_pixel_dimensions(dmx.v_width, dmx.v_height);

  if (boost::multiprecision::numerator(dmx.frame_rate)) {
    auto duration = mtx::to_int(mtx_mp_rational_t{boost::multiprecision::denominator(dmx.frame_rate), boost::multiprecision::numerator(dmx.frame_rate)} * 1'000'000'000ll);
    ptzr(dmx.ptzr).set_track_default_duration(duration);
  }

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_aac(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.esds.decoder_config;
  dmx.ptzr            = add_packetizer(new aac_packetizer_c(this, m_ti, *dmx.a_aac_audio_config, aac_packetizer_c::headerless));

  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

#if defined(HAVE_FLAC_FORMAT_H)
void
qtmp4_reader_c::create_audio_packetizer_flac(qtmp4_demuxer_c &dmx) {
  auto const &priv = *dmx.priv.at(0);
  dmx.ptzr         = add_packetizer(new flac_packetizer_c{this, m_ti, priv.get_buffer(), priv.get_size()});
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}
#endif

void
qtmp4_reader_c::create_audio_packetizer_mp3(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, dmx.a_samplerate, dmx.a_channels, true));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_opus(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv[0];
  dmx.ptzr            = add_packetizer(new opus_packetizer_c{this, m_ti});
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_pcm(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new pcm_packetizer_c(this, m_ti, static_cast<int32_t>(dmx.a_samplerate), dmx.a_channels, dmx.a_bitdepth, dmx.m_pcm_format));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_vorbis(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new vorbis_packetizer_c(this, m_ti, dmx.priv));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_passthrough(qtmp4_demuxer_c &dmx) {
  auto packetizer = new passthrough_packetizer_c{this, m_ti, track_audio};
  dmx.ptzr        = add_packetizer(packetizer);

  packetizer->set_codec_id(MKV_A_QUICKTIME);
  packetizer->set_codec_private(dmx.stsd);
  packetizer->set_audio_sampling_freq(dmx.a_samplerate);
  packetizer->set_audio_channels(dmx.a_channels);
  packetizer->prevent_lacing();

  show_packetizer_info(dmx.id, *packetizer);
}

void
qtmp4_reader_c::create_subtitles_packetizer_timed_text(qtmp4_demuxer_c &dmx) {
  dmx.ptzr        = add_packetizer(new textsubs_packetizer_c{this, m_ti, MKV_S_TEXTUTF8});
  dmx.m_converter = std::make_shared<timed_text_to_text_utf8_converter_c>(ptzr(dmx.ptzr));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_subtitles_packetizer_vobsub(qtmp4_demuxer_c &dmx) {
  std::string palette;
  auto format = [](int v) { return fmt::format("{0:02x}", std::min(std::max(v, 0), 255)); };
  auto buf    = dmx.esds.decoder_config->get_buffer();
  for (auto i = 0; i < 16; ++i) {
		int y = static_cast<int>(buf[(i << 2) + 1]) - 0x10;
		int u = static_cast<int>(buf[(i << 2) + 3]) - 0x80;
		int v = static_cast<int>(buf[(i << 2) + 2]) - 0x80;
		int r = (298 * y           + 409 * v + 128) >> 8;
		int g = (298 * y - 100 * u - 208 * v + 128) >> 8;
		int b = (298 * y + 516 * u           + 128) >> 8;

    if (i)
      palette += ", ";

    palette += format(r) + format(g) + format(b);
  }

  auto idx_str = mtx::vobsub::create_default_index(dmx.v_width, dmx.v_height, palette);

  mxdebug_if(m_debug_headers, fmt::format("VobSub IDX str:\n{0}", idx_str));

  m_ti.m_private_data = memory_c::clone(idx_str);
  dmx.ptzr = add_packetizer(new vobsub_packetizer_c(this, m_ti));
  show_packetizer_info(dmx.id, ptzr(dmx.ptzr));
}

void
qtmp4_reader_c::create_packetizer(int64_t tid) {
  unsigned int i;
  qtmp4_demuxer_cptr dmx_ptr;

  for (i = 0; i < m_demuxers.size(); ++i)
    if (m_demuxers[i]->id == tid) {
      dmx_ptr = m_demuxers[i];
      break;
    }

  if (!dmx_ptr)
    return;

  auto &dmx = *dmx_ptr;
  if (!dmx.ok || !demuxing_requested(dmx.type, dmx.id, dmx.language) || (-1 != dmx.ptzr)) {
    return;
  }

  m_ti.m_id       = dmx.id;
  m_ti.m_language = dmx.language;
  m_ti.m_private_data.reset();

  bool packetizer_ok = true;

  if (dmx.is_video()) {
    if (dmx.codec.is(codec_c::type_e::V_MPEG12))
      create_video_packetizer_mpeg1_2(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_MPEG4_P2))
      create_video_packetizer_mpeg4_p2(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_MPEG4_P10))
      create_video_packetizer_avc(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_MPEGH_P2) && dmx.m_hevc_is_annex_b)
      create_video_packetizer_mpegh_p2_es(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_MPEGH_P2))
      create_video_packetizer_mpegh_p2(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_AV1))
      create_video_packetizer_av1(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_SVQ1))
      create_video_packetizer_svq1(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_PRORES))
      create_video_packetizer_prores(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_VP8) || dmx.codec.is(codec_c::type_e::V_VP9))
      create_video_packetizer_vpx(dmx);

    else
      create_video_packetizer_standard(dmx);

    dmx.set_packetizer_display_dimensions();
    dmx.set_packetizer_color_properties();

  } else if (dmx.is_audio()) {
    if (dmx.codec.is(codec_c::type_e::A_AAC))
      create_audio_packetizer_aac(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_MP2) || dmx.codec.is(codec_c::type_e::A_MP3))
      create_audio_packetizer_mp3(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_PCM))
      create_audio_packetizer_pcm(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_AC3))
      packetizer_ok = create_audio_packetizer_ac3(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_ALAC))
      packetizer_ok = create_audio_packetizer_alac(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_DTS))
      packetizer_ok = create_audio_packetizer_dts(dmx);

#if defined(HAVE_FLAC_FORMAT_H)
    else if (dmx.codec.is(codec_c::type_e::A_FLAC))
      create_audio_packetizer_flac(dmx);
#endif

    else if (dmx.codec.is(codec_c::type_e::A_OPUS))
      create_audio_packetizer_opus(dmx);

    else if (dmx.codec.is(codec_c::type_e::A_VORBIS))
      create_audio_packetizer_vorbis(dmx);

    else
      create_audio_packetizer_passthrough(dmx);

    handle_audio_encoder_delay(dmx);

  } else {
    if (dmx.codec.is(codec_c::type_e::S_VOBSUB))
      create_subtitles_packetizer_vobsub(dmx);

    else if (dmx.codec.is(codec_c::type_e::S_TX3G))
      create_subtitles_packetizer_timed_text(dmx);
  }

  if (packetizer_ok) {
    dmx.set_packetizer_block_addition_mappings();
    m_reader_packetizers[dmx.ptzr]->set_track_enabled_flag(dmx.m_enabled, option_source_e::container);
  }

  if (packetizer_ok && (-1 == m_main_dmx))
    m_main_dmx = i;
}

void
qtmp4_reader_c::create_global_tags_from_meta_data() {
  if (m_comment.empty() && m_encoder.empty())
    return;

  m_tags.reset(mtx::construct::cons<libmatroska::KaxTags>(mtx::construct::cons<libmatroska::KaxTag>(mtx::construct::cons<libmatroska::KaxTagTargets>(new libmatroska::KaxTagTargetTypeValue, 50,
                                                                                                                                                     new libmatroska::KaxTagTargetType,      "MOVIE"))));
  auto &tag = static_cast<libmatroska::KaxTag &>(*(*m_tags)[0]);

  if (!m_comment.empty())
    tag.PushElement(*mtx::construct::cons<libmatroska::KaxTagSimple>(new libmatroska::KaxTagName,   "COMMENT",
                                                                     new libmatroska::KaxTagString, m_comment));

  if (!m_encoder.empty())
    tag.PushElement(*mtx::construct::cons<libmatroska::KaxTagSimple>(new libmatroska::KaxTagName,   "ENCODER",
                                                                     new libmatroska::KaxTagString, m_encoder));
}

void
qtmp4_reader_c::process_global_tags() {
  if (!m_tags || g_identifying)
    return;

  for (auto tag : *m_tags)
    add_tags(static_cast<libmatroska::KaxTag &>(*tag));

  m_tags->RemoveAll();
}

void
qtmp4_reader_c::create_packetizers() {
  maybe_set_segment_title(m_ti.m_title);
  if (!m_ti.m_no_global_tags)
    process_global_tags();

  m_main_dmx = -1;

  for (unsigned int i = 0; i < m_demuxers.size(); ++i)
    create_packetizer(m_demuxers[i]->id);
}

int64_t
qtmp4_reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
qtmp4_reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

void
qtmp4_reader_c::identify() {
  unsigned int i;
  auto info = mtx::id::info_c{};

  if (!m_ti.m_title.empty())
    info.add(mtx::id::title, m_ti.m_title);

  id_result_container(info.get());

  if (m_tags)
    id_result_tags(ID_RESULT_GLOBAL_TAGS_ID, mtx::tags::count_simple(*m_tags));

  for (i = 0; i < m_demuxers.size(); ++i) {
    auto &dmx = *m_demuxers[i];
    info      = mtx::id::info_c{};

    info.set(mtx::id::number, dmx.container_id);
    info.set(mtx::id::enabled_track, dmx.m_enabled);

    if (dmx.codec.is(codec_c::type_e::V_MPEG4_P10))
      info.add(mtx::id::packetizer, mtx::id::mpeg4_p10_video);

    else if (dmx.codec.is(codec_c::type_e::V_MPEGH_P2))
      info.add(mtx::id::packetizer, dmx.m_hevc_is_annex_b ? mtx::id::mpegh_p2_es_video : mtx::id::mpegh_p2_video);

    info.add(mtx::id::language, dmx.language.get_iso639_alpha_3_code());

    if (dmx.is_video()) {
      info.add_joined(mtx::id::pixel_dimensions, "x"s, dmx.v_width, dmx.v_height);

      if (dmx.v_color_primaries != 2)
        info.set(mtx::id::color_primaries, dmx.v_color_primaries);
      if (dmx.v_color_transfer_characteristics != 2)
        info.set(mtx::id::color_transfer_characteristics, dmx.v_color_transfer_characteristics);
      if (dmx.v_color_matrix_coefficients != 2)
        info.set(mtx::id::color_matrix_coefficients, dmx.v_color_matrix_coefficients);
      info.add(mtx::id::projection_pose_yaw, dmx.yaw);
      info.add(mtx::id::projection_pose_roll, dmx.roll);

    } else if (dmx.is_audio()) {
      info.add(mtx::id::audio_channels,           dmx.a_channels);
      info.add(mtx::id::audio_sampling_frequency, static_cast<uint64_t>(dmx.a_samplerate));
      info.add(mtx::id::audio_bits_per_sample,    dmx.a_bitdepth);
    }

    id_result_track(dmx.id,
                    dmx.is_video() ? ID_RESULT_TRACK_VIDEO : dmx.is_audio() ? ID_RESULT_TRACK_AUDIO : dmx.is_subtitles() ? ID_RESULT_TRACK_SUBTITLES : ID_RESULT_TRACK_UNKNOWN,
                    dmx.codec.get_name(dmx.fourcc.description()),
                    info.get());
  }

  for (auto &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description, attachment->id);

  if (m_chapters)
    id_result_chapters(mtx::chapters::count_atoms(*m_chapters));
}

void
qtmp4_reader_c::add_available_track_ids() {
  unsigned int i;

  for (i = 0; i < m_demuxers.size(); ++i)
    add_available_track_id(m_demuxers[i]->id);

  if (m_chapters)
    add_available_track_id(track_info_c::chapter_track_id);
}

mtx::bcp47::language_c
qtmp4_reader_c::decode_and_verify_language(uint16_t coded_language) {
  std::string language;

  for (int i = 0; 3 > i; ++i)
    language += static_cast<char>(((coded_language >> ((2 - i) * 5)) & 0x1f) + 0x60);

  return mtx::bcp47::language_c::parse(language);
}

void
qtmp4_reader_c::recode_chapter_entries(std::vector<qtmp4_chapter_entry_t> &entries) {
  if (g_identifying) {
    for (auto &entry : entries)
      entry.m_name.clear();
    return;
  }

  std::string charset              = m_ti.m_chapter_charset.empty() ? "UTF-8" : m_ti.m_chapter_charset;
  charset_converter_cptr converter = charset_converter_c::init(m_ti.m_chapter_charset);
  converter->enable_byte_order_marker_detection(true);

  if (m_debug_chapters) {
    mxdebug(fmt::format("Number of chapter entries: {0}\n", entries.size()));
    size_t num = 0;
    for (auto &entry : entries) {
      mxdebug(fmt::format("  Chapter {0}: name length {1}\n", num++, entry.m_name.length()));
      debugging_c::hexdump(entry.m_name.c_str(), entry.m_name.length());
    }
  }

  for (auto &entry : entries)
    entry.m_name = converter->utf8(entry.m_name);

  converter->enable_byte_order_marker_detection(false);
}

void
qtmp4_reader_c::detect_interleaving() {
  decltype(m_demuxers) demuxers_to_read;

  std::copy_if(m_demuxers.begin(), m_demuxers.end(), std::back_inserter(demuxers_to_read), [this](auto const &dmx) {
    return (dmx->ok && (dmx->is_audio() || dmx->is_video()) && this->demuxing_requested(dmx->type, dmx->id, dmx->language) && (dmx->sample_table.size() > 1));
  });

  if (demuxers_to_read.size() < 2) {
    mxdebug_if(m_debug_interleaving, fmt::format("Interleaving: Not enough tracks to care about interleaving.\n"));
    return;
  }

  auto cmp = [](const qt_sample_t &s1, const qt_sample_t &s2) -> uint64_t { return s1.pos < s2.pos; };

  std::list<double> gradients;
  for (auto &dmx : demuxers_to_read) {
    uint64_t min = std::min_element(dmx->sample_table.begin(), dmx->sample_table.end(), cmp)->pos;
    uint64_t max = std::max_element(dmx->sample_table.begin(), dmx->sample_table.end(), cmp)->pos;
    gradients.push_back(static_cast<double>(max - min) / m_in->get_size());

    mxdebug_if(m_debug_interleaving, fmt::format("Interleaving: Track id {0} min {1} max {2} gradient {3}\n", dmx->id, min, max, gradients.back()));
  }

  double badness = *std::max_element(gradients.begin(), gradients.end()) - *std::min_element(gradients.begin(), gradients.end());
  mxdebug_if(m_debug_interleaving, fmt::format("Interleaving: Badness: {0} ({1})\n", badness, MAX_INTERLEAVING_BADNESS < badness ? "badly interleaved" : "ok"));

  if (MAX_INTERLEAVING_BADNESS < badness)
    m_in->enable_buffering(false);
}

// ----------------------------------------------------------------------

void
qtmp4_demuxer_c::calculate_frame_rate() {
  if ((1 == durmap_table.size()) && (0 != durmap_table[0].duration) && ((0 != sample_size) || (0 == frame_offset_table.size()))) {
    // Constant frame_rate. Let's set the default duration.
    frame_rate = mtx::rational(time_scale, durmap_table[0].duration);
    mxdebug_if(m_debug_frame_rate, fmt::format("calculate_frame_rate: case 1: {0}/{1}\n", boost::multiprecision::numerator(frame_rate), boost::multiprecision::denominator(frame_rate)));

    return;
  }

  if (('v' == type) && time_scale && global_duration && (sample_table.size() < 2)) {
    frame_rate = mtx::frame_timing::determine_frame_rate(static_cast<uint64_t>(global_duration) * 1'000'000'000ull / static_cast<uint64_t>(time_scale));
    if (frame_rate)
      m_use_frame_rate_for_duration = mtx::to_int(mtx_mp_rational_t{1'000'000'000ll} / frame_rate);

    mxdebug_if(m_debug_frame_rate,
               fmt::format("calculate_frame_rate: case 2: video track with time scale {0} & duration {1} result: {2}\n",
                           time_scale, global_duration, frame_rate ? mtx::string::format_timestamp(*m_use_frame_rate_for_duration) : "<invalid>"s));

    return;
  }

  if (sample_table.size() < 2) {
    mxdebug_if(m_debug_frame_rate, fmt::format("calculate_frame_rate: case 3: sample table too small\n"));
    return;
  }

  auto max_pts = sample_table[0].pts;
  auto min_pts = max_pts;

  for (auto const &sample : sample_table) {
    max_pts = std::max(max_pts, sample.pts);
    min_pts = std::min(min_pts, sample.pts);
  }

  auto duration   = to_nsecs(max_pts - min_pts);
  auto num_frames = sample_table.size() - 1;
  frame_rate      = mtx::frame_timing::determine_frame_rate(duration / num_frames);

  if (frame_rate) {
    if ('v' == type)
      m_use_frame_rate_for_duration = mtx::to_int(mtx_mp_rational_t{1'000'000'000ll} / frame_rate);

    mxdebug_if(m_debug_frame_rate,
               fmt::format("calculate_frame_rate: case 4: duration {0} num_frames {1} frame_duration {2} frame_rate {3}/{4} use_frame_rate_for_duration {5}\n",
                           duration, num_frames, duration / num_frames, boost::multiprecision::numerator(frame_rate), boost::multiprecision::denominator(frame_rate), m_use_frame_rate_for_duration ? *m_use_frame_rate_for_duration : -1));

    return;
  }

  std::map<int64_t, int> duration_map;

  std::accumulate(sample_table.begin() + 1, sample_table.end(), sample_table[0], [&duration_map](auto const &previous_sample, auto const &current_sample) -> auto {
    duration_map[current_sample.pts - previous_sample.pts]++;
    return current_sample;
  });

  auto most_common = std::accumulate(duration_map.begin(), duration_map.end(), std::pair<int64_t, int>(*duration_map.begin()),
                                     [](auto const &winner, std::pair<int64_t, int> const &current) { return current.second > winner.second ? current : winner; });

  if (most_common.first)
    frame_rate = mtx::rational(1000000000ll, to_nsecs(most_common.first));

  mxdebug_if(m_debug_frame_rate,
             fmt::format("calculate_frame_rate: case 5: duration {0} num_frames {1} frame_duration {2} most_common.num_occurances {3} most_common.duration {4} frame_rate {5}/{6}\n",
                         duration, num_frames, duration / num_frames, most_common.second, to_nsecs(most_common.first), boost::multiprecision::numerator(frame_rate), boost::multiprecision::denominator(frame_rate)));
}

int64_t
qtmp4_demuxer_c::to_nsecs(int64_t value,
                          std::optional<int64_t> time_scale_to_use) {
  auto actual_time_scale = time_scale_to_use ? *time_scale_to_use : time_scale;
  if (!actual_time_scale)
    return 0;

  auto value_mp  = static_cast<mtx_mp_int_t>(value);
  value_mp      *= 1'000'000'000ll;
  value_mp      /= actual_time_scale;

  return mtx::to_int(value_mp);
}

void
qtmp4_demuxer_c::calculate_timestamps_constant_sample_size() {
  int const num_frame_offsets = frame_offset_table.size();
  auto chunk_index            = 0;

  timestamps.reserve(timestamps.size() + chunk_table.size());
  durations.reserve(durations.size() + chunk_table.size());
  frame_indices.reserve(frame_indices.size() + chunk_table.size());

  for (auto const &chunk : chunk_table) {
    auto frame_offset = chunk_index < num_frame_offsets ? frame_offset_table[chunk_index] : 0;

    timestamps.push_back(to_nsecs(static_cast<uint64_t>(chunk.samples) * track_duration + frame_offset));
    durations.push_back(to_nsecs(static_cast<uint64_t>(chunk.size)     * track_duration));
    frame_indices.push_back(chunk_index);

    ++chunk_index;
  }
}

void
qtmp4_demuxer_c::calculate_timestamps_variable_sample_size() {
  auto const num_frame_offsets = frame_offset_table.size();
  auto const num_samples       = sample_table.size();

  std::vector<int64_t> timestamps_before_offsets;

  timestamps.reserve(num_samples);
  timestamps_before_offsets.reserve(num_samples);
  durations.reserve(num_samples);
  frame_indices.reserve(num_samples);

  for (int frame = 0; static_cast<int>(num_samples) > frame; ++frame) {
    auto timestamp = to_nsecs(sample_table[frame].pts);

    frame_indices.push_back(frame);
    timestamps_before_offsets.push_back(timestamp);
    timestamps.push_back(timestamp + (static_cast<unsigned int>(frame) < num_frame_offsets ? to_nsecs(frame_offset_table[frame]) : 0));
  }

  int64_t avg_duration = 0, num_good_frames = 0;

  for (int frame = 0; static_cast<int>(num_samples) > (frame + 1); ++frame) {
    int64_t diff = timestamps_before_offsets[frame + 1] - timestamps_before_offsets[frame];

    if (0 >= diff)
      durations.push_back(0);
    else {
      ++num_good_frames;
      avg_duration += diff;
      durations.push_back(diff);
    }
  }

  durations.push_back(0);

  if (num_good_frames) {
    avg_duration /= num_good_frames;
    for (auto &duration : durations)
      if (!duration)
        duration = avg_duration;
  }
}

void
qtmp4_demuxer_c::calculate_timestamps() {
  if (m_timestamps_calculated)
    return;

  if (0 != sample_size)
    calculate_timestamps_constant_sample_size();
  else
    calculate_timestamps_variable_sample_size();

  if (m_debug_tables) {
    mxdebug(fmt::format("Timestamps for track ID {0}:\n", id));
    auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), timestamps.size());

    for (auto idx = 0u; idx < end; ++idx)
      mxdebug(fmt::format("  {0}: pts {1}\n", idx, mtx::string::format_timestamp(timestamps[idx])));
  }

  build_index();
  apply_edit_list();

  m_timestamps_calculated = true;
}

void
qtmp4_demuxer_c::adjust_timestamps(int64_t delta) {
  for (auto &timestamp : timestamps)
    timestamp += delta;

  for (auto &index : m_index)
    index.timestamp += delta;
}

std::optional<int64_t>
qtmp4_demuxer_c::min_timestamp()
  const {
  if (m_index.empty()) {
    return {};
  }

  return std::accumulate(m_index.begin(), m_index.end(), std::numeric_limits<int64_t>::max(), [](int64_t min, auto const &entry) { return std::min(min, entry.timestamp); });
}

bool
qtmp4_demuxer_c::update_tables() {
  if (m_tables_updated)
    return true;

  uint64_t last = chunk_table.size();

  if (!last)
    return false;

  // process chunkmap:
  size_t j, i = chunkmap_table.size();
  while (i > 0) {
    --i;
    for (j = chunkmap_table[i].first_chunk; j < last; ++j) {
      auto &chunk = chunk_table[j];
      // Don't update chunks that were added by parsing moof atoms
      // (DASH). Those have their "size" field already set. Only
      // update chunks from the "moov" atom's "stco"/"co64" atoms.
      if (chunk.size != 0)
        continue;

      chunk.desc = chunkmap_table[i].sample_description_id;
      chunk.size = chunkmap_table[i].samples_per_chunk;
    }

    last = chunkmap_table[i].first_chunk;

    if (chunk_table.size() <= last)
      break;
  }

  // calc pts of chunks:
  uint64_t s = 0;
  for (j = 0; j < chunk_table.size(); ++j) {
    chunk_table[j].samples  = s;
    s                      += chunk_table[j].size;
  }

  // Workaround for fixed-size video frames (dv and uncompressed), but
  // also for non-PCM audio with constant sample size. PCM audio will
  // be handled by the directly following code.
  if (   sample_table.empty()
      && (sample_size > 1)
      && (   ('a' != type)
          || !codec.is(codec_c::type_e::A_PCM))) {
    for (i = 0; i < s; ++i) {
      qt_sample_t sample;

      sample.size = sample_size;
      sample_table.push_back(sample);
    }

    sample_size = 0;
  }

  if (sample_table.empty()) {
    // constant sample size
    if ((1 == durmap_table.size()) || ((2 == durmap_table.size()) && (1 == durmap_table[1].number)))
      track_duration = durmap_table[0].duration;
    else
      mxerror(Y("Quicktime/MP4 reader: Constant sample size & variable duration not yet supported. Contact the author if you have such a sample file.\n"));

    m_tables_updated = true;

    return true;
  }

  // calc pts:
  auto num_samples = sample_table.size();
  s                = 0;
  uint64_t pts     = 0;

  for (j = 0; (j < durmap_table.size()) && (s < num_samples); ++j) {
    for (i = 0; (i < durmap_table[j].number) && (s < num_samples); ++i) {
      sample_table[s].pts  = pts;
      pts                 += durmap_table[j].duration;
      ++s;
    }
  }

  if (s < num_samples) {
    mxdebug_if(m_debug_headers, fmt::format("Track {0}: fewer timestamps assigned than entries in the sample table: {1} < {2}; dropping the excessive items\n", id, s, num_samples));
    sample_table.resize(s);
    num_samples = s;
  }

  // calc sample offsets
  s = 0;
  for (j = 0; (j < chunk_table.size()) && (s < num_samples); ++j) {
    uint64_t chunk_pos = chunk_table[j].pos;

    for (i = 0; (i < chunk_table[j].size) && (s < num_samples); ++i) {
      sample_table[s].pos  = chunk_pos;
      chunk_pos           += sample_table[s].size;
      ++s;
    }
  }

  // calc pts/dts offsets
  for (j = 0; j < raw_frame_offset_table.size(); ++j) {
    size_t k;

    for (k = 0; k < raw_frame_offset_table[j].count; ++k)
      frame_offset_table.push_back(raw_frame_offset_table[j].offset);
  }

  m_tables_updated = true;

  if (!m_debug_tables)
    return true;

  mxdebug(fmt::format(" Frame offset table for track ID {0}: {1} entries\n",    id, frame_offset_table.size()));
  mxdebug(fmt::format(" Sample table contents for track ID {0}: {1} entries\n", id, sample_table.size()));

  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), sample_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt::format("   {0}: pts {1} size {2} pos {3}\n", idx, sample_table[idx].pts, sample_table[idx].size, sample_table[idx].pos));

  return true;
}

void
qtmp4_demuxer_c::apply_edit_list() {
  if (editlist_table.empty())
    return;

  mxdebug_if(m_debug_editlists,
             fmt::format("Applying edit list for track {0}: {1} entries; track time scale {2}, global time scale {3}\n",
                         id, editlist_table.size(), time_scale, m_reader.m_time_scale));

  std::vector<qt_index_t> edited_index;

  auto const num_edits         = editlist_table.size();
  auto const num_index_entries = m_index.size();
  auto const index_begin       = m_index.begin();
  auto const index_end         = m_index.end();
  auto const global_time_scale = m_reader.m_time_scale;
  auto timeline_cts            = int64_t{};
  auto entry_index             = 0u;

  for (auto &edit : editlist_table) {
    auto info = fmt::format("{0} [segment_duration {1} media_time {2} media_rate {3}/{4}]", entry_index, edit.segment_duration, edit.media_time, edit.media_rate_integer, edit.media_rate_fraction);
    ++entry_index;

    if ((edit.media_rate_integer == 0) && (edit.media_rate_fraction == 0)) {
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: dwell at timeline CTS {1}; such entries are not supported yet\n", info, mtx::string::format_timestamp(timeline_cts)));
      continue;
    }

    if ((edit.media_rate_integer != 1) || (edit.media_rate_fraction != 0)) {
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: slow down/speed up at timeline CTS {1}; such entries are not supported yet\n", info, mtx::string::format_timestamp(timeline_cts)));
      continue;
    }

    if ((edit.media_time < 0) && (edit.media_time != -1)) {
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: invalid media_time (< 0 and != -1)\n", info));
      continue;
    }

    if (edit.media_time == -1) {
      timeline_cts = to_nsecs(edit.segment_duration, global_time_scale);
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: empty edit (media_time == -1); track start offset {1}\n", info, mtx::string::format_timestamp(timeline_cts)));
      continue;
    }

    if ((edit.segment_duration == 0) && (edit.media_time >= 0) && (entry_index < num_edits)) {
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: empty edit (segment_duration == 0, media_time >= 0) and more entries present; ignoring\n", info));
      continue;
    }

    if (num_edits == 1) {
      timeline_cts          = to_nsecs(edit.media_time) * -1;
      edit.media_time       = 0;
      edit.segment_duration = 0;
      mxdebug_if(m_debug_editlists, fmt::format("  {0}: single edit with positive media_time; track start offset {1}; change to non-edit to copy the rest\n", info, mtx::string::format_timestamp(timeline_cts)));

    } else if (   (num_edits   == 2)
               && (entry_index == 2)
               && (edit        == editlist_table.front())
               && m_reader.m_duration
               && ((timeline_cts - static_cast<int64_t>(*m_reader.m_duration)) >= timestamp_c::s(-60).to_ns())) {
      mxdebug_if(m_debug_editlists,
                 fmt::format("  {0}: edit list with two identical entries, each spanning the whole file; ignoring the second one; total duration: {1} timeline CTS {2}\n",
                             info, mtx::string::format_timestamp(timeline_cts), mtx::string::format_timestamp(*m_reader.m_duration)));
      continue;
    }

    // Determine first frame index whose CTS (composition timestamp)
    // is bigger than or equal to the current edit's start CTS.
    auto const edit_duration  = to_nsecs(edit.segment_duration, global_time_scale);
    auto const edit_start_cts = to_nsecs(edit.media_time);
    auto const edit_end_cts   = edit_start_cts + edit_duration;
    auto itr                  = std::find_if(m_index.begin(), m_index.end(), [edit_start_cts](auto const &entry) { return (entry.timestamp + entry.duration - (entry.duration > 0 ? 1 : 0)) >= edit_start_cts; });
    auto const frame_idx      = static_cast<uint64_t>(std::distance(index_begin, itr));

    mxdebug_if(m_debug_editlists,
               fmt::format("  {0}: normal entry; first frame {1} edit CTS {2}–{3} at timeline CTS {4}\n",
                           info, frame_idx >= num_index_entries ? -1 : frame_idx, mtx::string::format_timestamp(edit_start_cts), mtx::string::format_timestamp(edit_end_cts), mtx::string::format_timestamp(timeline_cts)));

    // Find active key frame.
    while ((itr != index_end) && (itr > index_begin) && !itr->is_keyframe) {
      --itr;
    }

    while ((itr < index_end) && (!edit_duration || (itr->timestamp < edit_end_cts))) {
      itr->timestamp = timeline_cts + itr->timestamp - edit_start_cts;
      edited_index.emplace_back(*itr);

      ++itr;
    }

    timeline_cts += edit_end_cts - edit_start_cts;
  }

  if (!edited_index.empty())
    m_index = std::move(edited_index);

  if (m_debug_editlists)
    dump_index_entries("Index after edit list");
}

void
qtmp4_demuxer_c::dump_index_entries(std::string const &message)
  const {
  mxdebug(fmt::format("{0} for track ID {1}: {2} entries\n", message, id, m_index.size()));

  auto end = std::min<int>(!m_debug_indexes_full ? 10 : std::numeric_limits<int>::max(), m_index.size());

  for (int idx = 0; idx < end; ++idx) {
    auto const &entry = m_index[idx];
    mxdebug(fmt::format("  {0}: timestamp {1} duration {2} key? {3} file_pos {4} size {5}\n", idx, mtx::string::format_timestamp(entry.timestamp), mtx::string::format_timestamp(entry.duration), entry.is_keyframe, entry.file_pos, entry.size));
  }

  if (m_debug_indexes_full)
    return;

  auto start = m_index.size() - std::min<int>(m_index.size(), 10);
  end        = m_index.size();

  for (int idx = start; idx < end; ++idx) {
    auto const &entry = m_index[idx];
    mxdebug(fmt::format("  {0}: timestamp {1} duration {2} key? {3} file_pos {4} size {5}\n", idx, mtx::string::format_timestamp(entry.timestamp), mtx::string::format_timestamp(entry.duration), entry.is_keyframe, entry.file_pos, entry.size));
  }
}

void
qtmp4_demuxer_c::build_index() {
  if (sample_size != 0)
    build_index_constant_sample_size_mode();
  else
    build_index_chunk_mode();

  mark_key_frames_from_key_frame_table();
  mark_open_gop_random_access_points_as_key_frames();

  if (m_debug_indexes)
    dump_index_entries("Index before edit list");
}

void
qtmp4_demuxer_c::build_index_constant_sample_size_mode() {
  auto is_audio              = 'a' == type;
  auto sound_stsd_atom       = reinterpret_cast<sound_v1_stsd_atom_t *>(is_audio && stsd ? stsd->get_buffer() : nullptr);
  auto v0_sample_size        = sound_stsd_atom       ? get_uint16_be(&sound_stsd_atom->v0.sample_size)        : 0;
  auto v0_audio_version      = sound_stsd_atom       ? get_uint16_be(&sound_stsd_atom->v0.version)            : 0;
  auto v1_bytes_per_frame    = 1 == v0_audio_version ? get_uint32_be(&sound_stsd_atom->v1.bytes_per_frame)    : 0;
  auto v1_samples_per_packet = 1 == v0_audio_version ? get_uint32_be(&sound_stsd_atom->v1.samples_per_packet) : 0;

  m_index.reserve(m_index.size() + chunk_table.size());

  size_t frame_idx;
  for (frame_idx = 0; frame_idx < chunk_table.size(); ++frame_idx) {
    uint64_t frame_size;

    if (1 != sample_size) {
      frame_size = chunk_table[frame_idx].size * sample_size;

    } else {
      frame_size = chunk_table[frame_idx].size;

      if (is_audio) {
        if ((0 != v1_bytes_per_frame) && (0 != v1_samples_per_packet)) {
          frame_size *= v1_bytes_per_frame;
          frame_size /= v1_samples_per_packet;
        } else
          frame_size  = frame_size * a_channels * v0_sample_size / 8;
      }
    }

    m_index.emplace_back(chunk_table[frame_idx].pos, frame_size, timestamps[frame_idx], durations[frame_idx], false);
  }
}

void
qtmp4_demuxer_c::build_index_chunk_mode() {
  m_index.reserve(m_index.size() + frame_indices.size());

  for (int frame_idx = 0, num_frames = frame_indices.size(); frame_idx < num_frames; ++frame_idx) {
    auto act_frame_idx = frame_indices[frame_idx];
    auto &sample       = sample_table[act_frame_idx];

    m_index.emplace_back(sample.pos, sample.size, timestamps[frame_idx], durations[frame_idx], false);
  }
}

void
qtmp4_demuxer_c::mark_key_frames_from_key_frame_table() {
  if (keyframe_table.empty()) {
    for (auto &index : m_index)
      index.is_keyframe = true;
    return;
  }

  auto num_index_entries = m_index.size();

  for (auto const &keyframe_number : keyframe_table)
    if ((keyframe_number > 0) && (keyframe_number <= num_index_entries))
      m_index[keyframe_number - 1].is_keyframe = true;
}

void
qtmp4_demuxer_c::mark_open_gop_random_access_points_as_key_frames() {
  // Mark samples indicated by the 'rap ' sample group to be key
  // frames, too.
  auto table_itr = sample_to_group_tables.find(fourcc_c{"rap "}.value());
  if (table_itr == sample_to_group_tables.end())
    return;

  int const num_index_entries         = m_index.size();
  auto const num_random_access_points = random_access_point_table.size();
  auto current_sample                 = 0;

  for (auto const &s2g : table_itr->second) {
    if (s2g.group_description_index && ((s2g.group_description_index - 1) < num_random_access_points)) {
      for (auto end = std::min<int>(current_sample + s2g.sample_count, num_index_entries); current_sample < end; ++current_sample)
        m_index[current_sample].is_keyframe = true;

    } else
      current_sample += s2g.sample_count;

    if (current_sample >= num_index_entries)
      return;
  }
}

memory_cptr
qtmp4_demuxer_c::read_first_bytes(int num_bytes) {
  if (!update_tables())
    return memory_cptr{};

  calculate_timestamps();

  auto buf       = memory_c::alloc(num_bytes);
  size_t buf_pos = 0;
  size_t idx_pos = 0;

  while ((0 < num_bytes) && (idx_pos < m_index.size())) {
    qt_index_t &index          = m_index[idx_pos];
    uint64_t num_bytes_to_read = std::min<int64_t>(num_bytes, index.size);

    m_reader.m_in->setFilePointer(index.file_pos);
    if (m_reader.m_in->read(buf->get_buffer() + buf_pos, num_bytes_to_read) < num_bytes_to_read)
      return memory_cptr{};

    num_bytes -= num_bytes_to_read;
    buf_pos   += num_bytes_to_read;
    ++idx_pos;
  }

  return 0 == num_bytes ? buf : memory_cptr{};
}

bool
qtmp4_demuxer_c::is_audio()
  const {
  return 'a' == type;
}

bool
qtmp4_demuxer_c::is_video()
  const {
  return 'v' == type;
}

bool
qtmp4_demuxer_c::is_subtitles()
  const {
  return 's' == type;
}

bool
qtmp4_demuxer_c::is_chapters()
  const {
  return 'C' == type;
}

bool
qtmp4_demuxer_c::is_unknown()
  const {
  return !is_audio() && !is_video() && !is_subtitles() && !is_chapters();
}

void
qtmp4_demuxer_c::set_packetizer_display_dimensions() {
  // Set the display width/height from the track header atom ('tkhd')
  // if they're set and differ from the pixel dimensions. They're
  // stored as a 32bit fix-point number. First round them to the
  // nearest integer.
  auto display_width  = (v_display_width_flt  + 0x8000) >> 16;
  auto display_height = (v_display_height_flt + 0x8000) >> 16;

  if (   (display_width  != 0)
      && (display_height != 0)
      && (   (v_width  != display_width)
          || (v_height != display_height)))
    m_reader.m_reader_packetizers[ptzr]->set_video_display_dimensions(display_width, display_height, generic_packetizer_c::ddu_pixels, option_source_e::container);
}

void
qtmp4_demuxer_c::set_packetizer_color_properties() {
  if (v_color_primaries != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_color_primaries(v_color_primaries, option_source_e::container);
  }
  if (v_color_transfer_characteristics != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_color_transfer_character(v_color_transfer_characteristics, option_source_e::container);
  }
  if (v_color_matrix_coefficients != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_color_matrix(v_color_matrix_coefficients, option_source_e::container);
  }
}

void
qtmp4_demuxer_c::set_packetizer_block_addition_mappings() {
  m_reader.m_reader_packetizers[ptzr]->set_block_addition_mappings(m_block_addition_mappings);
}

void
qtmp4_demuxer_c::handle_stsd_atom(uint64_t atom_size,
                                  int level) {
  if (is_audio()) {
    handle_audio_stsd_atom(atom_size, level);
    if ((0 < stsd_non_priv_struct_size) && (stsd_non_priv_struct_size < atom_size))
      parse_audio_header_priv_atoms(atom_size, level);

  } else if (is_video()) {
    handle_video_stsd_atom(atom_size, level);
    if ((0 < stsd_non_priv_struct_size) && (stsd_non_priv_struct_size < atom_size))
      parse_video_header_priv_atoms(atom_size, level);

  } else if (is_subtitles()) {
    handle_subtitles_stsd_atom(atom_size, level);
    if ((0 < stsd_non_priv_struct_size) && (stsd_non_priv_struct_size < atom_size))
      parse_subtitles_header_priv_atoms(atom_size, level);
  }

}

void
qtmp4_demuxer_c::handle_audio_stsd_atom(uint64_t atom_size,
                                        int level) {
  auto stsd_raw = stsd->get_buffer();
  auto size     = stsd->get_size();

  if (sizeof(sound_v0_stsd_atom_t) > atom_size)
    mxerror(fmt::format(FY("Quicktime/MP4 reader: Could not read the sound description atom for track ID {0}.\n"), id));

  sound_v1_stsd_atom_t sv1_stsd;
  sound_v2_stsd_atom_t sv2_stsd;
  memcpy(&sv1_stsd, stsd_raw, sizeof(sound_v0_stsd_atom_t));
  memcpy(&sv2_stsd, stsd_raw, sizeof(sound_v0_stsd_atom_t));

  auto this_fourcc = fourcc_c{sv1_stsd.v0.base.fourcc};

  if (fourcc && (this_fourcc != fourcc))
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track ID {0} has more than one FourCC. Only using the first one ({1}) and not this one ({2}).\n"),
                       id, fourcc.description(), this_fourcc.description()));
  else
    fourcc = this_fourcc;

  auto version = get_uint16_be(&sv1_stsd.v0.version);
  a_channels   = get_uint16_be(&sv1_stsd.v0.channels);
  a_bitdepth   = get_uint16_be(&sv1_stsd.v0.sample_size);
  auto tmp     = get_uint32_be(&sv1_stsd.v0.sample_rate);
  a_samplerate = ((tmp & 0xffff0000) >> 16) + (tmp & 0x0000ffff) / 65536.0;

  mxdebug_if(m_debug_headers,
             fmt::format("{0}[v0] FourCC: {1} channels: {2} sample size: {3} compression id: {4} sample rate: {5} version: {6}",
                         space(level * 2 + 1),
                         fourcc_c{reinterpret_cast<const uint8_t *>(sv1_stsd.v0.base.fourcc)}.description(),
                         a_channels,
                         a_bitdepth,
                         get_uint16_be(&sv1_stsd.v0.compression_id),
                         a_samplerate,
                         version));

  if (0 == version)
    stsd_non_priv_struct_size = sizeof(sound_v0_stsd_atom_t);

  else if (1 == version) {
    if (sizeof(sound_v1_stsd_atom_t) > size)
      mxerror(fmt::format(FY("Quicktime/MP4 reader: Could not read the extended sound description atom for track ID {0}.\n"), id));

    stsd_non_priv_struct_size = sizeof(sound_v1_stsd_atom_t);
    memcpy(&sv1_stsd, stsd_raw, stsd_non_priv_struct_size);

    if (m_debug_headers)
      mxinfo(fmt::format(" [v1] samples per packet: {0} bytes per packet: {1} bytes per frame: {2} bytes_per_sample: {3}",
                         get_uint32_be(&sv1_stsd.v1.samples_per_packet),
                         get_uint32_be(&sv1_stsd.v1.bytes_per_packet),
                         get_uint32_be(&sv1_stsd.v1.bytes_per_frame),
                         get_uint32_be(&sv1_stsd.v1.bytes_per_sample)));

  } else if (2 == version) {
    if (sizeof(sound_v2_stsd_atom_t) > size)
      mxerror(fmt::format(FY("Quicktime/MP4 reader: Could not read the extended sound description atom for track ID {0}.\n"), id));

    stsd_non_priv_struct_size = sizeof(sound_v2_stsd_atom_t);
    memcpy(&sv2_stsd, stsd_raw, stsd_non_priv_struct_size);

    a_channels   = get_uint32_be(&sv2_stsd.v2.channels);
    a_bitdepth   = get_uint32_be(&sv2_stsd.v2.bits_per_channel);
    a_samplerate = mtx::math::int_to_double(get_uint64_be(&sv2_stsd.v2.sample_rate));
    a_flags      = get_uint32_be(&sv2_stsd.v2.flags);


    if (m_debug_headers)
      mxinfo(fmt::format(" [v2] struct size: {0} sample rate: {1} channels: {2} const1: 0x{3:08x} bits per channel: {4} flags: {5} bytes per frame: {6} samples per frame: {7}",
                         get_uint32_be(&sv2_stsd.v2.v2_struct_size),
                         a_samplerate,
                         a_channels,
                         get_uint32_be(&sv2_stsd.v2.const1),
                         a_bitdepth,
                         a_flags,
                         get_uint32_be(&sv2_stsd.v2.bytes_per_frame),
                         get_uint32_be(&sv2_stsd.v2.samples_per_frame)));
  }

  if (m_debug_headers) {
    mxinfo(fmt::format(" non-priv struct size: {0} priv struct size: {1}", stsd_non_priv_struct_size, size - std::min<unsigned int>(size, stsd_non_priv_struct_size)));
    mxinfo("\n");
  }
}

void
qtmp4_demuxer_c::handle_video_stsd_atom(uint64_t atom_size,
                                        int level) {
  stsd_non_priv_struct_size = sizeof(video_stsd_atom_t);

  if (sizeof(video_stsd_atom_t) > atom_size)
    mxerror(fmt::format(FY("Quicktime/MP4 reader: Could not read the video description atom for track ID {0}.\n"), id));

  video_stsd_atom_t v_stsd;
  auto stsd_raw = stsd->get_buffer();
  memcpy(&v_stsd, stsd_raw, sizeof(video_stsd_atom_t));

  auto this_fourcc = fourcc_c{v_stsd.base.fourcc};

  if (fourcc && (this_fourcc != fourcc))
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track ID {0} has more than one FourCC. Only using the first one ({1}) and not this one ({2}).\n"),
                       id, fourcc.description(), this_fourcc.description()));
  else
    fourcc = this_fourcc;

  codec = codec_c::look_up(fourcc);

  mxdebug_if(m_debug_headers,
             fmt::format("{0}FourCC: {1}, width: {2}, height: {3}, depth: {4}\n",
                         space(level * 2 + 1),
                         fourcc_c{v_stsd.base.fourcc}.description(),
                         get_uint16_be(&v_stsd.width),
                         get_uint16_be(&v_stsd.height),
                         get_uint16_be(&v_stsd.depth)));

  v_width    = get_uint16_be(&v_stsd.width);
  v_height   = get_uint16_be(&v_stsd.height);
  v_bitdepth = get_uint16_be(&v_stsd.depth);
}

void
qtmp4_demuxer_c::handle_colr_atom(memory_cptr const &atom_content,
                                  int level) {
  if (sizeof(colr_atom_t) > atom_content->get_size())
    return;

  auto &colr_atom = *reinterpret_cast<colr_atom_t *>(atom_content->get_buffer());
  fourcc_c color_type{reinterpret_cast<uint8_t const *>(&colr_atom) + offsetof(colr_atom_t, color_type)};

  if (!mtx::included_in(color_type, "nclc", "nclx"))
    return;

  v_color_primaries                = get_uint16_be(&colr_atom.color_primaries);
  v_color_transfer_characteristics = get_uint16_be(&colr_atom.transfer_characteristics);
  v_color_matrix_coefficients      = get_uint16_be(&colr_atom.matrix_coefficients);

  mxdebug_if(m_debug_headers,
             fmt::format("{0}color primaries: {1}, transfer characteristics: {2}, matrix coefficients: {3}\n",
                         space(level * 2 + 1), v_color_primaries, v_color_transfer_characteristics, v_color_matrix_coefficients));
}

void
qtmp4_demuxer_c::handle_subtitles_stsd_atom(uint64_t atom_size,
                                            int level) {
  auto stsd_raw = stsd->get_buffer();
  auto size     = stsd->get_size();

  if (sizeof(base_stsd_atom_t) > atom_size)
    return;

  base_stsd_atom_t base_stsd;
  memcpy(&base_stsd, stsd_raw, sizeof(base_stsd_atom_t));

 fourcc                    = fourcc_c{base_stsd.fourcc};
 stsd_non_priv_struct_size = sizeof(base_stsd_atom_t);

  if (m_debug_headers) {
    mxdebug(fmt::format("{0}FourCC: {1}\n", space(level * 2 + 1), fourcc.description()));
    debugging_c::hexdump(stsd_raw, size);
  }
}

void
qtmp4_demuxer_c::parse_aac_esds_decoder_config() {
  if (!esds.decoder_config || (2 > esds.decoder_config->get_size())) {
    a_aac_audio_config                     = mtx::aac::audio_config_t{};
    a_aac_audio_config->profile            = mtx::aac::PROFILE_MAIN;
    a_aac_audio_config->sample_rate        = a_samplerate;
    a_aac_audio_config->output_sample_rate = a_samplerate;
    a_aac_audio_config->channels           = a_channels;
    esds.decoder_config                    = mtx::aac::create_audio_specific_config(*a_aac_audio_config);

    mxdebug_if(m_debug_headers,
               fmt::format(" AAC: no decoder config in ESDS; generating default with profile: {0}, sample_rate: {1}, channels: {2}\n",
                           a_aac_audio_config->profile, a_aac_audio_config->sample_rate, a_aac_audio_config->channels));

    return;
  }

  a_aac_audio_config = mtx::aac::parse_audio_specific_config(esds.decoder_config->get_buffer(), esds.decoder_config->get_size());
  if (!a_aac_audio_config) {
    mxwarn(fmt::format(FY("Track {0}: The AAC information could not be parsed.\n"), id));
    return;
  }

  mxdebug_if(m_debug_headers,
             fmt::format(" AAC audio-specific config: profile: {0}, sample_rate: {1}, channels: {2}, output_sample_rate: {3}, sbr: {4}\n",
                         a_aac_audio_config->profile, a_aac_audio_config->sample_rate, a_aac_audio_config->channels, a_aac_audio_config->output_sample_rate, a_aac_audio_config->sbr));

  if ((a_channels != 8) || (a_aac_audio_config->channels != 7))
    a_channels = a_aac_audio_config->channels;
  a_samplerate = a_aac_audio_config->sample_rate;
}

void
qtmp4_demuxer_c::parse_vorbis_esds_decoder_config() {
  if (!esds.decoder_config || !esds.decoder_config->get_size())
    return;

  try {
    codec = codec_c::look_up(codec_c::type_e::A_VORBIS);
    priv  = unlace_memory_xiph(esds.decoder_config);

  } catch (mtx::mem::lacing_x &) {
  }
}

void
qtmp4_demuxer_c::parse_esds_audio_header_priv_atom(mm_io_c &io,
                                                   int level) {
  esds_parsed = parse_esds_atom(io, level);

  if (!esds_parsed)
    return;

  if (codec_c::look_up_object_type_id(esds.object_type_id).is(codec_c::type_e::A_AAC))
    parse_aac_esds_decoder_config();

  else if (codec_c::look_up_object_type_id(esds.object_type_id).is(codec_c::type_e::A_VORBIS))
    parse_vorbis_esds_decoder_config();
}

void
qtmp4_demuxer_c::parse_dops_audio_header_priv_atom(mm_io_c &io,
                                                   int level) {
  mxdebug_if(m_debug_headers, fmt::format("{0}Opus box size: {1}\n", space((level + 1) * 2 + 1), io.get_size()));

  if (io.get_size() < 11)
    return;

  auto opus_priv     = memory_c::alloc(io.get_size() + 8);
  auto opus_priv_ptr = opus_priv->get_buffer();

  std::memcpy(opus_priv_ptr, "OpusHead", 8);

  if (io.read(&opus_priv_ptr[8], io.get_size()) != static_cast<uint64_t>(io.get_size())) {
    mxdebug_if(m_debug_headers, fmt::format("{0}Opus box read failure", space((level + 1) * 2 + 1)));
    return;
  }

  // Data in MP4 is stored in Big Endian while Opus-in-Ogg and
  // therefore Opus-in-Matroska uses Little Endian in its ID header.
  put_uint16_le(&opus_priv_ptr[10], get_uint16_be(&opus_priv_ptr[10])); // pre-skip
  put_uint32_le(&opus_priv_ptr[12], get_uint32_be(&opus_priv_ptr[12])); // input sample rate
  put_uint16_le(&opus_priv_ptr[16], get_uint16_be(&opus_priv_ptr[16])); // output gain

  priv.push_back(opus_priv);

  a_bitdepth = 0;
}

void
qtmp4_demuxer_c::parse_dfla_audio_header_priv_atom(mm_io_c &io,
                                                   int level) {
  auto size = io.get_size();

  mxdebug_if(m_debug_headers, fmt::format("{0}FLAC box size: {1}\n", space((level + 1) * 2 + 1), size));

  if (size <= 4)
    return;

  io.skip(4);
  size -= 4;

  auto flac_priv = memory_c::alloc(size);
  if (io.read(flac_priv, size) != size)
    return;

  priv.clear();
  priv.emplace_back(flac_priv);

  mxdebug_if(m_debug_headers, fmt::format("{0}FLAC private data: {1}\n", space((level + 1) * 2 + 1), mtx::string::to_hex(priv.at(0))));
}

void
qtmp4_demuxer_c::parse_audio_header_priv_atoms(uint64_t atom_size,
                                               int level) {
  auto mem  = stsd->get_buffer() + stsd_non_priv_struct_size;
  auto size = atom_size - stsd_non_priv_struct_size;

  mm_mem_io_c mio(mem, size);

  parse_audio_header_priv_atoms(mio, size, level);
}

void
qtmp4_demuxer_c::parse_audio_header_priv_atoms(mm_mem_io_c &mio,
                                               uint64_t size,
                                               int level) {
  if (size < 8)
    return;

  try {
    while (!mio.eof() && (mio.getFilePointer() < (size - 8))) {
      qt_atom_t atom;

      try {
        atom = read_qtmp4_atom(&mio, false);
      } catch (...) {
        return;
      }

      mxdebug_if(m_debug_headers, fmt::format("{0}Audio private data size: {1}, type: '{2}'\n", space((level + 1) * 2 + 1), atom.size, atom.fourcc));

      mm_mem_io_c sub_io{mio.get_ro_buffer() + atom.pos + atom.hsize, std::min<uint64_t>(atom.size - atom.hsize, size - atom.pos - atom.hsize)};

      if ((atom.fourcc == "esds") && !esds_parsed)
        parse_esds_audio_header_priv_atom(sub_io, level + 1);

      else if (atom.fourcc == "wave")
        parse_audio_header_priv_atoms(sub_io, sub_io.get_size(), level + 1);

      else if (atom.fourcc == "dOps")
        parse_dops_audio_header_priv_atom(sub_io, level + 1);

      else if (atom.fourcc == "dfLa")
        parse_dfla_audio_header_priv_atom(sub_io, level + 1);

      mio.setFilePointer(atom.pos + atom.size);
    }
  } catch(...) {
  }
}

void
qtmp4_demuxer_c::add_data_as_block_addition(uint32_t atom_type,
                                            memory_cptr const &data) {
  block_addition_mapping_t mapping;

  mapping.id_type       = atom_type;
  mapping.id_extra_data = data;

  m_block_addition_mappings.emplace_back(mapping);
}

void
qtmp4_demuxer_c::parse_video_header_priv_atoms(uint64_t atom_size,
                                               int level) {
  auto mem  = stsd->get_buffer() + stsd_non_priv_struct_size;
  auto size = atom_size - stsd_non_priv_struct_size;

  if (   !codec.is(codec_c::type_e::V_MPEG4_P10)
      && !codec.is(codec_c::type_e::V_MPEGH_P2)
      && !codec.is(codec_c::type_e::V_AV1)
      && size
      && !fourcc.equiv("mp4v")
      && !fourcc.equiv("xvid")) {
    priv.clear();
    priv.emplace_back(memory_c::clone(mem, size));
    return;
  }

  mm_mem_io_c mio(mem, size);

  try {
    while (!mio.eof() && (mio.getFilePointer() < size)) {
      qt_atom_t atom;

      try {
        atom = read_qtmp4_atom(&mio);
      } catch (...) {
        return;
      }

      mxdebug_if(m_debug_headers, fmt::format("{0}Video private data size: {1}, type: '{2}'\n", space((level + 1) * 2 + 1), atom.size, atom.fourcc));

      if (mtx::included_in(atom.fourcc, "esds", "avcC", "hvcC", "av1C")) {
        auto atom_data = memory_c::alloc(atom.size - atom.hsize);

        if (mio.read(atom_data->get_buffer(), atom_data->get_size()) != atom_data->get_size())
          return;

        if (priv.empty())
          priv.emplace_back(atom_data);

        if ((atom.fourcc == "esds") && !esds_parsed) {
          mm_mem_io_c memio{*atom_data};
          esds_parsed = parse_esds_atom(memio, level + 1);
        }

      } else if (atom.fourcc == "colr")
        handle_colr_atom(mio.read(atom.size - atom.hsize), level + 2);

      else if (mtx::included_in(atom.fourcc, "dvcC", "dvvC", "hvcE"))
        add_data_as_block_addition(atom.fourcc.value(), mio.read(atom.size - atom.hsize));

      mio.setFilePointer(atom.pos + atom.size);
    }
  } catch(...) {
  }
}

void
qtmp4_demuxer_c::parse_subtitles_header_priv_atoms(uint64_t atom_size,
                                                   int level) {
  auto mem  = stsd->get_buffer() + stsd_non_priv_struct_size;
  auto size = atom_size - stsd_non_priv_struct_size;

  if (!fourcc.equiv("mp4s")) {
    priv.clear();
    priv.emplace_back(memory_c::clone(mem, size));
    return;
  }

  mm_mem_io_c mio(mem, size);

  try {
    while (!mio.eof() && (mio.getFilePointer() < size)) {
      qt_atom_t atom;

      try {
        atom = read_qtmp4_atom(&mio);
      } catch (...) {
        return;
      }

      mxdebug_if(m_debug_headers, fmt::format("{0}Subtitles private data size: {1}, type: '{2}'\n", space((level + 1) * 2 + 1), atom.size, atom.fourcc));

      if (atom.fourcc == "esds") {
        if (priv.empty()) {
          priv.emplace_back(memory_c::alloc(atom.size - atom.hsize));
          if (mio.read(priv[0], priv[0]->get_size()) != priv[0]->get_size()) {
            priv.clear();
            return;
          }
        }

        if (!esds_parsed) {
          mm_mem_io_c memio(priv[0]->get_buffer(), priv[0]->get_size());
          esds_parsed = parse_esds_atom(memio, level + 1);
        }
      }

      mio.setFilePointer(atom.pos + atom.size);
    }
  } catch(...) {
  }

  mxdebug_if(m_debug_headers, fmt::format("{0}Decoder config data at end of parsing: {1}\n", space((level + 1) * 2 + 1), esds.decoder_config ? esds.decoder_config->get_size() : 0));
}

bool
qtmp4_demuxer_c::parse_esds_atom(mm_io_c &io,
                                 int level) {
  int lsp      = (level + 1) * 2;
  esds.version = io.read_uint8();
  esds.flags   = io.read_uint24_be();
  auto tag     = io.read_uint8();

  mxdebug_if(m_debug_headers, fmt::format("{0}esds: version: {1}, flags: {2}\n", space(lsp + 1), static_cast<unsigned int>(esds.version), esds.flags));

  if (mtx::mp4::DESCRIPTOR_TYPE_ES == tag) {
    auto len             = io.read_mp4_descriptor_len();
    esds.esid            = io.read_uint16_be();
    esds.stream_priority = io.read_uint8();
    mxdebug_if(m_debug_headers, fmt::format("{0}esds: id: {1}, stream priority: {2}, len: {3}\n", space(lsp + 1), static_cast<unsigned int>(esds.esid), static_cast<unsigned int>(esds.stream_priority), len));

  } else {
    esds.esid = io.read_uint16_be();
    mxdebug_if(m_debug_headers, fmt::format("{0}esds: id: {1}\n", space(lsp + 1), static_cast<unsigned int>(esds.esid)));
  }

  tag = io.read_uint8();
  mxdebug_if(m_debug_headers, fmt::format("{0}tag is 0x{1:02x} ({2}).\n", space(lsp + 1), static_cast<unsigned int>(tag), displayable_esds_tag_name(tag)));

  if (mtx::mp4::DESCRIPTOR_TYPE_DEC_CONFIG != tag)
    return false;

  auto len                = io.read_mp4_descriptor_len();

  esds.object_type_id     = io.read_uint8();
  esds.stream_type        = io.read_uint8();
  esds.buffer_size_db     = io.read_uint24_be();
  esds.max_bitrate        = io.read_uint32_be();
  esds.avg_bitrate        = io.read_uint32_be();
  esds.decoder_config.reset();

  mxdebug_if(m_debug_headers,
             fmt::format("{0}esds: decoder config descriptor, len: {1}, object_type_id: {2}, "
                         "stream_type: 0x{3:2x}, buffer_size_db: {4}, max_bitrate: {5:.3f}kbit/s, avg_bitrate: {6:.3f}kbit/s\n",
                         space(lsp + 1),
                         len,
                         static_cast<unsigned int>(esds.object_type_id),
                         static_cast<unsigned int>(esds.stream_type),
                         esds.buffer_size_db,
                         esds.max_bitrate / 1000.0,
                         esds.avg_bitrate / 1000.0));

  tag = io.read_uint8();
  mxdebug_if(m_debug_headers, fmt::format("{0}tag is 0x{1:02x} ({2}).\n", space(lsp + 1), static_cast<unsigned int>(tag), displayable_esds_tag_name(tag)));

  if (mtx::mp4::DESCRIPTOR_TYPE_DEC_SPECIFIC == tag) {
    len = io.read_mp4_descriptor_len();
    if (!len)
      throw mtx::input::header_parsing_x();

    esds.decoder_config = memory_c::alloc(len);
    if (io.read(esds.decoder_config, len) != len) {
      esds.decoder_config.reset();
      throw mtx::input::header_parsing_x();
    }

    tag = io.read_uint8();

    if (m_debug_headers) {
      mxdebug(fmt::format("{0}esds: decoder specific descriptor, len: {1}\n", space(lsp + 1), len));
      mxdebug(fmt::format("{0}esds: dumping decoder specific descriptor\n", space(lsp + 3)));
      debugging_c::hexdump(esds.decoder_config->get_buffer(), esds.decoder_config->get_size());
      mxdebug(fmt::format("{0}tag is 0x{1:02x} ({2}).\n", space(lsp + 1), static_cast<unsigned int>(tag), displayable_esds_tag_name(tag)));
    }

  }

  if (mtx::mp4::DESCRIPTOR_TYPE_SL_CONFIG == tag) {
    len = io.read_mp4_descriptor_len();
    if (!len)
      throw mtx::input::header_parsing_x{};

    esds.sl_config     = memory_c::alloc(len);
    if (io.read(esds.sl_config, len) != len) {
      esds.sl_config.reset();
      throw mtx::input::header_parsing_x();
    }

    mxdebug_if(m_debug_headers, fmt::format("{0}esds: sync layer config descriptor, len: {1}\n", space(lsp + 1), len));

  }

  return true;
}

void
qtmp4_demuxer_c::derive_track_params_from_ac3_audio_bitstream() {
  auto buf    = read_first_bytes(64);
  auto header = mtx::ac3::frame_c{};

  if (!buf || (-1 == header.find_in(buf)))
    return;

  a_channels   = header.m_channels;
  a_samplerate = header.m_sample_rate;
  codec        = header.get_codec();
}

void
qtmp4_demuxer_c::derive_track_params_from_dts_audio_bitstream() {
  auto buf    = read_first_bytes(16384);
  auto header = mtx::dts::header_t{};

  if (!buf || (-1 == mtx::dts::find_header(buf->get_buffer(), buf->get_size(), header, false)))
    return;

  a_channels   = header.get_total_num_audio_channels();
  a_samplerate = header.get_effective_sampling_frequency();
  a_bitdepth   = std::max(header.source_pcm_resolution, 0);
}

void
qtmp4_demuxer_c::derive_track_params_from_mp3_audio_bitstream() {
  auto buf = read_first_bytes(64);
  if (!buf)
    return;

  mp3_header_t header;
  auto offset = find_mp3_header(buf->get_buffer(), buf->get_size());
  if ((-1 == offset) || !decode_mp3_header(&buf->get_buffer()[offset], &header))
    return;

  a_channels   = header.channels;
  a_samplerate = header.sampling_frequency;
  codec        = header.get_codec();
}

bool
qtmp4_demuxer_c::derive_track_params_from_opus_private_data() {
  if (priv.empty() || !priv[0]) {
    mxdebug_if(m_debug_headers, "derive_opus: no private data\n");
    return false;
  }

  if (priv[0]->get_size() < 19) {
    mxdebug_if(m_debug_headers, fmt::format("derive_opus: private data too small: {} < 19\n", priv[0]->get_size()));
    return false;
  }

  mxdebug_if(m_debug_headers, fmt::format("derive_opus: OK\n"));

  return true;
}

bool
qtmp4_demuxer_c::derive_track_params_from_vorbis_private_data() {
  if (priv.size() != 3)
    return false;

  ogg_packet ogg_headers[3];
  memset(ogg_headers, 0, 3 * sizeof(ogg_packet));
  ogg_headers[0].packet   = priv[0]->get_buffer();
  ogg_headers[1].packet   = priv[1]->get_buffer();
  ogg_headers[2].packet   = priv[2]->get_buffer();
  ogg_headers[0].bytes    = priv[0]->get_size();
  ogg_headers[1].bytes    = priv[1]->get_size();
  ogg_headers[2].bytes    = priv[2]->get_size();
  ogg_headers[0].b_o_s    = 1;
  ogg_headers[1].packetno = 1;
  ogg_headers[2].packetno = 2;
  auto derived            = false;

  vorbis_info vi;
  vorbis_comment vc;

  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  try {
    for (int i = 0; 3 > i; ++i)
      if (vorbis_synthesis_headerin(&vi, &vc, &ogg_headers[i]) < 0)
        throw false;

    a_channels   = vi.channels;
    a_samplerate = vi.rate;
    a_bitdepth   = 0;
    derived      = true;

  } catch (bool) {
  }

  vorbis_info_clear(&vi);
  vorbis_comment_clear(&vc);

  return derived;
}

void
qtmp4_demuxer_c::check_for_hevc_video_annex_b_bitstream() {
  auto buf = read_first_bytes(4);
  if (!buf || (buf->get_size() < 4))
    return;

  auto value = get_uint32_be(buf->get_buffer());
  if (value != 0x00000001)
    return;

  auto probe_size = 128 * 1024;
  while (probe_size <= (1024 * 1024)) {
    mtx::hevc::es_parser_c parser;

    buf = read_first_bytes(probe_size);
    if (!buf)
      return;

    parser.add_bytes(buf);
    if (!parser.headers_parsed()) {
      probe_size *= 2;
      continue;
    }

    priv.clear();
    priv.emplace_back(parser.get_configuration_record());

    break;
  }

  m_hevc_is_annex_b = true;
}

bool
qtmp4_demuxer_c::verify_audio_parameters() {
  if (codec.is(codec_c::type_e::A_MP2) || codec.is(codec_c::type_e::A_MP3))
    derive_track_params_from_mp3_audio_bitstream();

  else if (codec.is(codec_c::type_e::A_AC3))
    derive_track_params_from_ac3_audio_bitstream();

  else if (codec.is(codec_c::type_e::A_DTS))
    derive_track_params_from_dts_audio_bitstream();

#if defined(HAVE_FLAC_FORMAT_H)
  else if (codec.is(codec_c::type_e::A_FLAC))
    return priv.size() == 1;
#endif

  else if (codec.is(codec_c::type_e::A_VORBIS))
    return derive_track_params_from_vorbis_private_data();

  else if (codec.is(codec_c::type_e::A_OPUS))
    return derive_track_params_from_opus_private_data();

  else if (codec.is(codec_c::type_e::A_PCM)) {
    if (fourcc == "in24")
      a_bitdepth = 24;
  }

  if ((0 == a_channels) || (0.0 == a_samplerate)) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track {0} is missing some data. Broken header atoms?\n"), id));
    return false;
  }

  if (fourcc.equiv("MP4A") && !verify_mp4a_audio_parameters())
    return false;

  if (codec.is(codec_c::type_e::A_ALAC))
    return verify_alac_audio_parameters();

  if (codec.is(codec_c::type_e::A_DTS))
    return verify_dts_audio_parameters();

  return true;
}

bool
qtmp4_demuxer_c::verify_alac_audio_parameters() {
  if (!stsd || (stsd->get_size() < (stsd_non_priv_struct_size + 12 + sizeof(mtx::alac::codec_config_t)))) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track {0} is missing some data. Broken header atoms?\n"), id));
    return false;
  }

  auto codec_config = reinterpret_cast<mtx::alac::codec_config_t *>(stsd->get_buffer() + stsd_non_priv_struct_size + 12);
  a_channels        = codec_config->num_channels;
  a_bitdepth        = codec_config->bit_depth;
  a_samplerate      = get_uint32_be(&codec_config->sample_rate);

  return true;
}

bool
qtmp4_demuxer_c::verify_dts_audio_parameters() {
  auto buf = read_first_bytes(16384);

  if (!buf || (-1 == mtx::dts::find_header(buf->get_buffer(), buf->get_size(), m_dts_header, false))) {
    mxwarn_tid(m_reader.m_ti.m_fname, id, Y("No DTS header found in first frames; track will be skipped.\n"));
    return false;
  }

  codec.set_specialization(m_dts_header.get_codec_specialization());

  return true;
}

bool
qtmp4_demuxer_c::verify_mp4a_audio_parameters() {
  auto cdc = codec_c::look_up_object_type_id(esds.object_type_id);
  if (!cdc.is(codec_c::type_e::A_AAC) && !cdc.is(codec_c::type_e::A_DTS) && !cdc.is(codec_c::type_e::A_MP2) && !cdc.is(codec_c::type_e::A_MP3)) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: The audio track {0} is using an unsupported 'object type id' of {1} in the 'esds' atom. Skipping this track.\n"), id, static_cast<unsigned int>(esds.object_type_id)));
    return false;
  }

  if (cdc.is(codec_c::type_e::A_AAC) && (!esds.decoder_config || !a_aac_audio_config)) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: The AAC track {0} is missing the esds atom/the decoder config. Skipping this track.\n"), id));
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_video_parameters() {
  if (codec.is(codec_c::type_e::V_MPEGH_P2))
    check_for_hevc_video_annex_b_bitstream();

  if (!v_width || !v_height || !fourcc) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track {0} is missing some data. Broken header atoms?\n"), id));
    return false;
  }

  if (fourcc.equiv("mp4v"))
    return verify_mp4v_video_parameters();

  if (codec.is(codec_c::type_e::V_MPEG4_P10))
    return verify_avc_video_parameters();

  else if (codec.is(codec_c::type_e::V_MPEGH_P2))
    return verify_hevc_video_parameters();

  return true;
}

bool
qtmp4_demuxer_c::derive_track_params_from_avc_bitstream() {
  priv.clear();

  // No avcC content? Try to build one from the first frames.
  auto mem = read_first_bytes(10'000);

  mxdebug_if(m_debug_headers, fmt::format("derive_track_params_from_avc_bitstream: deriving avcC from bitstream; read {0} bytes\n", mem ? mem->get_size() : 0));

  if (!mem)
    return false;

  mtx::avc::es_parser_c parser;

  parser.add_bytes(mem->get_buffer(), mem->get_size());
  parser.flush();

  if (parser.headers_parsed())
    priv.emplace_back(parser.get_configuration_record());

  mxdebug_if(m_debug_headers, fmt::format("derive_track_params_from_avc_bitstream: avcC derived? size {0} bytes\n", !priv.empty() && priv[0] ? priv[0]->get_size() : 0));

  return !priv.empty() && priv[0]->get_size();
}

bool
qtmp4_demuxer_c::verify_avc_video_parameters() {
  if (!priv.empty() && (4 <= priv[0]->get_size()))
    return true;

  if (derive_track_params_from_avc_bitstream())
    return true;

  mxwarn(fmt::format(FY("Quicktime/MP4 reader: MPEG4 part 10/AVC track {0} is missing its decoder config. Skipping this track.\n"), id));
  return false;
}

bool
qtmp4_demuxer_c::verify_hevc_video_parameters() {
  if (priv.empty() || (23 > priv[0]->get_size())) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: MPEGH part 2/HEVC track {0} is missing its decoder config. Skipping this track.\n"), id));
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_mp4v_video_parameters() {
  if (!esds_parsed) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: The video track {0} is missing the ESDS atom. Skipping this track.\n"), id));
    return false;
  }

  if (codec.is(codec_c::type_e::V_MPEG4_P2) && !esds.decoder_config) {
    // This is MPEG4 video, and we need header data for it.
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: MPEG4 track {0} is missing the esds atom/the decoder config. Skipping this track.\n"), id));
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_subtitles_parameters() {
  if (codec.is(codec_c::type_e::S_VOBSUB))
    return verify_vobsub_subtitles_parameters();

  else if (codec.is(codec_c::type_e::S_TX3G))
    return true;

  return false;
}

bool
qtmp4_demuxer_c::verify_vobsub_subtitles_parameters() {
  if (!esds.decoder_config || (64 > esds.decoder_config->get_size())) {
    mxwarn(fmt::format(FY("Quicktime/MP4 reader: Track {0} is missing some data. Broken header atoms?\n"), id));
    return false;
  }

  return true;
}

void
qtmp4_demuxer_c::determine_codec() {
  codec = codec_c::look_up_object_type_id(esds.object_type_id);

  if (codec)
    return;

  codec = codec_c::look_up(fourcc);

  if (codec.is(codec_c::type_e::A_PCM))
    m_pcm_format = fourcc == "twos" ? pcm_packetizer_c::big_endian_integer
                 : fourcc != "lpcm" ? pcm_packetizer_c::little_endian_integer
                 : a_flags & 0x01   ? pcm_packetizer_c::ieee_float
                 : a_flags & 0x02   ? pcm_packetizer_c::big_endian_integer
                 :                    pcm_packetizer_c::little_endian_integer;
}

void
qtmp4_demuxer_c::process(packet_cptr const &packet) {
  if (!m_converter || !m_converter->convert(packet))
    m_reader.m_reader_packetizers[ptzr]->process(packet);
}
