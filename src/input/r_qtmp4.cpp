/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

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

#include "avilib.h"
#include "common/aac.h"
#include "common/alac.h"
#include "common/chapters/chapters.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/frame_timing.h"
#include "common/hacks.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mp3.h"
#include "common/mp4.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/vobsub.h"
#include "input/r_qtmp4.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_alac.h"
#include "output/p_avc.h"
#include "output/p_dts.h"
#include "output/p_hevc.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_mpeg4_p2.h"
#include "output/p_passthrough.h"
#include "output/p_pcm.h"
#include "output/p_quicktime.h"
#include "output/p_video_for_windows.h"
#include "output/p_vobsub.h"
#include "output/p_vorbis.h"

using namespace libmatroska;

#define MAX_INTERLEAVING_BADNESS 0.4

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
    return (boost::format("invalid atom chunk size %1% at %2%") % m_size % m_pos).str();
  }
};

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
      mxerror(boost::format(Y("Quicktime/MP4 reader: Invalid chunk size %1% at %2%.\n")) % a.size % a.pos);
    else
      throw mtx::atom_chunk_size_x{a.size, a.pos};
  }

  return a;
}

int
qtmp4_reader_c::probe_file(mm_io_c *in,
                           uint64_t) {
  try {
    in->setFilePointer(0, seek_beginning);

    while (1) {
      uint64_t atom_pos  = in->getFilePointer();
      uint64_t atom_size = in->read_uint32_be();
      fourcc_c atom(in);

      if (1 == atom_size)
        atom_size = in->read_uint64_be();

      mxverb(3, boost::format("Quicktime/MP4 reader: Atom: '%1%': %2%\n") % atom % atom_size);

      if (   (atom == "moov")
          || (atom == "ftyp")
          || (atom == "mdat")
          || (atom == "pnot"))
        return 1;

      if ((atom != "wide") && (atom != "skip"))
        return 0;

      in->setFilePointer(atom_pos + atom_size);
    }

  } catch (...) {
  }

  return 0;
}

qtmp4_reader_c::qtmp4_reader_c(const track_info_c &ti,
                               const mm_io_cptr &in)
  : generic_reader_c(ti, in)
  , m_time_scale(1)
  , m_compression_algorithm{}
  , m_main_dmx(-1)
  , m_audio_encoder_delay_samples(0)
  , m_moof_offset{}
  , m_fragment_implicit_offset{}
  , m_fragment{}
  , m_track_for_fragment{}
  , m_timestamps_calculated{}
  , m_debug_chapters{    "qtmp4|qtmp4_full|qtmp4_chapters"}
  , m_debug_headers{     "qtmp4|qtmp4_full|qtmp4_headers"}
  , m_debug_tables{            "qtmp4_full|qtmp4_tables|qtmp4_tables_full"}
  , m_debug_tables_full{                               "qtmp4_tables_full"}
  , m_debug_interleaving{"qtmp4|qtmp4_full|qtmp4_interleaving"}
  , m_debug_resync{      "qtmp4|qtmp4_full|qtmp4_resync"}
{
}

void
qtmp4_reader_c::read_headers() {
  try {
    if (!qtmp4_reader_c::probe_file(m_in.get(), m_size))
      throw mtx::input::invalid_format_x();

    show_demuxer_info();

    parse_headers();

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }
}

qtmp4_reader_c::~qtmp4_reader_c() {
}

qt_atom_t
qtmp4_reader_c::read_atom(mm_io_c *read_from,
                          bool exit_on_error) {
  return read_qtmp4_atom(read_from ? read_from : m_in.get(), exit_on_error);
}

#define skip_atom() m_in->setFilePointer(atom.pos + atom.size)

