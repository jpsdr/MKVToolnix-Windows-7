/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   SSA/ASS subtitle parser

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/base64.h"
#include "common/codec.h"
#include "common/extern_data.h"
#include "common/id_info.h"
#include "common/locale.h"
#include "input/r_ssa.h"
#include "merge/input_x.h"
#include "merge/file_status.h"


int
ssa_reader_c::probe_file(mm_text_io_c *in,
                         uint64_t) {
  return ssa_parser_c::probe(in);
}

ssa_reader_c::ssa_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c(ti, in)
{
}

void
ssa_reader_c::read_headers() {
  mm_text_io_cptr text_in;

  try {
    text_in = std::shared_ptr<mm_text_io_c>(new mm_text_io_c(m_in.get(), false));
  } catch (...) {
    throw mtx::input::open_x();
  }

  if (!ssa_reader_c::probe_file(text_in.get(), 0))
    throw mtx::input::invalid_format_x();

  charset_converter_cptr cc_utf8 = text_in->get_byte_order() != BO_NONE   ? charset_converter_c::init("UTF-8")
                                 : mtx::includes(m_ti.m_sub_charsets,  0) ? charset_converter_c::init(m_ti.m_sub_charsets[ 0])
                                 : mtx::includes(m_ti.m_sub_charsets, -1) ? charset_converter_c::init(m_ti.m_sub_charsets[-1])
                                 :                                          g_cc_local_utf8;

  m_ti.m_id  = 0;
  m_subs     = ssa_parser_cptr(new ssa_parser_c(this, text_in.get(), m_ti.m_fname, 0));
  m_encoding = text_in->get_encoding();

  m_subs->set_charset_converter(cc_utf8);
  m_subs->parse();

  show_demuxer_info();
}

ssa_reader_c::~ssa_reader_c() {
}

void
ssa_reader_c::create_packetizer(int64_t) {
  if (!demuxing_requested('s', 0) || (NPTZR() != 0))
    return;

  m_ti.m_private_data = memory_c::clone(m_subs->get_global());
  add_packetizer(new textsubs_packetizer_c(this, m_ti, m_subs->is_ass() ?  MKV_S_TEXTASS : MKV_S_TEXTSSA, false, false));
  show_packetizer_info(0, PTZR0);
}

file_status_e
ssa_reader_c::read(generic_packetizer_c *,
                   bool) {
  if (!m_subs->empty())
    m_subs->process((textsubs_packetizer_c *)PTZR0);

  return m_subs->empty() ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

int
ssa_reader_c::get_progress() {
  int num_entries = m_subs->get_num_entries();

  return 0 == num_entries ? 100 : 100 * m_subs->get_num_processed() / num_entries;
}

void
ssa_reader_c::identify() {
  auto info = mtx::id::info_c{};

  info.add(mtx::id::text_subtitles, true);
  if (m_encoding)
    info.add(mtx::id::encoding, *m_encoding);

  id_result_container();
  id_result_track(0, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_SSA_ASS, "SSA/ASS"), info.get());

  for (auto const &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description);
}
