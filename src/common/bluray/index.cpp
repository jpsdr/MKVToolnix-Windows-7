/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  Blu-ray index file handling

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/bit_reader.h"
#include "common/bluray/index.h"
#include "common/bluray/index_p.h"
#include "common/list_utils.h"
#include "common/mm_file_io.h"

namespace mtx::bluray::index {

void
first_playback_t::dump()
  const {
  mxinfo(fmt::format("  first_playback:\n"
                     "    object_type:              {0} ({1})\n",
                     object_type,
                       object_type == 0b01 ? "movie object"
                     : object_type == 0b10 ? "BD-J object"
                     :                       "reserved"));

  if (object_type == 0b01)
    mxinfo(fmt::format("    hdmv_title_playback_type: {0} ({2})\n"
                       "    mobj_id_ref:              0x{1:04x}\n",
                       hdmv_title_playback_type, mobj_id_ref,
                         hdmv_title_playback_type == 0b00 ? "movie title"
                       : hdmv_title_playback_type == 0b01 ? "interactive title"
                       :                                    "reserved"));
  else
    mxinfo(fmt::format("    bd_j_title_playback_type: {0} ({2})\n"
                       "    bjdo_file_name:           {1}\n",
                       bd_j_title_playback_type, bdjo_file_name,
                         bd_j_title_playback_type == 0b10 ? "movie title"
                       : bd_j_title_playback_type == 0b11 ? "interactive title"
                       :                                    "reserved"));
}

void
top_menu_t::dump()
  const {
  mxinfo(fmt::format("  top_menu:\n"
                     "    object_type:              {0} ({1})\n",
                     object_type,
                       object_type == 0b01 ? "movie object"
                     : object_type == 0b10 ? "BD-J object"
                     :                          "reserved"));

  if (object_type == 0b01)
    mxinfo(fmt::format("    mobj_id_ref:              0x{0:04x}\n", mobj_id_ref));

  else
    mxinfo(fmt::format("    bjdo_file_name:           {0}\n", bdjo_file_name));
}

void
title_t::dump(unsigned int idx)
  const {
  mxinfo(fmt::format("  title {0}:\n"
                     "    object_type:              {1} ({3})\n"
                     "    access_type:              {2} ({4})\n",
                     idx, object_type, access_type,
                       object_type == 0b01 ? "movie object"
                     : object_type == 0b10 ? "BD-J object"
                     :                       "reserved",
                       access_type == 0b00 ? "title search permitted"
                     : access_type == 0b01 ? "title search prohibited, title_number display permitted"
                     : access_type == 0b11 ? "title search prohibited, title_number display prohibited"
                     :                       "reserved"));

  if (object_type == 0b01)
    mxinfo(fmt::format("    hdmv_title_playback_type: {0} ({2})\n"
                       "    mobj_id_ref:              0x{1:04x}\n",
                       hdmv_title_playback_type, mobj_id_ref,
                         hdmv_title_playback_type == 0b00 ? "movie title"
                       : hdmv_title_playback_type == 0b01 ? "interactive title"
                       :                                    "reserved"));
  else
    mxinfo(fmt::format("    bd_j_title_playback_type: {0} ({2})\n"
                       "    bjdo_file_name:           {1}\n",
                       bd_j_title_playback_type, bdjo_file_name,
                         bd_j_title_playback_type == 0b10 ? "movie title"
                       : bd_j_title_playback_type == 0b11 ? "interactive title"
                       :                                    "reserved"));
}

void
index_t::dump()
  const {
  mxinfo(fmt::format("Index dump:\n"));

  first_playback.dump();
  top_menu.dump();

  mxinfo(fmt::format("  num_titles:                 {0}\n", titles.size()));

  auto idx = 0;
  for (auto const &title : titles)
    title.dump(idx++);
}

// ------------------------------------------------------------

parser_c::parser_c(std::string file_name)
  : p_ptr{new parser_private_c{std::move(file_name)}}
{
}

parser_c::parser_c(parser_private_c &p)
  : p_ptr{&p}
{
}

parser_c::~parser_c() {         // NOLINT(modernize-use-equals-default) due to pimpl idiom requiring explicit dtor declaration somewhere
}

void
parser_c::dump()
  const {
  auto p = p_func();

  mxinfo(fmt::format("Index parser dump:\n"
                     "  ok:          {0}\n"
                     "  index_start: {1}\n",
                     p->m_ok, p->m_index_start));

  p->m_index.dump();
}

bool
parser_c::parse() {
  auto p = p_func();

  try {
    auto content = mm_file_io_c::slurp(p->m_file_name);
    if (!content)
      throw false;

    p->m_bc = std::make_shared<mtx::bits::reader_c>(content->get_buffer(), content->get_size());

    parse_header();
    parse_index();

    if (p->m_debug)
      dump();

    p->m_ok = true;

  } catch (...) {
    mxdebug_if(p->m_debug, "Parsing NOT OK\n");
  }

  p->m_bc.reset();

  return p->m_ok;
}

void
parser_c::parse_header() {
  auto p = p_func();

  p->m_bc->set_bit_position(0);

  auto magic = p->m_bc->get_bits(32);
  mxdebug_if(p->m_debug, fmt::format("File magic 1: 0x{0:08x}\n", magic));

  if (magic != mtx::calc_fourcc('I', 'N', 'D', 'X'))
    throw false;

  magic = p->m_bc->get_bits(32);
  mxdebug_if(p->m_debug, fmt::format("File magic 2: 0x{0:08x}\n", magic));

  if (!mtx::included_in(magic, mtx::calc_fourcc('0', '1', '0', '0'), mtx::calc_fourcc('0', '2', '0', '0'), mtx::calc_fourcc('0', '3', '0', '0')))
    throw false;

  p->m_index_start = p->m_bc->get_bits(32);
}

void
parser_c::parse_first_playback() {
  auto p         = p_func();
  auto &fp       = p->m_index.first_playback;
  fp.object_type = p->m_bc->get_bits(2);

  p->m_bc->skip_bits(30);             // reserved

  if (fp.object_type == 0b01) {
    fp.hdmv_title_playback_type = p->m_bc->get_bits(2);
    p->m_bc->skip_bits(14);           // alignment
    fp.mobj_id_ref              = p->m_bc->get_bits(16);
    p->m_bc->skip_bits(8 * 4);        // reserved

  } else if (fp.object_type == 0b10) {
    fp.bd_j_title_playback_type = p->m_bc->get_bits(2);
    p->m_bc->skip_bits(14);           // alignment
    fp.bdjo_file_name           = p->m_bc->get_string(5);
    p->m_bc->skip_bits(8);            // alignment

  } else
    throw false;
}

void
parser_c::parse_top_menu() {
  auto p         = p_func();
  auto &tm       = p->m_index.top_menu;
  tm.object_type = p->m_bc->get_bits(2);

  p->m_bc->skip_bits(30);             // reserved

  if (tm.object_type == 0b01) {
    p->m_bc->skip_bits(2 + 14);       // 01, alignment
    tm.mobj_id_ref = p->m_bc->get_bits(16);
    p->m_bc->skip_bits(8 * 4);        // reserved

  } else if (tm.object_type == 0b10) {
    p->m_bc->skip_bits(2 + 14);       // 11, alignment
    tm.bdjo_file_name = p->m_bc->get_string(5);
    p->m_bc->skip_bits(8);            // alignment

  } else
    throw false;
}

void
parser_c::parse_title() {
  auto p = p_func();

  p->m_index.titles.emplace_back();

  auto &t       = p->m_index.titles.back();
  t.object_type = p->m_bc->get_bits(2);
  t.access_type = p->m_bc->get_bits(2);
  p->m_bc->skip_bits(28);             // reserved

  if (t.object_type == 0b01) {
    t.hdmv_title_playback_type = p->m_bc->get_bits(2);
    p->m_bc->skip_bits(14);           // alignment
    t.mobj_id_ref              = p->m_bc->get_bits(16);
    p->m_bc->skip_bits(8 * 4);        // reserved

  } else if (t.object_type == 0b10) {
    t.bd_j_title_playback_type = p->m_bc->get_bits(2);
    p->m_bc->skip_bits(14);           // alignment
    t.bdjo_file_name           = p->m_bc->get_string(5);
    p->m_bc->skip_bits(8);            // alignment

  } else
    throw false;
}

void
parser_c::parse_index() {
  auto p = p_func();

  p->m_bc->set_bit_position(p->m_index_start * 8);

  p->m_bc->skip_bits(32);             // 32 bits length

  parse_first_playback();
  parse_top_menu();

  auto number_of_titles = p->m_bc->get_bits(16);

  for (auto title_idx = 0u; title_idx < number_of_titles; ++title_idx)
    parse_title();
}

bool
parser_c::is_ok()
  const {
  return p_ptr->m_ok;
}

index_t const &
parser_c::get_index()
  const {
  return p_ptr->m_index;
}

}