bool
qtmp4_reader_c::resync_to_top_level_atom(uint64_t start_pos) {
  static std::vector<std::string> const s_top_level_atoms{ "ftyp", "pdin", "moov", "moof", "mfra", "mdat", "free", "skip" };
  static auto test_atom_at = [this](uint64_t atom_pos, uint64_t expected_hsize, fourcc_c const &expected_fourcc) -> bool {
    m_in->setFilePointer(atom_pos);
    auto test_atom = read_atom(nullptr, false);
    mxdebug_if(m_debug_resync, boost::format("Test for %1%bit offset atom: %2%\n") % (8 == expected_hsize ? 32 : 64) % test_atom);

    if ((test_atom.fourcc == expected_fourcc) && (test_atom.hsize == expected_hsize) && ((test_atom.pos + test_atom.size) <= m_size)) {
      mxdebug_if(m_debug_resync, boost::format("%1%bit offset atom looks good\n") % (8 == expected_hsize ? 32 : 64));
      m_in->setFilePointer(atom_pos);
      return true;
    }

    return false;
  };

  try {
    m_in->setFilePointer(start_pos);
    fourcc_c fourcc{m_in};
    auto next_pos = start_pos;

    mxdebug_if(m_debug_resync, boost::format("Starting resync at %1%, FourCC %2%\n") % start_pos % fourcc);
    while (true) {
      m_in->setFilePointer(next_pos);
      fourcc.shift_read(m_in);
      next_pos = m_in->getFilePointer();

      if (!fourcc.human_readable() || !fourcc.equiv(s_top_level_atoms))
        continue;

      auto fourcc_pos = m_in->getFilePointer() - 4;
      mxdebug_if(m_debug_resync, boost::format("Human readable at %1%: %2%\n") % fourcc_pos % fourcc);

      if (test_atom_at(fourcc_pos - 12, 16, fourcc) || test_atom_at(fourcc_pos - 4, 8, fourcc))
        return true;
    }

  } catch (mtx::mm_io::exception &ex) {
    mxdebug_if(m_debug_resync, boost::format("I/O exception during resync: %1%\n") % ex.what());
  } catch (mtx::atom_chunk_size_x &ex) {
    mxdebug_if(m_debug_resync, boost::format("Atom exception during resync: %1%\n") % ex.error());
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
      mxdebug_if(m_debug_headers, boost::format("'%1%' atom, size %2%, at %3%–%4%, human readable? %5%\n") % atom.fourcc % atom.size % atom.pos % (atom.pos + atom.size) % atom.fourcc.human_readable());

      if (atom.fourcc == "ftyp") {
        auto tmp = fourcc_c{m_in};
        mxdebug_if(m_debug_headers, boost::format("  File type major brand: %1%\n") % tmp);
        tmp = fourcc_c{m_in};
        mxdebug_if(m_debug_headers, boost::format("  File type minor brand: %1%\n") % tmp);

        for (idx = 0; idx < ((atom.size - 16) / 4); ++idx) {
          tmp = fourcc_c{m_in};
          mxdebug_if(m_debug_headers, boost::format("  File type compatible brands #%1%: %2%\n") % idx % tmp);
        }

      } else if (atom.fourcc == "moov") {
        if (!headers_parsed)
          handle_moov_atom(atom.to_parent(), 0);
        else
          skip_atom();
        headers_parsed = true;

      } else if (atom.fourcc == "mdat") {
        skip_atom();
        mdat_found = true;

      } else if (atom.fourcc == "moof") {
        handle_moof_atom(atom.to_parent(), 0, atom);

      } else if (atom.fourcc.human_readable())
        skip_atom();

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

  brng::remove_erase_if(m_demuxers, [](qtmp4_demuxer_cptr const &dmx) { return !dmx->ok || dmx->is_chapters(); });

  detect_interleaving();

  if (!g_identifying)
    calculate_timestamps();

  mxdebug_if(m_debug_headers, boost::format("Number of valid tracks found: %1%\n") % m_demuxers.size());
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

boost::optional<int64_t>
qtmp4_reader_c::calculate_global_min_timestamp()
  const {
  boost::optional<int64_t> global_min_timestamp;

  for (auto const &dmx : m_demuxers) {
    if (!demuxing_requested(dmx->type, dmx->id, dmx->language)) {
      continue;
    }

    auto dmx_min_timestamp = dmx->min_timestamp();
    if (dmx_min_timestamp && (!global_min_timestamp || (dmx_min_timestamp < *global_min_timestamp))) {
      global_min_timestamp.reset(*dmx_min_timestamp);
    }

    mxdebug_if(m_debug_headers, boost::format("Minimum timestamp of track %1%: %2%\n") % dmx->id % (dmx_min_timestamp ? format_timestamp(*dmx_min_timestamp) : "none"));
  }

  mxdebug_if(m_debug_headers, boost::format("Minimum global timestamp: %1%\n") % (global_min_timestamp ? format_timestamp(*global_min_timestamp) : "none"));

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

  PTZR(dmx.ptzr)->m_ti.m_tcsync.displacement -= (m_audio_encoder_delay_samples * 1000000000ll) / dmx.a_samplerate;
  m_audio_encoder_delay_samples               = 0;
}

#define print_basic_atom_info() \
  mxdebug_if(m_debug_headers, boost::format("%1%'%2%' atom, size %3%, at %4%–%5%\n") % space(2 * level + 1) % atom.fourcc % atom.size % atom.pos % (atom.pos + atom.size));

#define print_atom_too_small_error(name, type)                                                                          \
  mxerror(boost::format(Y("Quicktime/MP4 reader: '%1%' atom is too small. Expected size: >= %2%. Actual size: %3%.\n")) \
          % name % sizeof(type) % atom.size);

void
qtmp4_reader_c::handle_cmov_atom(qt_atom_t parent,
                                 int level) {
  m_compression_algorithm = fourcc_c{};

  while (parent.size > 0) {
    qt_atom_t atom;

    atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "dcom")
      handle_dcom_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "cmvd")
      handle_cmvd_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_cmvd_atom(qt_atom_t atom,
                                 int level) {
  uint32_t moov_size = m_in->read_uint32_be();
  mxdebug_if(m_debug_headers, boost::format("%1%Uncompressed size: %2%\n") % space((level + 1) * 2 + 1) % moov_size);

  if (m_compression_algorithm != "zlib")
    mxerror(boost::format(Y("Quicktime/MP4 reader: This file uses compressed headers with an unknown "
                            "or unsupported compression algorithm '%1%'. Aborting.\n")) % m_compression_algorithm);

  mm_io_cptr old_in       = m_in;
  uint32_t cmov_size      = atom.size - atom.hsize;
  memory_cptr af_cmov_buf = memory_c::alloc(cmov_size);
  unsigned char *cmov_buf = af_cmov_buf->get_buffer();
  memory_cptr af_moov_buf = memory_c::alloc(moov_size + 16);
  unsigned char *moov_buf = af_moov_buf->get_buffer();

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
    mxerror(boost::format(Y("Quicktime/MP4 reader: This file uses compressed headers, but the zlib library could not be initialized. "
                            "Error code from zlib: %1%. Aborting.\n")) % zret);

  zret = inflate(&zs, Z_NO_FLUSH);
  if ((Z_OK != zret) && (Z_STREAM_END != zret))
    mxerror(boost::format(Y("Quicktime/MP4 reader: This file uses compressed headers, but they could not be uncompressed. "
                            "Error code from zlib: %1%. Aborting.\n")) % zret);

  if (moov_size != zs.total_out)
    mxwarn(boost::format(Y("Quicktime/MP4 reader: This file uses compressed headers, but the expected uncompressed size (%1%) "
                           "was not what is available after uncompressing (%2%).\n")) % moov_size % zs.total_out);

  zret = inflateEnd(&zs);

  m_in = mm_io_cptr(new mm_mem_io_c(moov_buf, zs.total_out));
  while (!m_in->eof()) {
    qt_atom_t next_atom = read_atom();
    mxdebug_if(m_debug_headers, boost::format("%1%'%2%' atom at %3%\n") % space((level + 1) * 2 + 1) % next_atom.fourcc % next_atom.pos);

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
  mxdebug_if(m_debug_headers, boost::format("%1%Frame offset table v%3%: %2% raw entries\n") % space(level * 2 + 1) % count % static_cast<unsigned int>(version));

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

  auto fmt = boost::format("%1%%2%: count %3% offset %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.raw_frame_offset_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.raw_frame_offset_table[idx].count % dmx.raw_frame_offset_table[idx].offset);
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample group description table: version %2%, type '%3%', %4% raw entries\n") % space(level * 2 + 2) % static_cast<unsigned int>(version) % grouping_type % count);

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

  auto fmt = boost::format("%1%%2%: leading samples known %3% num %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.random_access_point_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 2) % idx % dmx.random_access_point_table[idx].num_leading_samples_known % dmx.random_access_point_table[idx].num_leading_samples);
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample to group table: version %2%, type '%3%', %4% raw entries\n") % space(level * 2 + 2) % static_cast<unsigned int>(version) % grouping_type % count);

  auto &table = dmx.sample_to_group_tables[grouping_type.value()];
  table.reserve(table.size() + count);

  for (auto idx = 0u; idx < count; ++idx) {
    auto sample_count            = m_in->read_uint32_be();
    auto group_description_index = m_in->read_uint32_be();

    table.emplace_back(sample_count, group_description_index);
  }

  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%%2%: sample count %3% group description %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 2) % idx % table[idx].sample_count % table[idx].group_description_index);
}

void
qtmp4_reader_c::handle_dcom_atom(qt_atom_t,
                                 int level) {
  m_compression_algorithm = fourcc_c{m_in->read_uint32_be()};
  mxdebug_if(m_debug_headers, boost::format("%1%Compression algorithm: %2%\n") % space(level * 2 + 1) % m_compression_algorithm);
}

void
qtmp4_reader_c::handle_hdlr_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  hdlr_atom_t hdlr;

  if (sizeof(hdlr_atom_t) > atom.size)
    print_atom_too_small_error("hdlr", hdlr_atom_t);

  if (m_in->read(&hdlr, sizeof(hdlr_atom_t)) != sizeof(hdlr_atom_t))
    throw mtx::input::header_parsing_x();

  mxdebug_if(m_debug_headers, boost::format("%1%Component type: %|2$.4s| subtype: %|3$.4s|\n") % space(level * 2 + 1) % (char *)&hdlr.type % (char *)&hdlr.subtype);

  auto subtype = fourcc_c{reinterpret_cast<unsigned char const *>(&hdlr) + offsetof(hdlr_atom_t, subtype)};
  if (subtype == "soun")
    dmx.type = 'a';

  else if (subtype == "vide")
    dmx.type = 'v';

  else if ((subtype == "text") || (subtype == "subp"))
    dmx.type = 's';
}

void
qtmp4_reader_c::handle_mdhd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  if (1 > atom.size)
    print_atom_too_small_error("mdhd", mdhd_atom_t);

  int version = m_in->read_uint8();

  if (0 == version) {
    mdhd_atom_t mdhd;

    if (sizeof(mdhd_atom_t) > atom.size)
      print_atom_too_small_error("mdhd", mdhd_atom_t);
    if (m_in->read(&mdhd.flags, sizeof(mdhd_atom_t) - 1) != (sizeof(mdhd_atom_t) - 1))
      throw mtx::input::header_parsing_x();

    dmx.time_scale      = get_uint32_be(&mdhd.time_scale);
    dmx.global_duration = get_uint32_be(&mdhd.duration);
    dmx.language        = decode_and_verify_language(get_uint16_be(&mdhd.language));

  } else if (1 == version) {
    mdhd64_atom_t mdhd;

    if (sizeof(mdhd64_atom_t) > atom.size)
      print_atom_too_small_error("mdhd", mdhd64_atom_t);
    if (m_in->read(&mdhd.flags, sizeof(mdhd64_atom_t) - 1) != (sizeof(mdhd64_atom_t) - 1))
      throw mtx::input::header_parsing_x();

    dmx.time_scale      = get_uint32_be(&mdhd.time_scale);
    dmx.global_duration = get_uint64_be(&mdhd.duration);
    dmx.language        = decode_and_verify_language(get_uint16_be(&mdhd.language));

  } else
    mxerror(boost::format(Y("Quicktime/MP4 reader: The 'media header' atom ('mdhd') uses the unsupported version %1%.\n")) % version);

  mxdebug_if(m_debug_headers, boost::format("%1%Time scale: %2%, duration: %3%, language: %4%\n") % space(level * 2 + 1) % dmx.time_scale % dmx.global_duration % dmx.language);

  if (0 == dmx.time_scale)
    mxerror(Y("Quicktime/MP4 reader: The 'time scale' parameter was 0. This is not supported.\n"));
}

void
qtmp4_reader_c::handle_mdia_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  while (parent.size > 0) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "mdhd")
      handle_mdhd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "hdlr")
      handle_hdlr_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "minf")
      handle_minf_atom(dmx, atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_minf_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  while (0 < parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "hdlr")
      handle_hdlr_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "stbl")
      handle_stbl_atom(dmx, atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_moov_atom(qt_atom_t parent,
                                 int level) {
  while (parent.size > 0) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

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

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_mvex_atom(qt_atom_t parent,
                                 int level) {
  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "trex")
      handle_trex_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample defaults for track ID %2%: description idx %3% duration %4% size %5% flags %6%\n")
             % space(level * 2 + 1) % track_id % defaults.sample_description_id % defaults.sample_duration % defaults.sample_size % defaults.sample_flags);
}

void
qtmp4_reader_c::handle_moof_atom(qt_atom_t parent,
                                 int level,
                                 qt_atom_t const &moof_atom) {
  m_moof_offset              = moof_atom.pos;
  m_fragment_implicit_offset = moof_atom.pos;

  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "traf")
      handle_traf_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_traf_atom(qt_atom_t parent,
                                 int level) {
  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "tfhd")
      handle_tfhd_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "trun")
      handle_trun_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "edts") {
      if (m_track_for_fragment)
        handle_edts_atom(*m_track_for_fragment, atom.to_parent(), level + 1);
    }

    skip_atom();
    parent.size -= atom.size;
  }

  m_fragment           = nullptr;
  m_track_for_fragment = nullptr;
}

void
qtmp4_reader_c::handle_tfhd_atom(qt_atom_t,
                                 int level) {
  m_in->skip(1);                // Version

  auto flags     = m_in->read_uint24_be();
  auto track_id  = m_in->read_uint32_be();
  auto track_itr = brng::find_if(m_demuxers, [track_id](qtmp4_demuxer_cptr const &dmx) { return dmx->container_id == track_id; });

  if (!track_id || !mtx::includes(m_track_defaults, track_id) || (m_demuxers.end() == track_itr)) {
    mxdebug_if(m_debug_headers,
               boost::format("%1%tfhd atom with track_id(%2%) == 0, no entry in trex for it or no track(%3%) found\n")
               % space(level * 2 + 1) % track_id % (m_demuxers.end() == track_itr ? nullptr : track_itr->get()));
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
             boost::format("%1%Atom flags 0x%|2$05x| track_id %3% fragment content: moof_off %4% implicit_off %5% base_data_off %6% stsd_id %7% sample_duration %8% sample_size %9% sample_flags %10%\n")
             % space(level * 2 + 1) % flags % track_id
             % fragment.moof_offset % fragment.implicit_offset % fragment.base_data_offset % fragment.sample_description_id % fragment.sample_duration % fragment.sample_size % fragment.sample_flags);
}

