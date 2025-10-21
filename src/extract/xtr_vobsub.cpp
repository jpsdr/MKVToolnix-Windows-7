/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
   Parts of this code (the MPEG header generation) was written by
   Mike Matsnev <mike@po.cs.msu.su>.
*/

#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/iso639.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/path.h"
#include "common/spu.h"
#include "common/strings/formatting.h"
#include "common/tta.h"
#include "extract/xtr_vobsub.h"

#if defined(COMP_MSC)
#pragma pack(push,1)
#endif
struct PACKED_STRUCTURE mpeg_es_header_t {
  uint8_t pfx[3];               // 00 00 01
  uint8_t stream_id;            // BD
  uint8_t len[2];
  uint8_t flags[2];
  uint8_t hlen;
  uint8_t pts[5];
  uint8_t lidx;
};

struct PACKED_STRUCTURE mpeg_ps_header_t {
  uint8_t pfx[4];               // 00 00 01 BA
  uint8_t scr[6];
  uint8_t muxr[3];
  uint8_t stlen;
};
#if defined(COMP_MSC)
#pragma pack(pop)
#endif

xtr_vobsub_c::xtr_vobsub_c(const std::string &codec_id,
                           int64_t tid,
                           track_spec_t &tspec)
  : xtr_base_c(codec_id, tid, tspec)
  , m_idx_file_name{mtx::fs::to_path(tspec.out_name).replace_extension(".idx")}
  , m_sub_file_name{mtx::fs::to_path(tspec.out_name).replace_extension(".sub")}
  , m_stream_id(0x20)
{
}

void
xtr_vobsub_c::create_file(xtr_base_c *master,
                          libmatroska::KaxTrackEntry &track) {
  init_content_decoder(track);

  auto priv = find_child<libmatroska::KaxCodecPrivate>(&track);
  if (priv) {
    m_private_data = decode_codec_private(priv);
    m_private_data->take_ownership();
  }

  m_master   = master;
  m_language = get_track_language(track);

  if (!master) {
    try {
      m_out = mm_write_buffer_io_c::open(m_sub_file_name.string(), 128 * 1024);
    } catch (mtx::mm_io::exception &ex) {
      mxerror(fmt::format(FY("Failed to create the VobSub data file '{0}': {1}\n"), m_sub_file_name.string(), ex));
    }

  } else {
    xtr_vobsub_c *vmaster = dynamic_cast<xtr_vobsub_c *>(m_master);

    if (!vmaster)
      mxerror(fmt::format(FY("Cannot extract tracks of different kinds to the same file. This was requested for the tracks {0} and {1}.\n"),
                          m_tid, m_master->m_tid));

    if (   (!m_private_data != !vmaster->m_private_data)
        || (m_private_data && (m_private_data->get_size() != vmaster->m_private_data->get_size()))
        || (m_private_data && memcmp(m_private_data->get_buffer(), vmaster->m_private_data->get_buffer(), m_private_data->get_size())))
      mxerror(fmt::format(FY("Two VobSub tracks can only be extracted into the same file if their CodecPrivate data matches. "
                             "This is not the case for the tracks {0} and {1}.\n"), m_tid, m_master->m_tid));

    vmaster->m_slaves.push_back(this);
    m_stream_id = vmaster->m_stream_id + 1;
    vmaster->m_stream_id++;
  }
}

void
xtr_vobsub_c::fix_spu_duration(memory_c &buffer,
                               timestamp_c const &duration)
  const {
  static debugging_option_c debug{"spu|spu_duration"};

  if (!duration.valid())
    return;

  auto current_duration = mtx::spu::get_duration(buffer.get_buffer(), buffer.get_size());
  auto diff             = current_duration.valid() ? (current_duration - duration).abs() : timestamp_c::ns(0);

  if (diff >= timestamp_c::ms(1)) {
    mxdebug_if(debug,
               fmt::format("vobsub: setting SPU duration to {0} (existing duration: {1}, difference: {2})\n",
                           mtx::string::format_timestamp(duration), mtx::string::format_timestamp(current_duration.to_ns(0)), mtx::string::format_timestamp(diff)));
    mtx::spu::set_duration(buffer.get_buffer(), buffer.get_size(), duration);
  }
}

