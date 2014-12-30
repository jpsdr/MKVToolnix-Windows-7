/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   AAC demultiplexer module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <algorithm>

#include "common/codec.h"
#include "common/error.h"
#include "common/id3.h"
#include "input/r_aac.h"
#include "output/p_aac.h"
#include "merge/output_control.h"

int
aac_reader_c::probe_file(mm_io_c *in,
                         uint64_t,
                         int64_t probe_range,
                         int num_headers,
                         bool require_zero_offset) {
  int offset = find_valid_headers(*in, probe_range, num_headers);
  return (require_zero_offset && (0 == offset)) || (!require_zero_offset && (0 <= offset));
}

#define INITCHUNKSIZE 16384

aac_reader_c::aac_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c{ti, in}
  , m_chunk{memory_c::alloc(INITCHUNKSIZE)}
  , m_sbr_status_set{}
{
}

void
aac_reader_c::read_headers() {
  try {
    int tag_size_start = skip_id3v2_tag(*m_in);
    int tag_size_end   = id3_tag_present_at_end(*m_in);

    if (0 > tag_size_start)
      tag_size_start = 0;
    if (0 < tag_size_end)
      m_size -= tag_size_end;

    size_t init_read_len = std::min(m_size - tag_size_start, static_cast<uint64_t>(INITCHUNKSIZE));

    if (m_in->read(m_chunk, init_read_len) != init_read_len)
      throw mtx::input::header_parsing_x();

    m_in->setFilePointer(tag_size_start, seek_beginning);

    m_parser.copy_data(false);
    m_parser.add_bytes(m_chunk->get_buffer(), init_read_len);

    if (!m_parser.frames_available() || !m_parser.headers_parsed())
      throw mtx::input::header_parsing_x();

    m_aacheader          = m_parser.get_frame().m_header;
    m_parser             = aac::parser_c{};

    m_ti.m_id            = 0;       // ID for this track.
    int detected_profile = m_aacheader.profile;

    if (24000 >= m_aacheader.sample_rate)
      m_aacheader.profile = AAC_PROFILE_SBR;

    if (   (map_has_key(m_ti.m_all_aac_is_sbr,  0) && m_ti.m_all_aac_is_sbr[ 0])
        || (map_has_key(m_ti.m_all_aac_is_sbr, -1) && m_ti.m_all_aac_is_sbr[-1]))
      m_aacheader.profile = AAC_PROFILE_SBR;

    if (   (map_has_key(m_ti.m_all_aac_is_sbr,  0) && !m_ti.m_all_aac_is_sbr[ 0])
        || (map_has_key(m_ti.m_all_aac_is_sbr, -1) && !m_ti.m_all_aac_is_sbr[-1]))
      m_aacheader.profile = detected_profile;

    if (   map_has_key(m_ti.m_all_aac_is_sbr,  0)
        || map_has_key(m_ti.m_all_aac_is_sbr, -1))
      m_sbr_status_set = true;

  } catch (...) {
    throw mtx::input::open_x();
  }

  show_demuxer_info();
}

aac_reader_c::~aac_reader_c() {
}

void
aac_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('a', 0) || (NPTZR() != 0))
    return;

  if (!m_sbr_status_set)
    mxwarn(Y("AAC files may contain HE-AAC / AAC+ / SBR AAC audio. "
             "This can NOT be detected automatically. Therefore you have to "
             "specifiy '--aac-is-sbr 0' manually for this input file if the "
             "file actually contains SBR AAC. The file will be muxed in the "
             "WRONG way otherwise. Also read mkvmerge's documentation.\n"));

  auto aacpacketizer = new aac_packetizer_c(this, m_ti, m_aacheader.id, m_aacheader.profile, m_aacheader.sample_rate, m_aacheader.channels, false, true);
  add_packetizer(aacpacketizer);

  if (AAC_PROFILE_SBR == m_aacheader.profile)
    aacpacketizer->set_audio_output_sampling_freq(m_aacheader.sample_rate * 2);

  show_packetizer_info(0, aacpacketizer);
}

file_status_e
aac_reader_c::read(generic_packetizer_c *,
                   bool) {
  int remaining_bytes = m_size - m_in->getFilePointer();
  int read_len        = std::min(INITCHUNKSIZE, remaining_bytes);
  int num_read        = m_in->read(m_chunk, read_len);

  if (0 < num_read) {
    m_parser.add_bytes(m_chunk->get_buffer(), num_read);

    while (m_parser.frames_available()) {
      auto frame = m_parser.get_frame();
      PTZR0->process(std::make_shared<packet_t>(frame.m_data));
    }
  }

  return (0 != num_read) && (0 < (remaining_bytes - num_read)) ? FILE_STATUS_MOREDATA : flush_packetizers();
}

void
aac_reader_c::identify() {
  std::string verbose_info = std::string("aac_is_sbr:") + std::string(AAC_PROFILE_SBR == m_aacheader.profile ? "true" : "unknown");

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_AUDIO, codec_c::get_name(CT_A_AAC, "AAC"), verbose_info);
}

int
aac_reader_c::find_valid_headers(mm_io_c &in,
                                 int64_t probe_range,
                                 int num_headers) {
  try {
    in.setFilePointer(0, seek_beginning);
    auto buf      = memory_c::alloc(probe_range);
    auto num_read = in.read(buf->get_buffer(), probe_range);
    in.setFilePointer(0, seek_beginning);

    return aac::parser_c::find_consecutive_frames(buf->get_buffer(), num_read, num_headers);
  } catch (...) {
    return -1;
  }
}