void
qtmp4_reader_c::handle_trun_atom(qt_atom_t,
                                 int level) {
  if (!m_fragment || !m_track_for_fragment) {
    mxdebug_if(m_debug_headers, boost::format("%1%No current fragment (%2%) or track for that fragment (%3%)\n") % space(2 * level + 1) % m_fragment % m_track_for_fragment);
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

  std::vector<uint32_t> all_sample_flags;
  std::vector<bool> all_keyframe_flags;

  track.durmap_table.reserve(track.durmap_table.size() + entries);
  track.sample_table.reserve(track.sample_table.size() + entries);
  track.chunk_table.reserve(track.chunk_table.size() + entries);
  track.raw_frame_offset_table.reserve(track.raw_frame_offset_table.size() + entries);
  track.keyframe_table.reserve(track.keyframe_table.size() + entries);
  all_sample_flags.reserve(entries);
  all_keyframe_flags.reserve(entries);

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

    all_sample_flags.emplace_back(sample_flags);
    all_keyframe_flags.push_back(keyframe);
  }

  m_fragment->implicit_offset = offset;
  m_fragment_implicit_offset  = offset;

  mxdebug_if(m_debug_headers, boost::format("%1%Number of entries: %2%\n") % space((level + 1) * 2 + 1) % entries);

  if (!m_debug_tables)
    return;

  auto fmt                = boost::format("%1%%2%: duration %3% size %4% data start %5% end %6% pts offset %7% key? %8% raw flags 0x%|9$08x|\n");
  auto spc                = space((level + 2) * 2 + 1);
  auto durmap_start       = track.durmap_table.size()           - entries;
  auto sample_start       = track.sample_table.size()           - entries;
  auto chunk_start        = track.chunk_table.size()            - entries;
  auto frame_offset_start = track.raw_frame_offset_table.size() - entries;
  auto end                = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), entries);

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt
            % spc % idx
            % track.durmap_table[durmap_start + idx].duration
            % track.sample_table[sample_start + idx].size
            % track.chunk_table[chunk_start + idx].pos
            % (track.sample_table[sample_start + idx].size + track.chunk_table[chunk_start + idx].pos)
            % track.raw_frame_offset_table[frame_offset_start + idx].offset
            % static_cast<unsigned int>(all_keyframe_flags[idx])
            % all_sample_flags[idx]);
}

void
qtmp4_reader_c::handle_mvhd_atom(qt_atom_t atom,
                                 int level) {
  mvhd_atom_t mvhd;

  if (sizeof(mvhd_atom_t) > (atom.size - atom.hsize))
    print_atom_too_small_error("mvhd", mvhd_atom_t);
  if (m_in->read(&mvhd, sizeof(mvhd_atom_t)) != sizeof(mvhd_atom_t))
    throw mtx::input::header_parsing_x();

  m_time_scale = get_uint32_be(&mvhd.time_scale);

  mxdebug_if(m_debug_headers, boost::format("%1%Time scale: %2%\n") % space(level * 2 + 1) % m_time_scale);
}

void
qtmp4_reader_c::handle_udta_atom(qt_atom_t parent,
                                 int level) {
  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "chpl")
      handle_chpl_atom(atom.to_parent(), level + 1);

    else if (atom.fourcc == "meta")
      handle_meta_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_chpl_atom(qt_atom_t,
                                 int level) {
  if (m_ti.m_no_chapters || m_chapters)
    return;

  m_in->skip(1 + 3 + 4);          // Version, flags, zero

  int count = m_in->read_uint8();
  mxdebug_if(m_debug_chapters, boost::format("%1%Chapter list: %2% entries\n") % space(level * 2 + 1) % count);

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

  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "ilst")
      handle_ilst_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_ilst_atom(qt_atom_t parent,
                                 int level) {
  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "----")
      handle_4dashes_atom(atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
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

  while (8 <= parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "name")
      name = read_string_atom(atom, 4);

    else if (atom.fourcc == "mean")
      mean = read_string_atom(atom, 4);

    else if (atom.fourcc == "data")
      data = read_string_atom(atom, 8);

    skip_atom();
    parent.size -= atom.size;
  }

  mxdebug_if(m_debug_headers, boost::format("'----' content: name=%1% mean=%2% data=%3%\n") % name % mean % data);

  if (name == "iTunSMPB")
    parse_itunsmpb(data);
}

void
qtmp4_reader_c::parse_itunsmpb(std::string data) {
  data = boost::regex_replace(data, boost::regex("[^\\da-fA-F]+", boost::regex::perl), "");

  if (16 > data.length())
    return;

  try {
    m_audio_encoder_delay_samples = from_hex(data.substr(8, 8));
  } catch (std::bad_cast &) {
  }
}

void
qtmp4_reader_c::read_chapter_track() {
  if (m_ti.m_no_chapters || m_chapters)
    return;

  auto chapter_dmx_itr = brng::find_if(m_demuxers, [](qtmp4_demuxer_cptr const &dmx) { return dmx->is_chapters(); });
  if (m_demuxers.end() == chapter_dmx_itr)
    return;

  if ((*chapter_dmx_itr)->sample_table.empty())
    return;

  std::vector<qtmp4_chapter_entry_t> entries;
  uint64_t pts_scale_gcd = boost::math::gcd(static_cast<uint64_t>(1000000000ull), static_cast<uint64_t>((*chapter_dmx_itr)->time_scale));
  uint64_t pts_scale_num = 1000000000ull                                         / pts_scale_gcd;
  uint64_t pts_scale_den = static_cast<uint64_t>((*chapter_dmx_itr)->time_scale) / pts_scale_gcd;

  entries.reserve((*chapter_dmx_itr)->sample_table.size());

  for (auto &sample : (*chapter_dmx_itr)->sample_table) {
    if (2 >= sample.size)
      continue;

    m_in->setFilePointer(sample.pos, seek_beginning);
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

  mxdebug_if(m_debug_chapters, boost::format("%1%%2% chapter(s):\n") % space((level + 1) * 2 + 1) % entries.size());

  std::stable_sort(entries.begin(), entries.end());

  mm_mem_io_c out(nullptr, 0, 1000);
  out.set_file_name(m_ti.m_fname);
  out.write_bom("UTF-8");

  unsigned int i = 0;
  for (; entries.size() > i; ++i) {
    qtmp4_chapter_entry_t &chapter = entries[i];

    mxdebug_if(m_debug_chapters, boost::format("%1%%2%: start %4% name %3%\n") % space((level + 1) * 2 + 1) % i % chapter.m_name % format_timestamp(chapter.m_timestamp));

    out.puts(boost::format("CHAPTER%|1$02d|=%|2$02d|:%|3$02d|:%|4$02d|.%|5$03d|\n"
                           "CHAPTER%|1$02d|NAME=%6%\n")
             % i
             % ( chapter.m_timestamp / 60 / 60 / 1000000000)
             % ((chapter.m_timestamp      / 60 / 1000000000) %   60)
             % ((chapter.m_timestamp           / 1000000000) %   60)
             % ((chapter.m_timestamp           /    1000000) % 1000)
             % chapter.m_name);
  }

  mm_text_io_c text_out(&out, false);
  try {
    m_chapters = mtx::chapters::parse(&text_out, 0, -1, 0, m_ti.m_chapter_language, "", true);
    mtx::chapters::align_uids(m_chapters.get());

  } catch (mtx::chapters::parser_x &ex) {
    mxerror(boost::format(Y("The MP4 file '%1%' contains chapters whose format was not recognized. This is often the case if the chapters are not encoded in UTF-8. Use the '--chapter-charset' option in order to specify the charset to use.\n")) % m_ti.m_fname);
  }
}

void
qtmp4_reader_c::handle_stbl_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  while (0 < parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

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

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_stco_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, boost::format("%1%Chunk offset table: %2% entries\n") % space(level * 2 + 1) % count);

  dmx.chunk_table.reserve(dmx.chunk_table.size() + count);

  for (auto i = 0u; i < count; ++i)
    dmx.chunk_table.emplace_back(0, m_in->read_uint32_be());

  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%  %2%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunk_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space(level * 2 + 1) % dmx.chunk_table[idx].pos);
}