void
xtr_vobsub_c::handle_frame(xtr_frame_t &f) {
  fix_spu_duration(*f.frame, f.duration > 0 ? timestamp_c::ns(f.duration) : timestamp_c{});

  static uint8_t padding_data[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  xtr_vobsub_c *vmaster = !m_master ? this : static_cast<xtr_vobsub_c *>(m_master);

  uint8_t *data = f.frame->get_buffer();
  size_t size         = f.frame->get_size();

  if (size < 4)
    return;

  m_positions.push_back(vmaster->m_out->getFilePointer());
  m_timestamps.push_back(f.timestamp);

  uint32_t padding = (2048 - (size + sizeof(mpeg_ps_header_t) + sizeof(mpeg_es_header_t))) & 2047;
  uint32_t first   = size + sizeof(mpeg_ps_header_t) + sizeof(mpeg_es_header_t) > 2048 ? 2048 - sizeof(mpeg_ps_header_t) - sizeof(mpeg_es_header_t) : size;

  uint64_t c       = f.timestamp * 9 / 100000;

  mpeg_ps_header_t ps;
  memset(&ps, 0, sizeof(mpeg_ps_header_t));
  ps.pfx[2]  = 0x01;
  ps.pfx[3]  = 0xba;
  ps.scr[0]  = 0x40 | ((uint8_t)(c >> 27) & 0x38) | 0x04 | ((uint8_t)(c >> 28) & 0x03);
  ps.scr[1]  = (uint8_t)(c >> 20);
  ps.scr[2]  = ((uint8_t)(c >> 12) & 0xf8) | 0x04 | ((uint8_t)(c >> 13) & 0x03);
  ps.scr[3]  = (uint8_t)(c >> 5);
  ps.scr[4]  = ((uint8_t)(c << 3) & 0xf8) | 0x04;
  ps.scr[5]  = 1;
  ps.muxr[0] = 1;
  ps.muxr[1] = 0x89;
  ps.muxr[2] = 0xc3; // just some value
  ps.stlen   = 0xf8;

  mpeg_es_header_t es;
  memset(&es, 0, sizeof(mpeg_es_header_t));
  es.pfx[2]    = 1;
  es.stream_id = 0xbd;
  es.len[0]    = (uint8_t)((first + 9) >> 8);
  es.len[1]    = (uint8_t)(first + 9);
  es.flags[0]  = 0x81;
  es.flags[1]  = 0x80;
  es.hlen      = 5;
  es.pts[0]    = 0x20 | ((uint8_t)(c >> 29) & 0x0e) | 0x01;
  es.pts[1]    = (uint8_t)(c >> 22);
  es.pts[2]    = ((uint8_t)(c >> 14) & 0xfe) | 0x01;
  es.pts[3]    = (uint8_t)(c >> 7);
  es.pts[4]    = (uint8_t)(c << 1) | 0x01;
  es.lidx      = !m_master ? 0x20 : m_stream_id;
  if ((6 > padding) && (first == size)) {
    es.hlen += (uint8_t)padding;
    es.len[0]  = (uint8_t)((first + 9 + padding) >> 8);
    es.len[1]  = (uint8_t)(first + 9 + padding);
  }

  vmaster->m_out->write(&ps, sizeof(mpeg_ps_header_t));
  vmaster->m_out->write(&es, sizeof(mpeg_es_header_t) - 1);
  if ((0 < padding) && (6 > padding) && (first == size))
    vmaster->m_out->write(padding_data, padding);
  vmaster->m_out->write(&es.lidx, 1);
  vmaster->m_out->write(data, first);
  while (first < size) {
    size    -= first;
    data    += first;

    padding  = (2048 - (size + 10 + sizeof(mpeg_ps_header_t))) & 2047;
    first    = size + 10 + sizeof(mpeg_ps_header_t) > 2048 ? 2048 - 10 - sizeof(mpeg_ps_header_t) : size;

    es.len[0]   = (uint8_t)((first + 4) >> 8);
    es.len[1]   = (uint8_t)(first + 4);
    es.flags[1] = 0;
    es.hlen     = 0;
    if ((6 > padding) && (first == size)) {
      es.hlen += (uint8_t)padding;
      es.len[0]  = (uint8_t)((first + 4 + padding) >> 8);
      es.len[1]  = (uint8_t)(first + 4 + padding);
    }

    vmaster->m_out->write(&ps, sizeof(mpeg_ps_header_t));
    vmaster->m_out->write(&es, 9);
    if ((0 < padding) && (6 > padding) && (first == size))
      vmaster->m_out->write(padding_data, padding);
    vmaster->m_out->write(&es.lidx, 1);
    vmaster->m_out->write(data, first);
  }
  if (6 <= padding) {
    padding      -= 6;
    es.stream_id  = 0xbe;
    es.len[0]     = (uint8_t)(padding >> 8);
    es.len[1]     = (uint8_t)padding;

    vmaster->m_out->write(&es, 6); // XXX

    while (0 < padding) {
      uint32_t todo = 8 < padding ? 8 : padding;
      vmaster->m_out->write(padding_data, todo);
      padding -= todo;
    }
  }
}

void
xtr_vobsub_c::finish_file() {
  if (m_master)
    return;

  try {
    static const char *header_line = "# VobSub index file, v7 (do not modify this line!)\n";

    m_out.reset();

    mm_write_buffer_io_c idx(std::make_shared<mm_file_io_c>(m_idx_file_name.string(), libebml::MODE_CREATE), 128 * 1024);
    mxinfo(fmt::format(FY("Writing the VobSub index file '{0}'.\n"), m_idx_file_name.string()));

    std::string header;

    if (m_private_data) {
      auto buffer = reinterpret_cast<char const *>(m_private_data->get_buffer());
      auto size   = m_private_data->get_size();

      while ((size > 0) && (buffer[size - 1] == 0))
        --size;

      header = std::string{ buffer, size };
      mtx::string::strip(header, true);
    }

    if (!balg::istarts_with(header, header_line))
      idx.puts(header_line);

    if (!header.empty())
      idx.puts(header + "\n");

    if (header.find("langidx:") == std::string::npos)
      idx.puts("langidx: 0\n");

    write_idx(idx, 0);
    size_t slave;
    for (slave = 0; slave < m_slaves.size(); slave++)
      m_slaves[slave]->write_idx(idx, slave + 1);

  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("Failed to create the file '{0}': {1}\n"), m_idx_file_name.string(), ex));
  }
}

void
xtr_vobsub_c::write_idx(mm_io_c &idx,
                        int index) {
  auto iso639_1 = m_language.get_language();
  idx.puts(fmt::format("\nid: {0}, index: {1}\n", iso639_1.empty() ? "--"s : iso639_1, index));

  size_t i;
  for (i = 0; i < m_positions.size(); i++) {
    int64_t timestamp = m_timestamps[i] / 1000000;

    idx.puts(fmt::format("timestamp: {0:02}:{1:02}:{2:02}:{3:03}, filepos: {4:1x}{5:08x}\n",
                         (timestamp / 60 / 60 / 1'000) % 60,
                         (timestamp / 60 / 1'000) % 60,
                         (timestamp / 1'000) % 60,
                         timestamp % 1'000,
                         m_positions[i] >> 32,
                         m_positions[i] & 0xffffffff));
  }
}

boost::filesystem::path
xtr_vobsub_c::get_file_name()
  const {
  return m_sub_file_name;
}