void
qtmp4_reader_c::handle_co64_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t,
                                 int level) {
  m_in->skip(1 + 3);        // version & flags
  uint32_t count = m_in->read_uint32_be();

  mxdebug_if(m_debug_headers, boost::format("%1%64bit chunk offset table: %2% entries\n") % space(level * 2 + 1) % count);

  dmx.chunk_table.reserve(dmx.chunk_table.size() + count);

  for (auto i = 0u; i < count; ++i)
    dmx.chunk_table.emplace_back(0, m_in->read_uint64_be());

  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%  %2%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunk_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space(level * 2 + 1) % dmx.chunk_table[idx].pos);
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample to chunk/chunkmap table: %2% entries\n") % space(level * 2 + 1) % count);
  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%%2%: first_chunk %3% samples_per_chunk %4% sample_description_id %5%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.chunkmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.chunkmap_table[idx].first_chunk % dmx.chunkmap_table[idx].samples_per_chunk % dmx.chunkmap_table[idx].sample_description_id);
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
      mxerror(boost::format(Y("Quicktime/MP4 reader: The 'size' field is too small in the stream description atom for track ID %1%.\n")) % dmx.id);

    dmx.stsd = memory_c::alloc(size);
    auto priv     = dmx.stsd->get_buffer();

    put_uint32_be(priv, size);
    if (m_in->read(priv + sizeof(uint32_t), size - sizeof(uint32_t)) != (size - sizeof(uint32_t)))
      mxerror(boost::format(Y("Quicktime/MP4 reader: Could not read the stream description atom for track ID %1%.\n")) % dmx.id);

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

  mxdebug_if(m_debug_headers, boost::format("%1%Sync/keyframe table: %2% entries\n") % space(level * 2 + 1) % count);
  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%keyframe at %2%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.keyframe_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % dmx.keyframe_table[idx]);
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

    mxdebug_if(m_debug_headers, boost::format("%1%Sample size table: %2% entries\n") % space(level * 2 + 1) % count);
    if (m_debug_tables) {
      auto fmt = boost::format("%1%%2%: size %3%\n");
      auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.sample_table.size());

      for (auto idx = 0u; idx < end; ++idx)
        mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.sample_table[idx].size);
    }

  } else {
    dmx.sample_size = sample_size;
    mxdebug_if(m_debug_headers, boost::format("%1%Sample size table; global sample size: %2%\n") % space(level * 2 + 1) % sample_size);
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample duration table: %2% entries\n") % space(level * 2 + 1) % count);
  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%%2%: number %3% duration %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.durmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.durmap_table[idx].number % dmx.durmap_table[idx].duration);
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

  mxdebug_if(m_debug_headers, boost::format("%1%Sample duration table: %2% entries\n") % space(level * 2 + 1) % count);
  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%%2%: number %3% duration %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.durmap_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.durmap_table[idx].number % dmx.durmap_table[idx].duration);
}

void
qtmp4_reader_c::handle_edts_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  while (0 < parent.size) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "elst")
      handle_elst_atom(dmx, atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }
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

  mxdebug_if(m_debug_headers, boost::format("%1%Edit list table: %2% entries\n") % space(level * 2 + 1) % count);
  if (!m_debug_tables)
    return;

  auto fmt = boost::format("%1%%2%: segment duration %3% media time %4% media rate %5%.%6%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), dmx.editlist_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % space((level + 1) * 2 + 1) % idx % dmx.editlist_table[idx].segment_duration % dmx.editlist_table[idx].media_time % dmx.editlist_table[idx].media_rate_integer % dmx.editlist_table[idx].media_rate_fraction);
}

void
qtmp4_reader_c::handle_tkhd_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t atom,
                                 int level) {
  tkhd_atom_t tkhd;

  if (sizeof(tkhd_atom_t) > atom.size)
    print_atom_too_small_error("tkhd", tkhd_atom_t);

  if (m_in->read(&tkhd, sizeof(tkhd_atom_t)) != sizeof(tkhd_atom_t))
    throw mtx::input::header_parsing_x();

  dmx.container_id         = get_uint32_be(&tkhd.track_id);
  dmx.v_display_width_flt  = get_uint32_be(&tkhd.track_width);
  dmx.v_display_height_flt = get_uint32_be(&tkhd.track_height);
  dmx.v_width              = dmx.v_display_width_flt  >> 16;
  dmx.v_height             = dmx.v_display_height_flt >> 16;

  mxdebug_if(m_debug_headers,
             boost::format("%1%Track ID: %2% display width × height %3%.%4% × %5%.%6%\n")
             % space(level * 2 + 1) % dmx.id % (dmx.v_display_width_flt >> 16) % (dmx.v_display_width_flt & 0xffff) % (dmx.v_display_height_flt >> 16) % (dmx.v_display_height_flt & 0xffff));
}

void
qtmp4_reader_c::handle_tref_atom(qtmp4_demuxer_c &/* dmx */,
                                 qt_atom_t parent,
                                 int level) {
  while (parent.size > 0) {
    qt_atom_t atom = read_atom();
    if ((atom.size > parent.size) || (12 > atom.size))
      break;

    std::vector<uint32_t> track_ids;
    for (auto idx = (atom.size - 4) / 8; 0 < idx; --idx)
      track_ids.push_back(m_in->read_uint32_be());

    if (atom.fourcc == "chap")
      for (auto track_id : track_ids)
        m_chapter_track_ids[track_id] = true;

    if (m_debug_headers) {
      std::string message;
      for (auto track_id : track_ids)
        message += (boost::format(" %1%") % track_id).str();
      mxdebug(boost::format("%1%Reference type: %2%; track IDs:%3%\n") % space(level * 2 + 1) % atom.fourcc % message);
    }

    skip_atom();
    parent.size -= atom.size;
  }
}

void
qtmp4_reader_c::handle_trak_atom(qtmp4_demuxer_c &dmx,
                                 qt_atom_t parent,
                                 int level) {
  while (parent.size > 0) {
    qt_atom_t atom = read_atom();
    print_basic_atom_info();

    if (atom.fourcc == "tkhd")
      handle_tkhd_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "mdia")
      handle_mdia_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "edts")
      handle_edts_atom(dmx, atom.to_parent(), level + 1);

    else if (atom.fourcc == "tref")
      handle_tref_atom(dmx, atom.to_parent(), level + 1);

    skip_atom();
    parent.size -= atom.size;
  }

  dmx.determine_codec();
  mxdebug_if(m_debug_headers, boost::format("%1%Codec determination result: %2%\n") % space(level * 2 + 1) % dmx.codec);
}

file_status_e
qtmp4_reader_c::read(generic_packetizer_c *ptzr,
                     bool) {
  size_t dmx_idx;

  for (dmx_idx = 0; dmx_idx < m_demuxers.size(); ++dmx_idx) {
    auto &dmx = *m_demuxers[dmx_idx];

    if ((-1 == dmx.ptzr) || (PTZR(dmx.ptzr) != ptzr))
      continue;

    if (dmx.pos < dmx.m_index.size())
      break;
  }

  if (m_demuxers.size() == dmx_idx)
    return flush_packetizers();

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

  } else if (   dmx.is_video()
             && dmx.codec.is(codec_c::type_e::V_PRORES)
             && (index.size >= 8)) {
    m_in->skip(8);
    index.size -= 8;
    buffer = memory_c::alloc(index.size);

  } else {
    buffer = memory_c::alloc(index.size);
  }

  if (m_in->read(buffer->get_buffer() + buffer_offset, index.size) != index.size) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Could not read chunk number %1%/%2% with size %3% from position %4%. Aborting.\n"))
           % dmx.pos % dmx.m_index.size() % index.size % index.file_pos);
    return flush_packetizers();
  }

  auto duration = dmx.m_use_frame_rate_for_duration ? *dmx.m_use_frame_rate_for_duration : index.duration;
  PTZR(dmx.ptzr)->process(new packet_t(buffer, index.timestamp, duration, index.is_keyframe ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC, VFT_NOBFRAME));
  ++dmx.pos;

  if (dmx.pos < dmx.m_index.size())
    return FILE_STATUS_MOREDATA;

  return flush_packetizers();
}

memory_cptr
qtmp4_reader_c::create_bitmap_info_header(qtmp4_demuxer_c &dmx,
                                          const char *fourcc,
                                          size_t extra_size,
                                          const void *extra_data) {
  int full_size           = sizeof(alBITMAPINFOHEADER) + extra_size;
  memory_cptr bih_p       = memory_c::alloc(full_size);
  alBITMAPINFOHEADER *bih = (alBITMAPINFOHEADER *)bih_p->get_buffer();

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
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));

  return true;
}

bool
qtmp4_reader_c::create_audio_packetizer_alac(qtmp4_demuxer_c &dmx) {
  auto magic_cookie = memory_c::clone(dmx.stsd->get_buffer() + dmx.stsd_non_priv_struct_size + 12, dmx.stsd->get_size() - dmx.stsd_non_priv_struct_size - 12);
  dmx.ptzr          = add_packetizer(new alac_packetizer_c(this, m_ti, magic_cookie, dmx.a_samplerate, dmx.a_channels));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));

  return true;
}

bool
qtmp4_reader_c::create_audio_packetizer_dts(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new dts_packetizer_c(this, m_ti, dmx.m_dts_header));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));

  return true;
}

void
qtmp4_reader_c::create_video_packetizer_svq1(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = create_bitmap_info_header(dmx, "SVQ1");
  dmx.ptzr            = add_packetizer(new video_for_windows_packetizer_c(this, m_ti, 0.0, dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpeg4_p2(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = create_bitmap_info_header(dmx, "DIVX");
  dmx.ptzr            = add_packetizer(new mpeg4_p2_video_packetizer_c(this, m_ti, 0.0, dmx.v_width, dmx.v_height, false));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpeg1_2(qtmp4_demuxer_c &dmx) {
  int version = !dmx.esds_parsed                              ? (dmx.fourcc.value() & 0xff) - '0'
              : dmx.esds.object_type_id == MP4OTI_MPEG1Visual ? 1
              :                                                 2;
  dmx.ptzr    = add_packetizer(new mpeg1_2_video_packetizer_c(this, m_ti, version, -1.0, dmx.v_width, dmx.v_height, 0, 0, false));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_avc(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() ? dmx.priv[0] : memory_cptr{};
  dmx.ptzr            = add_packetizer(new avc_video_packetizer_c(this, m_ti, boost::rational_cast<double>(dmx.frame_rate), dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_mpegh_p2(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.priv.size() ? dmx.priv[0] : memory_cptr{};
  dmx.ptzr            = add_packetizer(new hevc_video_packetizer_c(this, m_ti, boost::rational_cast<double>(dmx.frame_rate), dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_prores(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = memory_c::clone(dmx.fourcc.str());
  dmx.ptzr            = add_packetizer(new generic_video_packetizer_c(this, m_ti, MKV_V_PRORES, boost::rational_cast<double>(dmx.frame_rate), dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_video_packetizer_standard(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.stsd;
  dmx.ptzr            = add_packetizer(new quicktime_video_packetizer_c(this, m_ti, dmx.v_width, dmx.v_height));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_aac(qtmp4_demuxer_c &dmx) {
  m_ti.m_private_data = dmx.esds.decoder_config;
  dmx.ptzr            = add_packetizer(new aac_packetizer_c(this, m_ti, *dmx.a_aac_audio_config, aac_packetizer_c::headerless));

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_mp3(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, dmx.a_samplerate, dmx.a_channels, true));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_pcm(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new pcm_packetizer_c(this, m_ti, static_cast<int32_t>(dmx.a_samplerate), dmx.a_channels, dmx.a_bitdepth, dmx.m_pcm_format));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_vorbis(qtmp4_demuxer_c &dmx) {
  dmx.ptzr = add_packetizer(new vorbis_packetizer_c(this, m_ti, dmx.priv[0]->get_buffer(), dmx.priv[0]->get_size(), dmx.priv[1]->get_buffer(), dmx.priv[1]->get_size(), dmx.priv[2]->get_buffer(), dmx.priv[2]->get_size()));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_audio_packetizer_passthrough(qtmp4_demuxer_c &dmx) {
  passthrough_packetizer_c *ptzr = new passthrough_packetizer_c(this, m_ti);
  dmx.ptzr                      = add_packetizer(ptzr);

  ptzr->set_track_type(track_audio);
  ptzr->set_codec_id(MKV_A_QUICKTIME);
  ptzr->set_codec_private(dmx.stsd);
  ptzr->set_audio_sampling_freq(dmx.a_samplerate);
  ptzr->set_audio_channels(dmx.a_channels);
  ptzr->prevent_lacing();

  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
}

void
qtmp4_reader_c::create_subtitles_packetizer_vobsub(qtmp4_demuxer_c &dmx) {
  std::string palette;
  auto format = [](int v) { return (boost::format("%|1$02x|") % std::min(std::max(v, 0), 255)).str(); };
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

  mxdebug_if(m_debug_headers, boost::format("VobSub IDX str:\n%1%") % idx_str);

  m_ti.m_private_data = memory_c::clone(idx_str);
  dmx.ptzr = add_packetizer(new vobsub_packetizer_c(this, m_ti));
  show_packetizer_info(dmx.id, PTZR(dmx.ptzr));
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

    else if (dmx.codec.is(codec_c::type_e::V_MPEGH_P2))
      create_video_packetizer_mpegh_p2(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_SVQ1))
      create_video_packetizer_svq1(dmx);

    else if (dmx.codec.is(codec_c::type_e::V_PRORES))
      create_video_packetizer_prores(dmx);

    else
      create_video_packetizer_standard(dmx);

    dmx.set_packetizer_display_dimensions();
    dmx.set_packetizer_colour_properties();

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

    else if (dmx.codec.is(codec_c::type_e::A_VORBIS))
      create_audio_packetizer_vorbis(dmx);

    else
      create_audio_packetizer_passthrough(dmx);

    handle_audio_encoder_delay(dmx);

  } else {
    if (dmx.codec.is(codec_c::type_e::S_VOBSUB))
      create_subtitles_packetizer_vobsub(dmx);
  }

  if (packetizer_ok && (-1 == m_main_dmx))
    m_main_dmx = i;
}

void
qtmp4_reader_c::create_packetizers() {
  unsigned int i;

  m_main_dmx = -1;

  for (i = 0; i < m_demuxers.size(); ++i)
    create_packetizer(m_demuxers[i]->id);
}

int
qtmp4_reader_c::get_progress() {
  if (-1 == m_main_dmx)
    return 100;

  auto &dmx       = *m_demuxers[m_main_dmx];
  auto max_chunks = (0 == dmx.sample_size) ? dmx.sample_table.size() : dmx.chunk_table.size();

  return 100 * dmx.pos / max_chunks;
}

void
qtmp4_reader_c::identify() {
  unsigned int i;

  id_result_container();

  for (i = 0; i < m_demuxers.size(); ++i) {
    auto &dmx = *m_demuxers[i];
    auto info = mtx::id::info_c{};

    info.set(mtx::id::number, dmx.container_id);

    if (dmx.codec.is(codec_c::type_e::V_MPEG4_P10))
      info.add(mtx::id::packetizer, mtx::id::mpeg4_p10_video);

    else if (dmx.codec.is(codec_c::type_e::V_MPEGH_P2))
      info.add(mtx::id::packetizer, mtx::id::mpegh_p2_video);

    info.add(mtx::id::language, dmx.language);

    if (dmx.is_video())
      info.add(mtx::id::pixel_dimensions, boost::format("%1%x%2%") % dmx.v_width % dmx.v_height);

    else if (dmx.is_audio()) {
      info.add(mtx::id::audio_channels,           dmx.a_channels);
      info.add(mtx::id::audio_sampling_frequency, static_cast<uint64_t>(dmx.a_samplerate));
      info.add(mtx::id::audio_bits_per_sample,    dmx.a_bitdepth);
    }

    id_result_track(dmx.id,
                    dmx.is_video() ? ID_RESULT_TRACK_VIDEO : dmx.is_audio() ? ID_RESULT_TRACK_AUDIO : dmx.is_subtitles() ? ID_RESULT_TRACK_SUBTITLES : ID_RESULT_TRACK_UNKNOWN,
                    dmx.codec.get_name(dmx.fourcc.description()),
                    info.get());
  }

  if (m_chapters)
    id_result_chapters(mtx::chapters::count_atoms(*m_chapters));
}

void
qtmp4_reader_c::add_available_track_ids() {
  unsigned int i;

  for (i = 0; i < m_demuxers.size(); ++i)
    add_available_track_id(m_demuxers[i]->id);
}

std::string
qtmp4_reader_c::decode_and_verify_language(uint16_t coded_language) {
  std::string language;
  int i;

  for (i = 0; 3 > i; ++i)
    language += static_cast<char>(((coded_language >> ((2 - i) * 5)) & 0x1f) + 0x60);

  int idx = map_to_iso639_2_code(balg::to_lower_copy(language));
  if (-1 != idx)
    return g_iso639_languages[idx].iso639_2_code;

  return "";
}

void
qtmp4_reader_c::recode_chapter_entries(std::vector<qtmp4_chapter_entry_t> &entries) {
  if (g_identifying) {
    for (auto &entry : entries)
      entry.m_name = empty_string;
    return;
  }

  std::string charset              = m_ti.m_chapter_charset.empty() ? "UTF-8" : m_ti.m_chapter_charset;
  charset_converter_cptr converter = charset_converter_c::init(m_ti.m_chapter_charset);
  converter->enable_byte_order_marker_detection(true);

  if (m_debug_chapters) {
    mxdebug(boost::format("Number of chapter entries: %1%\n") % entries.size());
    size_t num = 0;
    for (auto &entry : entries) {
      mxdebug(boost::format("  Chapter %1%: name length %2%\n") % num++ % entry.m_name.length());
      debugging_c::hexdump(entry.m_name.c_str(), entry.m_name.length());
    }
  }

  for (auto &entry : entries)
    entry.m_name = converter->utf8(entry.m_name);

  converter->enable_byte_order_marker_detection(false);
}

void
qtmp4_reader_c::detect_interleaving() {
  std::list<qtmp4_demuxer_cptr> demuxers_to_read;
  boost::remove_copy_if(m_demuxers, std::back_inserter(demuxers_to_read), [this](auto const &dmx) {
    return !(dmx->ok && (dmx->is_audio() || dmx->is_video()) && this->demuxing_requested(dmx->type, dmx->id, dmx->language) && (dmx->sample_table.size() > 1));
  });

  if (demuxers_to_read.size() < 2) {
    mxdebug_if(m_debug_interleaving, boost::format("Interleaving: Not enough tracks to care about interleaving.\n"));
    return;
  }

  auto cmp = [](const qt_sample_t &s1, const qt_sample_t &s2) -> uint64_t { return s1.pos < s2.pos; };

  std::list<double> gradients;
  for (auto &dmx : demuxers_to_read) {
    uint64_t min = boost::min_element(dmx->sample_table, cmp)->pos;
    uint64_t max = boost::max_element(dmx->sample_table, cmp)->pos;
    gradients.push_back(static_cast<double>(max - min) / m_in->get_size());

    mxdebug_if(m_debug_interleaving, boost::format("Interleaving: Track id %1% min %2% max %3% gradient %4%\n") % dmx->id % min % max % gradients.back());
  }

  double badness = *boost::max_element(gradients) - *boost::min_element(gradients);
  mxdebug_if(m_debug_interleaving, boost::format("Interleaving: Badness: %1% (%2%)\n") % badness % (MAX_INTERLEAVING_BADNESS < badness ? "badly interleaved" : "ok"));

  if (MAX_INTERLEAVING_BADNESS < badness)
    m_in->enable_buffering(false);
}

// ----------------------------------------------------------------------

void
qtmp4_demuxer_c::calculate_frame_rate() {
  if ((1 == durmap_table.size()) && (0 != durmap_table[0].duration) && ((0 != sample_size) || (0 == frame_offset_table.size()))) {
    // Constant frame_rate. Let's set the default duration.
    frame_rate.assign(time_scale, static_cast<int64_t>(durmap_table[0].duration));
    mxdebug_if(m_debug_frame_rate, boost::format("calculate_frame_rate: case 1: %1%/%2%\n") % frame_rate.numerator() % frame_rate.denominator());

    return;
  }

  if (sample_table.size() < 2) {
    mxdebug_if(m_debug_frame_rate, boost::format("calculate_frame_rate: case 2: sample table too small\n"));
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
      m_use_frame_rate_for_duration.reset(boost::rational_cast<int64_t>(int64_rational_c{1'000'000'000ll} / frame_rate));

    mxdebug_if(m_debug_frame_rate,
               boost::format("calculate_frame_rate: case 3: duration %1% num_frames %2% frame_duration %3% frame_rate %4%/%5% use_frame_rate_for_duration %6%\n")
               % duration % num_frames % (duration / num_frames) % frame_rate.numerator() % frame_rate.denominator() % (m_use_frame_rate_for_duration ? *m_use_frame_rate_for_duration : -1));

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
    frame_rate.assign(static_cast<int64_t>(1000000000ll), to_nsecs(most_common.first));

  mxdebug_if(m_debug_frame_rate,
             boost::format("calculate_frame_rate: case 4: duration %1% num_frames %2% frame_duration %3% most_common.num_occurances %4% most_common.duration %5% frame_rate %6%/%7%\n")
             % duration % num_frames % (duration / num_frames) % most_common.second % to_nsecs(most_common.first) % frame_rate.numerator() % frame_rate.denominator());
}

int64_t
qtmp4_demuxer_c::to_nsecs(int64_t value,
                          boost::optional<int64_t> time_scale_to_use) {
  return boost::rational_cast<int64_t>(int64_rational_c{value, time_scale_to_use ? *time_scale_to_use : time_scale} * int64_rational_c{1'000'000'000ll, 1});
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

    timestamps.push_back(to_nsecs(static_cast<uint64_t>(chunk.samples) * duration + frame_offset));
    durations.push_back(to_nsecs(static_cast<uint64_t>(chunk.size)    * duration));
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
    mxdebug(boost::format("Timestamps for track ID %1%:\n") % id);
    auto fmt = boost::format("  %1%: pts %2%\n");
    auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), timestamps.size());

    for (auto idx = 0u; idx < end; ++idx)
      mxdebug(fmt % idx % format_timestamp(timestamps[idx]));
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

boost::optional<int64_t>
qtmp4_demuxer_c::min_timestamp()
  const {
  if (m_index.empty()) {
    return {};
  }

  return boost::accumulate(m_index, std::numeric_limits<int64_t>::max(), [](int64_t min, auto const &entry) { return std::min(min, entry.timestamp); });
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

  // workaround for fixed-size video frames (dv and uncompressed), but
  // also for audio with constant sample size
  if (sample_table.empty() && (sample_size > 1)) {
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
      duration = durmap_table[0].duration;
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
    mxdebug_if(m_debug_headers, boost::format("Track %1%: fewer timestamps assigned than entries in the sample table: %2% < %3%; dropping the excessive items\n") % id % s % num_samples);
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

  mxdebug(boost::format(" Frame offset table for track ID %1%: %2% entries\n")    % id % frame_offset_table.size());
  mxdebug(boost::format(" Sample table contents for track ID %1%: %2% entries\n") % id % sample_table.size());

  auto fmt = boost::format("   %1%: pts %2% size %3% pos %4%\n");
  auto end = std::min<std::size_t>(!m_debug_tables_full ? 20 : std::numeric_limits<std::size_t>::max(), sample_table.size());

  for (auto idx = 0u; idx < end; ++idx)
    mxdebug(fmt % idx % sample_table[idx].pts % sample_table[idx].size % sample_table[idx].pos);

  return true;
}

void
qtmp4_demuxer_c::apply_edit_list() {
  if (editlist_table.empty())
    return;

  mxdebug_if(m_debug_editlists,
             boost::format("Applying edit list for track %1%: %2% entries; track time scale %3%, global time scale %4%\n")
             % id % editlist_table.size() % time_scale % m_reader.m_time_scale);

  std::vector<qt_index_t> edited_index;

  auto const num_edits         = editlist_table.size();
  auto const num_index_entries = m_index.size();
  auto const index_begin       = m_index.begin();
  auto const index_end         = m_index.end();
  auto const global_time_scale = m_reader.m_time_scale;
  auto timeline_cts            = int64_t{};
  auto info_fmt                = boost::format("%1% [segment_duration %2% media_time %3% media_rate %4%/%5%]");
  auto entry_index             = 0u;

  for (auto &edit : editlist_table) {
    auto info = (info_fmt % entry_index % edit.segment_duration % edit.media_time % edit.media_rate_integer % edit.media_rate_fraction).str();
    ++entry_index;

    if ((edit.media_rate_integer == 0) && (edit.media_rate_fraction == 0)) {
      mxdebug_if(m_debug_editlists, boost::format("  %1%: dwell at timeline CTS %2%; such entries are not supported yet\n") % info % format_timestamp(timeline_cts));
      continue;
    }

    if ((edit.media_rate_integer != 1) || (edit.media_rate_fraction != 0)) {
      mxdebug_if(m_debug_editlists, boost::format("  %1%: slow down/speed up at timeline CTS %2%; such entries are not supported yet\n") % info % format_timestamp(timeline_cts));
      continue;
    }

    if ((edit.media_time < 0) && (edit.media_time != -1)) {
      mxdebug_if(m_debug_editlists, boost::format("  %1%: invalid media_time (< 0 and != -1)\n") % info);
      continue;
    }

    if (edit.media_time == -1) {
      timeline_cts = to_nsecs(edit.segment_duration, global_time_scale);
      mxdebug_if(m_debug_editlists, boost::format("  %1%: empty edit (media_time == -1); track start offset %2%\n") % info % format_timestamp(timeline_cts));
      continue;
    }

    if ((edit.segment_duration == 0) && (edit.media_time >= 0) && (entry_index < num_edits)) {
      mxdebug_if(m_debug_editlists, boost::format("  %1%: empty edit (segment_duration == 0, media_time >= 0) and more entries present; ignoring\n") % info);
      continue;
    }

    if (num_edits == 1) {
      timeline_cts          = to_nsecs(edit.media_time) * -1;
      edit.media_time       = 0;
      edit.segment_duration = 0;
      mxdebug_if(m_debug_editlists, boost::format("  %1%: single edit with positive media_time; track start offset %2%; change to non-edit to copy the rest\n") % info % format_timestamp(timeline_cts));
    }

    // Determine first frame index whose CTS (composition timestamp)
    // is bigger than or equal to the current edit's start CTS.
    auto const edit_duration  = to_nsecs(edit.segment_duration, global_time_scale);
    auto const edit_start_cts = to_nsecs(edit.media_time);
    auto const edit_end_cts   = edit_start_cts + edit_duration;
    auto itr                  = boost::range::find_if(m_index, [edit_start_cts](auto const &entry) { return (entry.timestamp + entry.duration - (entry.duration > 0 ? 1 : 0)) >= edit_start_cts; });
    auto const frame_idx      = static_cast<uint64_t>(std::distance(index_begin, itr));

    mxdebug_if(m_debug_editlists,
               boost::format("  %1%: normal entry; first frame %2% edit CTS %3%–%4% at timeline CTS %5%\n")
               % info % (frame_idx >= num_index_entries ? -1 : frame_idx) % format_timestamp(edit_start_cts) % format_timestamp(edit_end_cts) % format_timestamp(timeline_cts));

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

  m_index = std::move(edited_index);

  if (m_debug_editlists)
    dump_index_entries("Index after edit list");
}

void
qtmp4_demuxer_c::dump_index_entries(std::string const &message)
  const {
  mxdebug(boost::format("%1% for track ID %2%: %3% entries\n") % message % id % m_index.size());

  auto fmt = boost::format("  %1%: timestamp %2% duration %3% key? %4% file_pos %5% size %6%\n");
  auto end = std::min<int>(!m_debug_indexes_full ? 10 : std::numeric_limits<int>::max(), m_index.size());

  for (int idx = 0; idx < end; ++idx) {
    auto const &entry = m_index[idx];
    mxdebug(fmt % idx % format_timestamp(entry.timestamp) % format_timestamp(entry.duration) % entry.is_keyframe % entry.file_pos % entry.size);
  }

  if (m_debug_indexes_full)
    return;

  auto start = m_index.size() - std::min<int>(m_index.size(), 10);
  end        = m_index.size();

  for (int idx = start; idx < end; ++idx) {
    auto const &entry = m_index[idx];
    mxdebug(fmt % idx % format_timestamp(entry.timestamp) % format_timestamp(entry.duration) % entry.is_keyframe % entry.file_pos % entry.size);
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
    m_reader.m_reader_packetizers[ptzr]->set_video_display_dimensions(display_width, display_height, OPTION_SOURCE_CONTAINER);
}

void
qtmp4_demuxer_c::set_packetizer_colour_properties() {
  if (v_colour_primaries != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_colour_primaries(v_colour_primaries, OPTION_SOURCE_CONTAINER);
  }
  if (v_colour_transfer_characteristics != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_colour_transfer_character(v_colour_transfer_characteristics, OPTION_SOURCE_CONTAINER);
  }
  if (v_colour_matrix_coefficients != 2) {
    m_reader.m_reader_packetizers[ptzr]->set_video_colour_matrix(v_colour_matrix_coefficients, OPTION_SOURCE_CONTAINER);
  }
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
    mxerror(boost::format(Y("Quicktime/MP4 reader: Could not read the sound description atom for track ID %1%.\n")) % id);

  sound_v1_stsd_atom_t sv1_stsd;
  sound_v2_stsd_atom_t sv2_stsd;
  memcpy(&sv1_stsd, stsd_raw, sizeof(sound_v0_stsd_atom_t));
  memcpy(&sv2_stsd, stsd_raw, sizeof(sound_v0_stsd_atom_t));

  if (fourcc)
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track ID %1% has more than one FourCC. Only using the first one (%2%) and not this one (%3%).\n"))
           % id % fourcc.description() % fourcc_c{sv1_stsd.v0.base.fourcc}.description());
  else
    fourcc = fourcc_c{sv1_stsd.v0.base.fourcc};

  auto version = get_uint16_be(&sv1_stsd.v0.version);
  a_channels   = get_uint16_be(&sv1_stsd.v0.channels);
  a_bitdepth   = get_uint16_be(&sv1_stsd.v0.sample_size);
  auto tmp     = get_uint32_be(&sv1_stsd.v0.sample_rate);
  a_samplerate = ((tmp & 0xffff0000) >> 16) + (tmp & 0x0000ffff) / 65536.0;

  mxdebug_if(m_debug_headers, boost::format("%1%[v0] FourCC: %2% channels: %3% sample size: %4% compression id: %5% sample rate: %6% version: %7%")
             % space(level * 2 + 1)
             % fourcc_c{reinterpret_cast<const unsigned char *>(sv1_stsd.v0.base.fourcc)}.description()
             % a_channels
             % a_bitdepth
             % get_uint16_be(&sv1_stsd.v0.compression_id)
             % a_samplerate
             % get_uint16_be(&sv1_stsd.v0.version));

  if (0 == version)
    stsd_non_priv_struct_size = sizeof(sound_v0_stsd_atom_t);

  else if (1 == version) {
    if (sizeof(sound_v1_stsd_atom_t) > size)
      mxerror(boost::format(Y("Quicktime/MP4 reader: Could not read the extended sound description atom for track ID %1%.\n")) % id);

    stsd_non_priv_struct_size = sizeof(sound_v1_stsd_atom_t);
    memcpy(&sv1_stsd, stsd_raw, stsd_non_priv_struct_size);

    if (m_debug_headers)
      mxinfo(boost::format(" [v1] samples per packet: %1% bytes per packet: %2% bytes per frame: %3% bytes_per_sample: %4%")
             % get_uint32_be(&sv1_stsd.v1.samples_per_packet)
             % get_uint32_be(&sv1_stsd.v1.bytes_per_packet)
             % get_uint32_be(&sv1_stsd.v1.bytes_per_frame)
             % get_uint32_be(&sv1_stsd.v1.bytes_per_sample));

  } else if (2 == version) {
    if (sizeof(sound_v2_stsd_atom_t) > size)
      mxerror(boost::format(Y("Quicktime/MP4 reader: Could not read the extended sound description atom for track ID %1%.\n")) % id);

    stsd_non_priv_struct_size = sizeof(sound_v2_stsd_atom_t);
    memcpy(&sv2_stsd, stsd_raw, stsd_non_priv_struct_size);

    a_channels   = get_uint32_be(&sv2_stsd.v2.channels);
    a_bitdepth   = get_uint32_be(&sv2_stsd.v2.bits_per_channel);
    a_samplerate = mtx::math::int_to_double(get_uint64_be(&sv2_stsd.v2.sample_rate));


    if (m_debug_headers)
      mxinfo(boost::format(" [v2] struct size: %1% sample rate: %2% channels: %3% const1: 0x%|4$08x| bits per channel: %5% flags: %6% bytes per frame: %7% samples per frame: %8%")
             % get_uint32_be(&sv2_stsd.v2.v2_struct_size)
             % a_samplerate
             % a_channels
             % get_uint32_be(&sv2_stsd.v2.const1)
             % a_bitdepth
             % get_uint32_be(&sv2_stsd.v2.flags)
             % get_uint32_be(&sv2_stsd.v2.bytes_per_frame)
             % get_uint32_be(&sv2_stsd.v2.samples_per_frame));
  }

  if (m_debug_headers)
    mxinfo("\n");
}

void
qtmp4_demuxer_c::handle_video_stsd_atom(uint64_t atom_size,
                                        int level) {
  stsd_non_priv_struct_size = sizeof(video_stsd_atom_t);

  if (sizeof(video_stsd_atom_t) > atom_size)
    mxerror(boost::format(Y("Quicktime/MP4 reader: Could not read the video description atom for track ID %1%.\n")) % id);

  video_stsd_atom_t v_stsd;
  auto stsd_raw = stsd->get_buffer();
  memcpy(&v_stsd, stsd_raw, sizeof(video_stsd_atom_t));

  if (fourcc)
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track ID %1% has more than one FourCC. Only using the first one (%2%) and not this one (%3%).\n"))
           % id % fourcc.description() % fourcc_c{v_stsd.base.fourcc}.description());

  else
    fourcc = fourcc_c{v_stsd.base.fourcc};

  codec = codec_c::look_up(fourcc);

  mxdebug_if(m_debug_headers, boost::format("%1%FourCC: %2%, width: %3%, height: %4%, depth: %5%\n")
             % space(level * 2 + 1)
             % fourcc_c{v_stsd.base.fourcc}.description()
             % get_uint16_be(&v_stsd.width)
             % get_uint16_be(&v_stsd.height)
             % get_uint16_be(&v_stsd.depth));

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
  fourcc_c colour_type{reinterpret_cast<unsigned char const *>(&colr_atom) + offsetof(colr_atom_t, colour_type)};

  if (!mtx::included_in(colour_type, "nclc", "nclx"))
    return;

  v_colour_primaries                = get_uint16_be(&colr_atom.colour_primaries);
  v_colour_transfer_characteristics = get_uint16_be(&colr_atom.transfer_characteristics);
  v_colour_matrix_coefficients      = get_uint16_be(&colr_atom.matrix_coefficients);

  mxdebug_if(m_debug_headers,
             boost::format("%1%colour primaries: %2%, transfer characteristics: %3%, matrix coefficients: %4%\n")
             % space(level * 2 + 1) % v_colour_primaries % v_colour_transfer_characteristics % v_colour_matrix_coefficients);
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
    mxdebug(boost::format("%1%FourCC: %2%\n") % space(level * 2 + 1) % fourcc.description());
    debugging_c::hexdump(stsd_raw, size);
  }
}

void
qtmp4_demuxer_c::parse_aac_esds_decoder_config() {
  if (!esds.decoder_config || (2 > esds.decoder_config->get_size())) {
    mxwarn(boost::format(Y("Track %1%: AAC found, but decoder config data has length %2%.\n")) % id % (esds.decoder_config ? esds.decoder_config->get_size() : 0));
    return;
  }

  a_aac_audio_config = mtx::aac::parse_audio_specific_config(esds.decoder_config->get_buffer(), esds.decoder_config->get_size());
  if (!a_aac_audio_config) {
    mxwarn(boost::format(Y("Track %1%: The AAC information could not be parsed.\n")) % id);
    return;
  }

  mxdebug_if(m_debug_headers,
             boost::format(" AAC: profile: %1%, sample_rate: %2%, channels: %3%, output_sample_rate: %4%, sbr: %5%\n")
             % a_aac_audio_config->profile % a_aac_audio_config->sample_rate % a_aac_audio_config->channels % a_aac_audio_config->output_sample_rate % a_aac_audio_config->sbr);

  a_channels   = a_aac_audio_config->channels;
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
qtmp4_demuxer_c::parse_audio_header_priv_atoms(uint64_t atom_size,
                                               int level) {
  auto mem  = stsd->get_buffer() + stsd_non_priv_struct_size;
  auto size = atom_size - stsd_non_priv_struct_size;

  mm_mem_io_c mio(mem, size);

  try {
    while (!mio.eof() && (mio.getFilePointer() < (size - 8))) {
      qt_atom_t atom;

      try {
        atom = read_qtmp4_atom(&mio, false);
      } catch (...) {
        return;
      }

      if (atom.fourcc != "esds") {
        mio.setFilePointer(atom.pos + 4);
        continue;
      }

      mxdebug_if(m_debug_headers, boost::format("%1%Audio private data size: %2%, type: '%3%'\n") % space((level + 1) * 2 + 1) % atom.size % atom.fourcc);

      if (!esds_parsed) {
        mm_mem_io_c memio(mem + atom.pos + atom.hsize, atom.size - atom.hsize);
        esds_parsed = parse_esds_atom(memio, level + 1);

        if (esds_parsed) {
          if (codec_c::look_up_object_type_id(esds.object_type_id).is(codec_c::type_e::A_AAC))
            parse_aac_esds_decoder_config();

          else if (codec_c::look_up_object_type_id(esds.object_type_id).is(codec_c::type_e::A_VORBIS))
            parse_vorbis_esds_decoder_config();
        }
      }

      mio.setFilePointer(atom.pos + atom.size);
    }
  } catch(...) {
  }
}

void
qtmp4_demuxer_c::parse_video_header_priv_atoms(uint64_t atom_size,
                                               int level) {
  auto mem  = stsd->get_buffer() + stsd_non_priv_struct_size;
  auto size = atom_size - stsd_non_priv_struct_size;

  if (!codec.is(codec_c::type_e::V_MPEG4_P10) && !codec.is(codec_c::type_e::V_MPEGH_P2) && size && !fourcc.equiv("mp4v") && !fourcc.equiv("xvid")) {
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

      mxdebug_if(m_debug_headers, boost::format("%1%Video private data size: %2%, type: '%3%'\n") % space((level + 1) * 2 + 1) % atom.size % atom.fourcc);

      if ((atom.fourcc == "esds") || (atom.fourcc == "avcC") || (atom.fourcc == "hvcC")) {
        if (priv.empty()) {
          priv.emplace_back(memory_c::alloc(atom.size - atom.hsize));

          if (mio.read(priv[0], priv[0]->get_size()) != priv[0]->get_size()) {
            priv.clear();
            return;
          }
        }

        if ((atom.fourcc == "esds") && !esds_parsed) {
          mm_mem_io_c memio(priv[0]->get_buffer(), priv[0]->get_size());
          esds_parsed = parse_esds_atom(memio, level + 1);
        }

      } else if (atom.fourcc == "colr")
        handle_colr_atom(mio.read(atom.size - atom.hsize), level + 2);

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

      mxdebug_if(m_debug_headers, boost::format("%1%Subtitles private data size: %2%, type: '%3%'\n") % space((level + 1) * 2 + 1) % atom.size % atom.fourcc);

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

  mxdebug_if(m_debug_headers, boost::format("%1%Decoder config data at end of parsing: %2%\n") % space((level + 1) * 2 + 1) % (esds.decoder_config ? esds.decoder_config->get_size() : 0));
}

bool
qtmp4_demuxer_c::parse_esds_atom(mm_mem_io_c &memio,
                                 int level) {
  int lsp      = (level + 1) * 2;
  esds.version = memio.read_uint8();
  esds.flags   = memio.read_uint24_be();
  auto tag     = memio.read_uint8();

  mxdebug_if(m_debug_headers, boost::format("%1%esds: version: %2%, flags: %3%\n") % space(lsp + 1) % static_cast<unsigned int>(esds.version) % esds.flags);

  if (MP4DT_ES == tag) {
    auto len             = memio.read_mp4_descriptor_len();
    esds.esid            = memio.read_uint16_be();
    esds.stream_priority = memio.read_uint8();
    mxdebug_if(m_debug_headers, boost::format("%1%esds: id: %2%, stream priority: %3%, len: %4%\n") % space(lsp + 1) % static_cast<unsigned int>(esds.esid) % static_cast<unsigned int>(esds.stream_priority) % len);

  } else {
    esds.esid = memio.read_uint16_be();
    mxdebug_if(m_debug_headers, boost::format("%1%esds: id: %2%\n") % space(lsp + 1) % static_cast<unsigned int>(esds.esid));
  }

  tag = memio.read_uint8();
  if (MP4DT_DEC_CONFIG != tag) {
    mxdebug_if(m_debug_headers, boost::format("%1%tag is not DEC_CONFIG (0x%|2$02x|) but 0x%|3$02x|.\n") % space(lsp + 1) % MP4DT_DEC_CONFIG % tag);
    return false;
  }

  auto len                = memio.read_mp4_descriptor_len();

  esds.object_type_id     = memio.read_uint8();
  esds.stream_type        = memio.read_uint8();
  esds.buffer_size_db     = memio.read_uint24_be();
  esds.max_bitrate        = memio.read_uint32_be();
  esds.avg_bitrate        = memio.read_uint32_be();
  esds.decoder_config.reset();

  mxdebug_if(m_debug_headers, boost::format("%1%esds: decoder config descriptor, len: %2%, object_type_id: %3%, "
                                            "stream_type: 0x%|4$2x|, buffer_size_db: %5%, max_bitrate: %|6$.3f|kbit/s, avg_bitrate: %|7$.3f|kbit/s\n")
             % space(lsp + 1)
             % len
             % static_cast<unsigned int>(esds.object_type_id)
             % static_cast<unsigned int>(esds.stream_type)
             % esds.buffer_size_db
             % (esds.max_bitrate / 1000.0)
             % (esds.avg_bitrate / 1000.0));

  tag = memio.read_uint8();
  if (MP4DT_DEC_SPECIFIC == tag) {
    len = memio.read_mp4_descriptor_len();
    if (!len)
      throw mtx::input::header_parsing_x();

    esds.decoder_config = memory_c::alloc(len);
    if (memio.read(esds.decoder_config, len) != len) {
      esds.decoder_config.reset();
      throw mtx::input::header_parsing_x();
    }

    tag = memio.read_uint8();

    if (m_debug_headers) {
      mxdebug(boost::format("%1%esds: decoder specific descriptor, len: %2%\n") % space(lsp + 1) % len);
      mxdebug(boost::format("%1%esds: dumping decoder specific descriptor\n") % space(lsp + 3));
      debugging_c::hexdump(esds.decoder_config->get_buffer(), esds.decoder_config->get_size());
    }

  } else
    mxdebug_if(m_debug_headers, boost::format("%1%tag is not DEC_SPECIFIC (0x%|2$02x|) but 0x%|3$02x|.\n") % space(lsp + 1) % MP4DT_DEC_SPECIFIC % tag);

  if (MP4DT_SL_CONFIG == tag) {
    len = memio.read_mp4_descriptor_len();
    if (!len)
      throw mtx::input::header_parsing_x{};

    esds.sl_config     = memory_c::alloc(len);
    if (memio.read(esds.sl_config, len) != len) {
      esds.sl_config.reset();
      throw mtx::input::header_parsing_x();
    }

    mxdebug_if(m_debug_headers, boost::format("%1%esds: sync layer config descriptor, len: %2%\n") % space(lsp + 1) % len);

  } else
    mxdebug_if(m_debug_headers, boost::format("%1%tag is not SL_CONFIG (0x%|2$02x|) but 0x%|3$02x|.\n") % space(lsp + 1) % MP4DT_SL_CONFIG % tag);

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
  auto ok                 = false;

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
    ok           = true;

  } catch (bool) {
  }

  vorbis_info_clear(&vi);
  vorbis_comment_clear(&vc);

  return ok;
}

bool
qtmp4_demuxer_c::verify_audio_parameters() {
  if (codec.is(codec_c::type_e::A_MP2) || codec.is(codec_c::type_e::A_MP3))
    derive_track_params_from_mp3_audio_bitstream();

  else if (codec.is(codec_c::type_e::A_AC3))
    derive_track_params_from_ac3_audio_bitstream();

  else if (codec.is(codec_c::type_e::A_DTS))
    derive_track_params_from_dts_audio_bitstream();

  else if (codec.is(codec_c::type_e::A_VORBIS))
    return derive_track_params_from_vorbis_private_data();

  if ((0 == a_channels) || (0.0 == a_samplerate)) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track %1% is missing some data. Broken header atoms?\n")) % id);
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
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track %1% is missing some data. Broken header atoms?\n")) % id);
    return false;
  }

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
    mxwarn(boost::format(Y("Quicktime/MP4 reader: The audio track %1% is using an unsupported 'object type id' of %2% in the 'esds' atom. Skipping this track.\n")) % id % static_cast<unsigned int>(esds.object_type_id));
    return false;
  }

  if (cdc.is(codec_c::type_e::A_AAC) && (!esds.decoder_config || !a_aac_audio_config)) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: The AAC track %1% is missing the esds atom/the decoder config. Skipping this track.\n")) % id);
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_video_parameters() {
  if (!v_width || !v_height || !fourcc) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track %1% is missing some data. Broken header atoms?\n")) % id);
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
qtmp4_demuxer_c::verify_avc_video_parameters() {
  if (priv.empty() || (4 > priv[0]->get_size())) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: MPEG4 part 10/AVC track %1% is missing its decoder config. Skipping this track.\n")) % id);
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_hevc_video_parameters() {
  if (priv.empty() || (23 > priv[0]->get_size())) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: MPEGH part 2/HEVC track %1% is missing its decoder config. Skipping this track.\n")) % id);
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_mp4v_video_parameters() {
  if (!esds_parsed) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: The video track %1% is missing the ESDS atom. Skipping this track.\n")) % id);
    return false;
  }

  if (codec.is(codec_c::type_e::V_MPEG4_P2) && !esds.decoder_config) {
    // This is MPEG4 video, and we need header data for it.
    mxwarn(boost::format(Y("Quicktime/MP4 reader: MPEG4 track %1% is missing the esds atom/the decoder config. Skipping this track.\n")) % id);
    return false;
  }

  return true;
}

bool
qtmp4_demuxer_c::verify_subtitles_parameters() {
  if (codec.is(codec_c::type_e::S_VOBSUB))
    return verify_vobsub_subtitles_parameters();

  return false;
}

bool
qtmp4_demuxer_c::verify_vobsub_subtitles_parameters() {
  if (!esds.decoder_config || (64 > esds.decoder_config->get_size())) {
    mxwarn(boost::format(Y("Quicktime/MP4 reader: Track %1% is missing some data. Broken header atoms?\n")) % id);
    return false;
  }

  return true;
}

void
qtmp4_demuxer_c::determine_codec() {
  codec = codec_c::look_up_object_type_id(esds.object_type_id);

  if (!codec) {
    codec = codec_c::look_up(fourcc);

    if (codec.is(codec_c::type_e::A_PCM))
      m_pcm_format = fourcc == "twos" ? pcm_packetizer_c::big_endian_integer : pcm_packetizer_c::little_endian_integer;
  }
}
