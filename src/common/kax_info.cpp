/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include <QDateTime>
#include <QTimeZone>

#include <ebml/EbmlDummy.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <ebml/EbmlCrc32.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#if LIBMATROSKA_VERSION < 0x020000
# include <matroska/KaxInfoData.h>
#endif
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxVersion.h>

#include "avilib.h"
#include "common/audio_emphasis.h"
#include "common/at_scope_exit.h"
#include "common/avc/avcc.h"
#include "common/avc/util.h"
#include "common/chapters/chapters.h"
#include "common/checksums/base.h"
#include "common/codec.h"
#include "common/command_line.h"
#include "common/date_time.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/fourcc.h"
#include "common/hevc/util.h"
#include "common/hevc/hevcc.h"
#include "common/kax_element_names.h"
#include "common/kax_file.h"
#include "common/kax_info.h"
#include "common/kax_info_p.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/mm_write_buffer_io.h"
#include "common/qt.h"
#include "common/stereo_mode.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/translation.h"
#include "common/version.h"
#include "common/xml/ebml_chapters_converter.h"
#include "common/xml/ebml_tags_converter.h"
#include "matroska/KaxSemantic.h"

using namespace mtx::kax_info;

namespace mtx {

kax_info_c::kax_info_c()
  : p_ptr{new kax_info::private_c}
{
  init();
}

kax_info_c::kax_info_c(kax_info::private_c &p)
  : p_ptr{&p}
{
  init();
}

kax_info_c::~kax_info_c() {     // NOLINT(modernize-use-equals-default) due to pimpl idiom requiring explicit dtor declaration somewhere
}

void
kax_info_c::init() {
  init_custom_element_value_formatters_and_processors();
}

void
kax_info_c::set_retain_elements(bool enable) {
  p_func()->m_retain_elements = enable;
}

void
kax_info_c::set_use_gui(bool enable) {
  p_func()->m_use_gui = enable;
}

void
kax_info_c::set_calc_checksums(bool enable) {
  p_func()->m_calc_checksums = enable;
}

void
kax_info_c::set_continue_at_cluster(bool enable) {
  p_func()->m_continue_at_cluster = enable;
}

void
kax_info_c::set_show_all_elements(bool enable) {
  p_func()->m_show_all_elements = enable;
}

void
kax_info_c::set_show_summary(bool enable) {
  p_func()->m_show_summary = enable;
}

void
kax_info_c::set_show_hexdump(bool enable) {
  p_func()->m_show_hexdump = enable;
}

void
kax_info_c::set_show_positions(bool enable) {
  p_func()->m_show_positions = enable;
}

void
kax_info_c::set_show_size(bool enable) {
  p_func()->m_show_size = enable;
}

void
kax_info_c::set_show_track_info(bool enable) {
  p_func()->m_show_track_info = enable;
}

void
kax_info_c::set_hex_positions(bool enable) {
  p_func()->m_hex_positions = enable;
}

void
kax_info_c::set_hexdump_max_size(int max_size) {
  p_func()->m_hexdump_max_size = max_size;
}

void
kax_info_c::set_destination_file_name(std::string const &file_name) {
  p_func()->m_destination_file_name = file_name;
}

void
kax_info_c::set_source_file_name(std::string const &file_name) {
  p_func()->m_source_file_name = file_name;
}

void
kax_info_c::set_source_file(mm_io_cptr const &file) {
  p_func()->m_in = file;
}

void
kax_info_c::ui_show_element_info(int level,
                                 std::string const &text,
                                 std::optional<int64_t> position,
                                 std::optional<int64_t> size,
                                 std::optional<int64_t> data_size) {
  std::string level_buffer(level, ' ');
  level_buffer[0] = '|';

  p_func()->m_out->puts(fmt::format("{0}+ {1}\n", level_buffer, create_element_text(text, position, size, data_size)));
}

void
kax_info_c::ui_show_element(libebml::EbmlElement &e) {
  auto p = p_func();

  if (p->m_show_summary)
    return;

  std::string level_buffer(p->m_level, ' ');
  level_buffer[0] = '|';

  p->m_out->puts(fmt::format("{0}+ {1}\n", level_buffer, create_text_representation(e)));
}

void
kax_info_c::ui_show_error(const std::string &error) {
  throw kax_info::exception{fmt::format("(MKVInfo) {0}\n", error)};
}

void
kax_info_c::ui_show_progress(int /* percentage */,
                             std::string const &/* text */) {
}

void
kax_info_c::show_element(libebml::EbmlElement *l,
                         int level,
                         const std::string &info,
                         std::optional<int64_t> position,
                         std::optional<int64_t> size) {
  if (p_func()->m_show_summary)
    return;

  ui_show_element_info(level, info,
                         position           ? *position
                       : !l                 ? std::optional<int64_t>{}
                       :                      static_cast<int64_t>(l->GetElementPosition()),
                         size               ? *size
                       : !l                 ? std::optional<int64_t>{}
                       : !l->IsFiniteSize() ? -2
                       :                      static_cast<int64_t>(l->GetSizeLength() + get_ebml_id(*l).GetLength() + l->GetSize()),
                         !l                 ? std::optional<int64_t>{}
                       : !l->IsFiniteSize() ? std::optional<int64_t>{}
                       :                      l->GetSize());
}

std::string
kax_info_c::format_ebml_id_as_hex(uint32_t id) {
  return fmt::format("{0:x}", id);
}

std::string
kax_info_c::format_ebml_id_as_hex(libebml::EbmlId const &id) {
  return format_ebml_id_as_hex(id.GetValue());
}

std::string
kax_info_c::format_ebml_id_as_hex(libebml::EbmlElement &e) {
  return format_ebml_id_as_hex(static_cast<const libebml::EbmlId &>(e));
}

std::string
kax_info_c::create_unknown_element_text(libebml::EbmlElement &e) {
  return fmt::format(FY("(Unknown element: {0}; ID: 0x{1} size: {2})"), EBML_NAME(&e), format_ebml_id_as_hex(e), (e.GetSize() + get_head_size(e)));
}

std::string
kax_info_c::create_known_element_but_not_allowed_here_text(libebml::EbmlElement &e) {
  return fmt::format(FY("(Known element, but invalid at this position: {0}; ID: 0x{1} size: {2})"), kax_element_names_c::get(e), format_ebml_id_as_hex(e), e.GetSize() + get_head_size(e));
}

std::string
kax_info_c::create_element_text(const std::string &text,
                                std::optional<int64_t> position,
                                std::optional<int64_t> size,
                                std::optional<int64_t> data_size) {
  auto p = p_func();

  std::string additional_text;

  if (position && p->m_show_positions)
    additional_text += fmt::format(p->m_hex_positions ? FY(" at 0x{0:x}") : FY(" at {0}"), *position);

  if (p->m_show_size && size) {
    if (-2 != *size)
      additional_text += fmt::format(FY(" size {0}"), *size);
    else
      additional_text += Y(" size is unknown");
  }

  if (p->m_show_size && data_size)
    additional_text += fmt::format(FY(" data size {0}"), *data_size);

  return text + additional_text;
}

std::string
kax_info_c::create_text_representation(libebml::EbmlElement &e) {
  auto text = kax_element_names_c::get(e);

  if (text.empty())
    text = create_unknown_element_text(e);

  else if (dynamic_cast<libebml::EbmlDummy *>(&e))
    text = create_known_element_but_not_allowed_here_text(e);

  else {
    auto value = format_element_value(e);
    if (!value.empty())
      text += ": "s + value;
  }

  auto size      = e.IsFiniteSize() ? get_head_size(e) + e.GetSize()      : -2ll;
  auto data_size = e.IsFiniteSize() ? std::optional<int64_t>{e.GetSize()} : std::optional<int64_t>{};

  return create_element_text(text, e.GetElementPosition(), size, data_size);
}

std::string
kax_info_c::format_element_value(libebml::EbmlElement &e) {
  auto p = p_func();

  auto formatter = p->m_custom_element_value_formatters.find(get_ebml_id(e).GetValue());

  return (formatter == p->m_custom_element_value_formatters.end()) || dynamic_cast<libebml::EbmlDummy *>(&e) ? format_element_value_default(e) : formatter->second(e);
}

std::string
kax_info_c::format_element_value_default(libebml::EbmlElement &e) {
  if (is_type<libebml::EbmlVoid>(e))
    return format_element_size(e);

  if (is_type<libebml::EbmlCrc32>(e))
    return fmt::format("0x{0:08x}", static_cast<libebml::EbmlCrc32 &>(e).GetCrc32());

  if (dynamic_cast<libebml::EbmlUInteger *>(&e))
    return fmt::to_string(static_cast<libebml::EbmlUInteger &>(e).GetValue());

  if (dynamic_cast<libebml::EbmlSInteger *>(&e))
    return fmt::to_string(static_cast<libebml::EbmlSInteger &>(e).GetValue());

  if (dynamic_cast<libebml::EbmlString *>(&e))
    return static_cast<libebml::EbmlString &>(e).GetValue();

  if (dynamic_cast<libebml::EbmlUnicodeString *>(&e))
    return static_cast<libebml::EbmlUnicodeString &>(e).GetValueUTF8();

  if (dynamic_cast<libebml::EbmlBinary *>(&e))
    return format_binary(static_cast<libebml::EbmlBinary &>(e));

  if (dynamic_cast<libebml::EbmlDate *>(&e))
    return mtx::date_time::format(QDateTime::fromSecsSinceEpoch(static_cast<libebml::EbmlDate &>(e).GetEpochDate(), QTimeZone::utc()), "%Y-%m-%d %H:%M:%S UTC");

  if (dynamic_cast<libebml::EbmlMaster *>(&e))
    return {};

  if (dynamic_cast<libebml::EbmlFloat *>(&e))
    return mtx::string::normalize_fmt_double_output(static_cast<libebml::EbmlFloat &>(e).GetValue());

  throw std::invalid_argument{"format_element_value: unsupported libebml::EbmlElement type"};
}

std::string
kax_info_c::create_hexdump(uint8_t const *buf,
                           int size) {
  std::string hex(" hexdump");
  int bmax = std::min(size, p_func()->m_hexdump_max_size);
  int b;

  for (b = 0; b < bmax; ++b)
    hex += fmt::format(" {0:02x}", static_cast<int>(buf[b]));

  return hex;
}

std::string
kax_info_c::create_codec_dependent_private_info(libmatroska::KaxCodecPrivate &c_priv,
                                                char track_type,
                                                std::string const &codec_id) {
  if ((codec_id == MKV_V_MSCOMP) && ('v' == track_type) && (c_priv.GetSize() >= sizeof(alBITMAPINFOHEADER))) {
    auto bih = reinterpret_cast<alBITMAPINFOHEADER *>(c_priv.GetBuffer());
    return fmt::format(FY(" (FourCC: {0})"), fourcc_c{&bih->bi_compression}.description());

  } else if ((codec_id == MKV_A_ACM) && ('a' == track_type) && (c_priv.GetSize() >= sizeof(alWAVEFORMATEX))) {
    auto wfe = reinterpret_cast<alWAVEFORMATEX *>(c_priv.GetBuffer());
    return fmt::format(FY(" (format tag: 0x{0:04x})"), get_uint16_le(&wfe->w_format_tag));

  } else if ((codec_id == MKV_V_MPEG4_AVC) && ('v' == track_type) && (c_priv.GetSize() >= 4)) {
    auto avcc = mtx::avc::avcc_c::unpack(memory_c::borrow(c_priv.GetBuffer(), c_priv.GetSize()));

    return fmt::format(FY(" (H.264 profile: {0} @L{1}.{2})"),
                          avcc.m_profile_idc ==  44 ? "CAVLC 4:4:4 Intra"
                        : avcc.m_profile_idc ==  66 ? "Baseline"
                        : avcc.m_profile_idc ==  77 ? "Main"
                        : avcc.m_profile_idc ==  83 ? "Scalable Baseline"
                        : avcc.m_profile_idc ==  86 ? "Scalable High"
                        : avcc.m_profile_idc ==  88 ? "Extended"
                        : avcc.m_profile_idc == 100 ? "High"
                        : avcc.m_profile_idc == 110 ? "High 10"
                        : avcc.m_profile_idc == 118 ? "Multiview High"
                        : avcc.m_profile_idc == 122 ? "High 4:2:2"
                        : avcc.m_profile_idc == 128 ? "Stereo High"
                        : avcc.m_profile_idc == 144 ? "High 4:4:4"
                        : avcc.m_profile_idc == 244 ? "High 4:4:4 Predictive"
                        :                             Y("Unknown"),
                        avcc.m_level_idc / 10, avcc.m_level_idc % 10);
  } else if ((codec_id == MKV_V_MPEGH_HEVC) && ('v' == track_type) && (c_priv.GetSize() >= 4)) {
    auto hevcc = mtx::hevc::hevcc_c::unpack(memory_c::borrow(c_priv.GetBuffer(), c_priv.GetSize()));

    return fmt::format(FY(" (HEVC profile: {0} @L{1}.{2})"),
                          hevcc.m_general_profile_idc == 1 ? "Main"
                        : hevcc.m_general_profile_idc == 2 ? "Main 10"
                        : hevcc.m_general_profile_idc == 3 ? "Main Still Picture"
                        :                                    Y("Unknown"),
                        hevcc.m_general_level_idc / 3 / 10, hevcc.m_general_level_idc / 3 % 10);
  }

  return "";
}

void
kax_info_c::add_track(std::shared_ptr<kax_info::track_t> const &t) {
  auto p = p_func();

  p->m_tracks.push_back(t);
  p->m_tracks_by_number[t->tnum] = t;
}

kax_info::track_t *
kax_info_c::find_track(int tnum) {
  return p_func()->m_tracks_by_number[tnum].get();
}

void
kax_info_c::read_master(libebml::EbmlMaster *m,
                        libebml::EbmlSemanticContext const &ctx,
                        int &upper_lvl_el,
                        libebml::EbmlElement *&l2) {
  m->Read(*p_func()->m_es, ctx, upper_lvl_el, l2, true);
  if (m->ListSize() == 0)
    return;

  std::sort(m->begin(), m->end(), [](auto const *a, auto const *b) { return a->GetElementPosition() < b->GetElementPosition(); });
}

std::string
kax_info_c::format_binary(libebml::EbmlBinary &bin) {
  auto &p      = *p_func();
  auto len     = std::min<std::size_t>(p.m_hexdump_max_size, bin.GetSize());
  auto const b = bin.GetBuffer();
  auto result  = fmt::format(FY("length {0}, data: {1}"), bin.GetSize(), mtx::string::to_hex(b, len));

  if (len < bin.GetSize())
    result += "…";

  if (p.m_calc_checksums)
    result += fmt::format(FY(" (adler: 0x{0:08x})"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, bin.GetBuffer(), bin.GetSize()));

  mtx::string::strip(result);

  return result;
}

std::string
kax_info_c::format_binary_as_hex(libebml::EbmlElement &e) {
  return mtx::string::to_hex(static_cast<libebml::EbmlBinary &>(e));
}

std::string
kax_info_c::format_element_size(libebml::EbmlElement &e) {
  if (e.IsFiniteSize())
    return fmt::format(FY("size {0}"), e.GetSize());
  return Y("size unknown");
}

std::string
kax_info_c::format_unsigned_integer_as_timestamp(libebml::EbmlElement &e) {
  return mtx::string::format_timestamp(static_cast<libebml::EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_unsigned_integer_as_scaled_timestamp(libebml::EbmlElement &e) {
  return mtx::string::format_timestamp(p_func()->m_ts_scale * static_cast<libebml::EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_timestamp(libebml::EbmlElement &e) {
  return mtx::string::format_timestamp(static_cast<libebml::EbmlSInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_scaled_timestamp(libebml::EbmlElement &e) {
  return mtx::string::format_timestamp(p_func()->m_ts_scale * static_cast<libebml::EbmlSInteger &>(e).GetValue());
}

void
kax_info_c::init_custom_element_value_formatters_and_processors() {
  using pre_mem_fn_t  = bool (kax_info_c::*)(libebml::EbmlElement &);
  using post_mem_fn_t = void (kax_info_c::*)(libebml::EbmlElement &);
  using fmt_mem_fn_t  = std::string (kax_info_c::*)(libebml::EbmlElement &);

  auto p = p_func();

  auto add_pre = [&p](libebml::EbmlId const &id, std::function<bool(libebml::EbmlElement &)> const &processor) {
    p->m_custom_element_pre_processors.insert({ id.GetValue(), processor });
  };

  auto add_pre_mem = [this, &p](libebml::EbmlId const &id, pre_mem_fn_t processor) {
    p->m_custom_element_pre_processors.insert({ id.GetValue(), std::bind(processor, this, std::placeholders::_1) });
  };

  auto add_post = [&p](libebml::EbmlId const &id, std::function<void(libebml::EbmlElement &)> const &processor) {
    p->m_custom_element_post_processors.insert({ id.GetValue(), processor });
  };

  auto add_post_mem = [this, &p](libebml::EbmlId const &id, post_mem_fn_t processor) {
    p->m_custom_element_post_processors.insert({ id.GetValue(), std::bind(processor, this, std::placeholders::_1) });
  };

  auto add_fmt = [&p](libebml::EbmlId const &id, std::function<std::string(libebml::EbmlElement &)> const &formatter) {
    p->m_custom_element_value_formatters.insert({ id.GetValue(), formatter });
  };

  auto add_fmt_mem = [this, &p](libebml::EbmlId const &id, fmt_mem_fn_t formatter) {
    p->m_custom_element_value_formatters.insert({ id.GetValue(), std::bind(formatter, this, std::placeholders::_1) });
  };

  p->m_custom_element_value_formatters.clear();
  p->m_custom_element_pre_processors.clear();
  p->m_custom_element_post_processors.clear();

  // Simple processors:
  add_pre(    EBML_ID(libmatroska::KaxInfo),        [p](libebml::EbmlElement &e) -> bool { p->m_ts_scale = find_child_value<kax_timestamp_scale_c>(static_cast<libmatroska::KaxInfo &>(e), TIMESTAMP_SCALE); return true; });
  add_pre(    EBML_ID(libmatroska::KaxTracks),      [p](libebml::EbmlElement &)  -> bool { p->m_mkvmerge_track_id = 0; return true; });
  add_pre_mem(EBML_ID(libmatroska::KaxSimpleBlock), &kax_info_c::pre_simple_block);
  add_pre_mem(EBML_ID(libmatroska::KaxBlockGroup),  &kax_info_c::pre_block_group);
  add_pre_mem(EBML_ID(libmatroska::KaxBlock),       &kax_info_c::pre_block);

  // More complex processors:
  add_pre(EBML_ID(libmatroska::KaxSeekHead), ([this, p](libebml::EbmlElement &e) -> bool {
    if (!p->m_show_all_elements && !p->m_use_gui)
      show_element(&e, p->m_level, Y("Seek head (subentries will be skipped)"));
    return p->m_show_all_elements || p->m_use_gui;
  }));

  add_pre(EBML_ID(libmatroska::KaxCues), ([this, p](libebml::EbmlElement &e) -> bool {
    if (!p->m_show_all_elements && !p->m_use_gui)
      show_element(&e, p->m_level, Y("Cues (subentries will be skipped)"));
    return p->m_show_all_elements || p->m_use_gui;
  }));

  add_pre(EBML_ID(libmatroska::KaxTrackEntry), [p](libebml::EbmlElement &e) -> bool {
    p->m_summary.clear();

    auto &master                 = static_cast<libebml::EbmlMaster &>(e);
    p->m_track                   = std::make_shared<kax_info::track_t>();
    p->m_track->tuid             = find_child_value<libmatroska::KaxTrackUID>(master);
    p->m_track->codec_id         = find_child_value<libmatroska::KaxCodecID>(master);
    p->m_track->default_duration = find_child_value<libmatroska::KaxTrackDefaultDuration>(master);

    return true;
  });

  add_pre(EBML_ID(libmatroska::KaxTrackNumber), ([this, p](libebml::EbmlElement &e) -> bool {
    p->m_track->tnum    = static_cast<libmatroska::KaxTrackNumber &>(e).GetValue();
    auto existing_track = find_track(p->m_track->tnum);
    auto track_id       = p->m_mkvmerge_track_id;

    if (!existing_track) {
      p->m_track->mkvmerge_track_id = p->m_mkvmerge_track_id;
      ++p->m_mkvmerge_track_id;
      add_track(p->m_track);

    } else
      track_id = existing_track->mkvmerge_track_id;

    p->m_summary.push_back(fmt::format(FY("mkvmerge/mkvextract track ID: {0}"), track_id));

    p->m_track_by_element[&e] = p->m_track;

    return true;
  }));

  add_pre(EBML_ID(libmatroska::KaxTrackType), [p](libebml::EbmlElement &e) -> bool {
    auto ttype       = static_cast<libmatroska::KaxTrackType &>(e).GetValue();
    p->m_track->type = track_audio    == ttype ? 'a'
                     : track_video    == ttype ? 'v'
                     : track_subtitle == ttype ? 's'
                     : track_buttons  == ttype ? 'b'
                     :                           '?';
    return true;
  });

  add_pre(EBML_ID(libmatroska::KaxCodecPrivate), ([this, p](libebml::EbmlElement &e) -> bool {
    auto &c_priv        = static_cast<libmatroska::KaxCodecPrivate &>(e);
    p->m_track-> fourcc = create_codec_dependent_private_info(c_priv, p->m_track->type, p->m_track->codec_id);

    if (p->m_calc_checksums && !p->m_show_summary)
      p->m_track->fourcc += fmt::format(FY(" (adler: 0x{0:08x})"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, c_priv.GetBuffer(), c_priv.GetSize()));

    if (p->m_show_hexdump)
      p->m_track->fourcc += create_hexdump(c_priv.GetBuffer(), c_priv.GetSize());

    p->m_track_by_element[&e] = p->m_track;

    return true;
  }));

  add_pre(EBML_ID(libmatroska::KaxCluster), ([this, p](libebml::EbmlElement &e) -> bool {
    p->m_cluster = static_cast<libmatroska::KaxCluster *>(&e);
    init_timestamp(*p->m_cluster, find_child_value<kax_cluster_timestamp_c>(p->m_cluster), p->m_ts_scale);

    ui_show_progress(100 * p->m_cluster->GetElementPosition() / p->m_file_size, Y("Parsing file"));

    return true;
  }));

  add_post(EBML_ID(libmatroska::KaxAudioSamplingFreq),       [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("sampling freq: {0}"), mtx::string::normalize_fmt_double_output(static_cast<libmatroska::KaxAudioSamplingFreq &>(e).GetValue()))); });
  add_post(EBML_ID(libmatroska::KaxAudioOutputSamplingFreq), [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("output sampling freq: {0}"), mtx::string::normalize_fmt_double_output(static_cast<libmatroska::KaxAudioOutputSamplingFreq &>(e).GetValue()))); });
  add_post(EBML_ID(libmatroska::KaxAudioChannels),           [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("channels: {0}"), static_cast<libmatroska::KaxAudioChannels &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxAudioBitDepth),           [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("bits per sample: {0}"), static_cast<libmatroska::KaxAudioBitDepth &>(e).GetValue())); });

  add_post(EBML_ID(libmatroska::KaxVideoPixelWidth),         [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel width: {0}"), static_cast<libmatroska::KaxVideoPixelWidth &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoPixelHeight),        [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel height: {0}"), static_cast<libmatroska::KaxVideoPixelHeight &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoDisplayWidth),       [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("display width: {0}"), static_cast<libmatroska::KaxVideoDisplayWidth &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoDisplayHeight),      [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("display height: {0}"), static_cast<libmatroska::KaxVideoDisplayHeight &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoPixelCropLeft),      [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel crop left: {0}"), static_cast<libmatroska::KaxVideoPixelCropLeft &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoPixelCropTop),       [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel crop top: {0}"), static_cast<libmatroska::KaxVideoPixelCropTop &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoPixelCropRight),     [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel crop right: {0}"), static_cast<libmatroska::KaxVideoPixelCropRight &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxVideoPixelCropBottom),    [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("pixel crop bottom: {0}"), static_cast<libmatroska::KaxVideoPixelCropBottom &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxTrackLanguage),           [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("language: {0}"), static_cast<libmatroska::KaxTrackLanguage &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxLanguageIETF),            [p](libebml::EbmlElement &e) { p->m_summary.push_back(fmt::format(FY("language (IETF BCP 47): {0}"), static_cast<libmatroska::KaxLanguageIETF &>(e).GetValue())); });
  add_post(EBML_ID(libmatroska::KaxBlockDuration),           [p](libebml::EbmlElement &e) { p->m_block_duration = static_cast<double>(static_cast<libmatroska::KaxBlockDuration &>(e).GetValue()) * p->m_ts_scale; });
  add_post(EBML_ID(libmatroska::KaxReferenceBlock),          [p](libebml::EbmlElement &)  { ++p->m_num_references; });

  add_post_mem(EBML_ID(libmatroska::KaxSimpleBlock),         &kax_info_c::post_simple_block);
  add_post_mem(EBML_ID(libmatroska::KaxBlock),               &kax_info_c::post_block);
  add_post_mem(EBML_ID(libmatroska::KaxBlockGroup),          &kax_info_c::post_block_group);

  add_post(EBML_ID(libmatroska::KaxTrackDefaultDuration),    [p](libebml::EbmlElement &) {
    p->m_summary.push_back(fmt::format(FY("default duration: {0:.3f}ms ({1:.3f} frames/fields per second for a video track)"),
                                       static_cast<double>(p->m_track->default_duration) / 1'000'000.0,
                                       1'000'000'000.0 / static_cast<double>(p->m_track->default_duration)));
  });

  add_post(EBML_ID(libmatroska::KaxTrackEntry), [p](libebml::EbmlElement &) {
    if (!p->m_show_summary)
      return;

    p->m_out->puts(fmt::format(FY("Track {0}: {1}, codec ID: {2}{3}{4}{5}\n"),
                               p->m_track->tnum,
                                 'a' == p->m_track->type ? Y("audio")
                      : 'v' == p->m_track->type ? Y("video")
                      : 's' == p->m_track->type ? Y("subtitles")
                      : 'b' == p->m_track->type ? Y("buttons")
                      :                           Y("unknown"),
                               p->m_track->codec_id,
                               p->m_track->fourcc,
                               p->m_summary.empty() ? "" : ", ",
                               mtx::string::join(p->m_summary, ", ")));
  });

  if (debugging_c::requested("dovi")) {
    add_post(EBML_ID(libmatroska::KaxBlockAddIDType),       [p](libebml::EbmlElement &e) { p->m_block_add_id_type       = static_cast<libmatroska::KaxBlockAddIDType &>(e).GetValue(); });
    add_post(EBML_ID(libmatroska::KaxBlockAddIDExtraData),  [p](libebml::EbmlElement &e) { p->m_block_add_id_extra_data = memory_c::clone(static_cast<libmatroska::KaxBlockAddIDExtraData &>(e)); });
    add_post(EBML_ID(libmatroska::KaxBlockAdditionMapping), [p](libebml::EbmlElement &) {
      if (p->m_block_add_id_type && p->m_block_add_id_extra_data) {
        try {
          mtx::bits::reader_c r{*p->m_block_add_id_extra_data};
          auto dovi_cfg = mtx::dovi::parse_dovi_decoder_configuration_record(r);
          dovi_cfg.dump();

        } catch(...) {
        }
      }

      p->m_block_add_id_type.reset();
      p->m_block_add_id_extra_data.reset();
    });
  }

  // Simple formatters:
  add_fmt_mem(EBML_ID(libmatroska::KaxSegmentUID),       &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(libmatroska::KaxSegmentFamily),    &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(libmatroska::KaxNextUID),          &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(libmatroska::KaxSegmentFamily),    &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(libmatroska::KaxPrevUID),          &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(libmatroska::KaxSegment),          &kax_info_c::format_element_size);
  add_fmt_mem(EBML_ID(libmatroska::KaxFileData),         &kax_info_c::format_element_size);
  add_fmt_mem(EBML_ID(libmatroska::KaxChapterTimeStart), &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxChapterTimeEnd),   &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxCueDuration),      &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxCueTime),          &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxCueRefTime),       &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxCodecDelay),       &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxSeekPreRoll),      &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(kax_cluster_timestamp_c),          &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxSliceDelay),       &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxSliceDuration),    &kax_info_c::format_signed_integer_as_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxSimpleBlock),      &kax_info_c::format_simple_block);
  add_fmt_mem(EBML_ID(libmatroska::KaxBlock),            &kax_info_c::format_block);
  add_fmt_mem(EBML_ID(libmatroska::KaxBlockDuration),    &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(libmatroska::KaxReferenceBlock),   &kax_info_c::format_signed_integer_as_scaled_timestamp);

  add_fmt(EBML_ID(libmatroska::KaxDuration),             [p](libebml::EbmlElement &e) { return mtx::string::format_timestamp(static_cast<int64_t>(static_cast<libebml::EbmlFloat &>(e).GetValue() * p->m_ts_scale)); });

  // More complex formatters:
  add_fmt(EBML_ID(libmatroska::KaxSeekID), [](libebml::EbmlElement &e) -> std::string {
    auto &seek_id = static_cast<libmatroska::KaxSeekID &>(e);
    if (seek_id.GetSize() > 4)
      return fmt::format("{0}", mtx::string::to_hex(seek_id));

    auto id = create_ebml_id_from(seek_id);

    return fmt::format("{0} ({1})",
                       mtx::string::to_hex(seek_id),
                         is_type<libmatroska::KaxInfo>(id)        ? "KaxInfo"
                       : is_type<libmatroska::KaxCluster>(id)     ? "KaxCluster"
                       : is_type<libmatroska::KaxTracks>(id)      ? "KaxTracks"
                       : is_type<libmatroska::KaxCues>(id)        ? "KaxCues"
                       : is_type<libmatroska::KaxAttachments>(id) ? "KaxAttachments"
                       : is_type<libmatroska::KaxChapters>(id)    ? "KaxChapters"
                       : is_type<libmatroska::KaxTags>(id)        ? "KaxTags"
                       : is_type<libmatroska::KaxSeekHead>(id)    ? "KaxSeekHead"
                       :                                            "unknown");
  });

  add_fmt(EBML_ID(libmatroska::KaxEmphasis), [](libebml::EbmlElement &e) -> std::string {
    auto audio_emphasis = static_cast<libmatroska::KaxEmphasis &>(e).GetValue();
    return fmt::format("{0} ({1})", audio_emphasis, audio_emphasis_c::translate(static_cast<audio_emphasis_c::mode_e>(audio_emphasis)));
  });

  add_fmt(EBML_ID(libmatroska::KaxVideoProjectionType), [](libebml::EbmlElement &e) -> std::string {
    auto value       = static_cast<libmatroska::KaxVideoProjectionType &>(e).GetValue();
    auto description = 0 == value ? Y("rectangular")
                     : 1 == value ? Y("equirectangular")
                     : 2 == value ? Y("cubemap")
                     : 3 == value ? Y("mesh")
                     :              Y("unknown");

    return fmt::format("{0} ({1})", value, description);
  });

  add_fmt(EBML_ID(libmatroska::KaxVideoDisplayUnit), [](libebml::EbmlElement &e) -> std::string {
      auto unit = static_cast<libmatroska::KaxVideoDisplayUnit &>(e).GetValue();
      return fmt::format("{0}{1}",
                         unit,
                           0 == unit ? Y(" (pixels)")
                         : 1 == unit ? Y(" (centimeters)")
                         : 2 == unit ? Y(" (inches)")
                         : 3 == unit ? Y(" (aspect ratio)")
                         :               "");
  });

  add_fmt(EBML_ID(libmatroska::KaxVideoFieldOrder), [](libebml::EbmlElement &e) -> std::string {
    auto field_order = static_cast<libmatroska::KaxVideoFieldOrder &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       field_order,
                          0 == field_order ? fmt::to_string(Y("progressive"))
                       :  1 == field_order ? fmt::format("tff ({})", Y("top field displayed first, top field stored first"))
                       :  2 == field_order ? fmt::to_string(Y("undetermined"))
                       :  6 == field_order ? fmt::format("bff ({})", Y("bottom field displayed first, bottom field stored first"))
                       :  9 == field_order ? fmt::format("tff ({})", Y("interleaved; top field displayed first; fields are interleaved in storage with the top line of the top field stored first"))
                       : 14 == field_order ? fmt::format("bff ({})", Y("interleaved; bottom field displayed first; fields are interleaved in storage with the top line of the top field stored first"))
                       :                     fmt::to_string(Y("unknown")));
  });

  add_fmt(EBML_ID(libmatroska::KaxVideoStereoMode), [](libebml::EbmlElement &e) -> std::string {
    auto stereo_mode = static_cast<libmatroska::KaxVideoStereoMode &>(e).GetValue();
    return fmt::format("{0} ({1})", stereo_mode, stereo_mode_c::translate(static_cast<stereo_mode_c::mode>(stereo_mode)));
  });

  add_fmt(EBML_ID(libmatroska::KaxVideoAspectRatio), [](libebml::EbmlElement &e) -> std::string {
    auto ar_type = static_cast<libmatroska::KaxVideoAspectRatio &>(e).GetValue();
    return fmt::format("{0}{1}",
                       ar_type,
                         0 == ar_type ? Y(" (free resizing)")
                       : 1 == ar_type ? Y(" (keep aspect ratio)")
                       : 2 == ar_type ? Y(" (fixed)")
                       :                  "");
  });

  add_fmt(EBML_ID(libmatroska::KaxContentEncodingScope), [](libebml::EbmlElement &e) -> std::string {
    std::vector<std::string> scope;
    auto ce_scope = static_cast<libmatroska::KaxContentEncodingScope &>(e).GetValue();

    if ((ce_scope & 0x01) == 0x01)
      scope.emplace_back(Y("1: all frames"));
    if ((ce_scope & 0x02) == 0x02)
      scope.emplace_back(Y("2: codec private data"));
    if ((ce_scope & 0xfc) != 0x00)
      scope.emplace_back(Y("rest: unknown"));
    if (scope.empty())
      scope.emplace_back(Y("unknown"));

    return fmt::format("{0} ({1})", ce_scope, mtx::string::join(scope, ", "));
  });

  add_fmt(EBML_ID(libmatroska::KaxContentEncodingType), [](libebml::EbmlElement &e) -> std::string {
    auto ce_type = static_cast<libmatroska::KaxContentEncodingType &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       ce_type,
                         0 == ce_type ? Y("compression")
                       : 1 == ce_type ? Y("encryption")
                       :                Y("unknown"));
  });

  add_fmt(EBML_ID(libmatroska::KaxContentCompAlgo), [](libebml::EbmlElement &e) -> std::string {
    auto c_algo = static_cast<libmatroska::KaxContentCompAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       c_algo,
                         0 == c_algo ?   "ZLIB"
                       : 1 == c_algo ?   "bzLib"
                       : 2 == c_algo ?   "lzo1x"
                       : 3 == c_algo ? Y("header removal")
                       :               Y("unknown"));
  });

  add_fmt(EBML_ID(libmatroska::KaxContentEncAlgo), [](libebml::EbmlElement &e) -> std::string {
    auto e_algo = static_cast<libmatroska::KaxContentEncAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       e_algo,
                         0 == e_algo ? Y("no encryption")
                       : 1 == e_algo ?   "DES"
                       : 2 == e_algo ?   "3DES"
                       : 3 == e_algo ?   "Twofish"
                       : 4 == e_algo ?   "Blowfish"
                       : 5 == e_algo ?   "AES"
                       :               Y("unknown"));
  });

  add_fmt(EBML_ID(libmatroska::KaxContentSigAlgo), [](libebml::EbmlElement &e) -> std::string {
    auto s_algo = static_cast<libmatroska::KaxContentSigAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       s_algo,
                         0 == s_algo ? Y("no signature algorithm")
                       : 1 == s_algo ? Y("RSA")
                       :               Y("unknown"));
  });

  add_fmt(EBML_ID(libmatroska::KaxContentSigHashAlgo), [](libebml::EbmlElement &e) -> std::string {
    auto s_halgo = static_cast<libmatroska::KaxContentSigHashAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       s_halgo,
                         0 == s_halgo ? Y("no signature hash algorithm")
                       : 1 == s_halgo ? Y("SHA1-160")
                       : 2 == s_halgo ? Y("MD5")
                       :                Y("unknown"));
  });

  add_fmt(EBML_ID(libmatroska::KaxTrackNumber), [p](libebml::EbmlElement &e) -> std::string { return fmt::format(FY("{0} (track ID for mkvmerge & mkvextract: {1})"), p->m_track_by_element[&e]->tnum, p->m_track_by_element[&e]->mkvmerge_track_id); });

  add_fmt(EBML_ID(libmatroska::KaxTrackType), [](libebml::EbmlElement &e) -> std::string {
    auto ttype = static_cast<libmatroska::KaxTrackType &>(e).GetValue();
    return track_audio    == ttype ? Y("audio")
         : track_video    == ttype ? Y("video")
         : track_subtitle == ttype ? Y("subtitles")
         : track_buttons  == ttype ? Y("buttons")
         :                           Y("unknown");
  });

  add_fmt(EBML_ID(libmatroska::KaxCodecPrivate), [p](libebml::EbmlElement &e) -> std::string {
    return fmt::format(FY("size {0}"), e.GetSize()) + p->m_track_by_element[&e]->fourcc;
  });

  add_fmt(EBML_ID(libmatroska::KaxTrackDefaultDuration), [](libebml::EbmlElement &e) -> std::string {
    auto default_duration = static_cast<libmatroska::KaxTrackDefaultDuration &>(e).GetValue();
    return fmt::format(FY("{0} ({1:.3f} frames/fields per second for a video track)"),
                       mtx::string::format_timestamp(default_duration),
                       1'000'000'000.0 / static_cast<double>(default_duration));
  });

  add_fmt(EBML_ID(libmatroska::KaxBlockAddIDType), [](auto &e) -> std::string {
    auto value       = static_cast<libebml::EbmlUInteger &>(e).GetValue();
    auto description = value >  std::numeric_limits<uint32_t>::max() ? Y("unknown")
                     : value >= 1u << 24                             ? fourcc_c{static_cast<uint32_t>(value)}.str()
                     : value ==   1                                  ? Y("codec-specific")
                     : value ==   4                                  ?   "ITU-T T.35"s
                     : value == 121                                  ? Y("SMPTE ST 12-1 timecodes")
                     :                                                 Y("unknown");
    return fmt::format("{0} ({1})", value, description);
  });

  add_fmt(EBML_ID(libmatroska::KaxChapterSkipType), [](libebml::EbmlElement &e) -> std::string {
    auto value       = static_cast<libebml::EbmlUInteger &>(e).GetValue();
    auto description = value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_NO_SKIPPING     ? Y("no skipping")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_OPENING_CREDITS ? Y("opening credits")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_END_CREDITS     ? Y("end credits")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_RECAP           ? Y("recap")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_NEXT_PREVIEW    ? Y("preview of next episode")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_PREVIEW         ? Y("preview of current episode")
                     : value == libmatroska::MATROSKA_CHAPTERSKIPTYPE_ADVERTISEMENT   ? Y("advertisement")
                     :                                                                  Y("unknown");

    return fmt::format("{0} ({1})", value, description);
  });
}

bool
kax_info_c::pre_block(libebml::EbmlElement &e) {
  auto p = p_func();

  auto &block = static_cast<libmatroska::KaxBlock &>(e);

  block.SetParent(*p->m_cluster);

  p->m_lf_timestamp   = get_global_timestamp(block);
  p->m_lf_tnum        = block.TrackNum();
  p->m_block_duration = std::nullopt;

  return true;
}

std::string
kax_info_c::format_block(libebml::EbmlElement &e) {
  auto p = p_func();

  auto &block = static_cast<libmatroska::KaxBlock &>(e);

  return fmt::format(FY("track number {0}, {1} frame(s), timestamp {2}"), block.TrackNum(), block.NumberFrames(), mtx::string::format_timestamp(p->m_lf_timestamp));
}

void
kax_info_c::post_block(libebml::EbmlElement &e) {
  auto p         = p_func();
  auto &block    = static_cast<libmatroska::KaxBlock &>(e);
  auto frame_pos = e.GetElementPosition() + e.ElementSize();

  for (int i = 0, num_frames = block.NumberFrames(); i < num_frames; ++i)
    frame_pos -= block.GetBuffer(i).Size();

  for (int i = 0, num_frames = block.NumberFrames(); i < num_frames; ++i) {
    auto &data = block.GetBuffer(i);
    auto adler = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (p->m_calc_checksums)
      adler_str = fmt::format(FY(" (adler: 0x{0:08x})"), adler);

    std::string hex;
    if (p->m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    auto text = p->m_show_size ? fmt::format(FY("Frame{0}{1}"),                            adler_str, hex)
              :                  fmt::format(FY("Frame with size {0}{1}{2}"), data.Size(), adler_str, hex);
    show_element(nullptr, p->m_level + 1, text, frame_pos, data.Size());

    p->m_frame_sizes.push_back(data.Size());
    p->m_frame_adlers.push_back(adler);
    p->m_frame_hexdumps.push_back(hex);

    frame_pos += data.Size();
  }
}

void
kax_info_c::show_frame_summary(libebml::EbmlElement &e) {
  auto p         = p_func();
  auto frame_pos = e.GetElementPosition() + e.ElementSize();

  for (auto size : p->m_frame_sizes)
    frame_pos -= size;

  for (auto fidx = 0u; fidx < p->m_frame_sizes.size(); fidx++) {
    std::string position;

    if (p->m_show_positions) {
      position   = fmt::format(p->m_hex_positions ? FY(", position 0x{0:x}") : FY(", position {0}"), frame_pos);
      frame_pos += p->m_frame_sizes[fidx];
    }

    if (p->m_block_duration)
      p->m_out->puts(fmt::format(FY("{0} frame, track {1}, timestamp {2}, duration {3}, size {4}, adler 0x{5:08x}{6}{7}\n"),
                                 (p->m_num_references >= 2 ? 'B' : p->m_num_references == 1 ? 'P' : 'I'),
                                 p->m_lf_tnum,
                                 mtx::string::format_timestamp(p->m_lf_timestamp),
                                 mtx::string::format_timestamp(*p->m_block_duration),
                                 p->m_frame_sizes[fidx],
                                 p->m_frame_adlers[fidx],
                                 p->m_frame_hexdumps[fidx],
                                 position));
    else
      p->m_out->puts(fmt::format(FY("{0} frame, track {1}, timestamp {2}, size {3}, adler 0x{4:08x}{5}{6}\n"),
                                 (p->m_num_references >= 2 ? 'B' : p->m_num_references == 1 ? 'P' : 'I'),
                                 p->m_lf_tnum,
                                 mtx::string::format_timestamp(p->m_lf_timestamp),
                                 p->m_frame_sizes[fidx],
                                 p->m_frame_adlers[fidx],
                                 p->m_frame_hexdumps[fidx],
                                 position));
  }
}

bool
kax_info_c::pre_block_group(libebml::EbmlElement &) {
  auto p              = p_func();

  p->m_num_references = 0;
  p->m_lf_timestamp   = 0;
  p->m_lf_tnum        = 0;

  p->m_frame_sizes.clear();
  p->m_frame_adlers.clear();
  p->m_frame_hexdumps.clear();

  return true;
}

void
kax_info_c::post_block_group(libebml::EbmlElement &e) {
  auto p = p_func();

  if (p->m_show_summary)
    show_frame_summary(e);

  auto &tinfo = p->m_track_info[p->m_lf_tnum];

  tinfo.m_blocks                                                       += p->m_frame_sizes.size();
  tinfo.m_blocks_by_ref_num[std::min<int64_t>(p->m_num_references, 2)] += p->m_frame_sizes.size();
  tinfo.m_min_timestamp                                                 = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : p->m_lf_timestamp, p->m_lf_timestamp);
  tinfo.m_size                                                         += std::accumulate(p->m_frame_sizes.begin(), p->m_frame_sizes.end(), 0);

  if (tinfo.m_max_timestamp && (*tinfo.m_max_timestamp >= p->m_lf_timestamp))
    return;

  tinfo.m_max_timestamp = p->m_lf_timestamp;

  if (!p->m_block_duration)
    tinfo.m_add_duration_for_n_packets  = p->m_frame_sizes.size();
  else {
    *tinfo.m_max_timestamp             += *p->m_block_duration;
    tinfo.m_add_duration_for_n_packets  = 0;
  }
}

bool
kax_info_c::pre_simple_block(libebml::EbmlElement &e) {
  auto p = p_func();

  p->m_frame_sizes.clear();
  p->m_frame_adlers.clear();
  p->m_frame_hexdumps.clear();

  static_cast<libmatroska::KaxSimpleBlock &>(e).SetParent(*p->m_cluster);

  return true;
}

std::string
kax_info_c::format_simple_block(libebml::EbmlElement &e) {
  auto &block       = static_cast<libmatroska::KaxSimpleBlock &>(e);
  auto timestamp_ns = mtx::math::to_signed(get_global_timestamp(block));

  std::string info;
  if (block.IsKeyframe())
    info = Y("key, ");
  if (block.IsDiscardable())
    info += Y("discardable, ");

  return fmt::format(FY("{0}track number {1}, {2} frame(s), timestamp {3}"),
                     info,
                     block.TrackNum(),
                     block.NumberFrames(),
                     mtx::string::format_timestamp(timestamp_ns));
}

void
kax_info_c::post_simple_block(libebml::EbmlElement &e) {
  auto p            = p_func();

  auto &block       = static_cast<libmatroska::KaxSimpleBlock &>(e);
  auto &tinfo       = p->m_track_info[block.TrackNum()];
  auto timestamp_ns = mtx::math::to_signed(get_global_timestamp(block));
  int num_frames    = block.NumberFrames();
  auto frames_start = block.GetElementPosition() + block.ElementSize();

  for (int idx = 0; idx < num_frames; ++idx)
    frames_start -= block.GetBuffer(idx).Size();

  auto frame_pos = frames_start;

  for (int idx = 0; idx < num_frames; ++idx) {
    auto &data = block.GetBuffer(idx);
    auto adler = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (p->m_calc_checksums)
      adler_str = fmt::format(FY(" (adler: 0x{0:08x})"), adler);

    std::string hex;
    if (p->m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    auto text = p->m_show_size ? fmt::format(FY("Frame{0}{1}"),                            adler_str, hex)
              :                  fmt::format(FY("Frame with size {0}{1}{2}"), data.Size(), adler_str, hex);
    show_element(nullptr, p->m_level + 1, text, frame_pos, data.Size());

    p->m_frame_sizes.push_back(data.Size());
    p->m_frame_adlers.push_back(adler);

    frame_pos += data.Size();
  }

  if (p->m_show_summary) {
    std::string position;
    frame_pos = frames_start;

    for (int idx = 0; idx < num_frames; idx++) {
      if (p->m_show_positions) {
        position   = fmt::format(p->m_hex_positions ? FY(", position 0x{0:x}") : FY(", position {0}"), frame_pos);
        frame_pos += p->m_frame_sizes[idx];
      }

      p->m_out->puts(fmt::format(FY("{0} frame, track {1}, timestamp {2}, size {3}, adler 0x{4:08x}{5}\n"),
                                 (block.IsKeyframe() ? 'I' : block.IsDiscardable() ? 'B' : 'P'),
                                 block.TrackNum(),
                                 mtx::string::format_timestamp(timestamp_ns),
                                 p->m_frame_sizes[idx],
                                 p->m_frame_adlers[idx],
                                 position));
    }
  }

  tinfo.m_blocks                                                                    += block.NumberFrames();
  tinfo.m_blocks_by_ref_num[block.IsKeyframe() ? 0 : block.IsDiscardable() ? 2 : 1] += block.NumberFrames();
  tinfo.m_min_timestamp                                                              = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_max_timestamp                                                              = std::max(tinfo.m_max_timestamp ? *tinfo.m_max_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_add_duration_for_n_packets                                                 = block.NumberFrames();
  tinfo.m_size                                                                      += std::accumulate(p->m_frame_sizes.begin(), p->m_frame_sizes.end(), 0);
}

kax_info_c::result_e
kax_info_c::handle_segment(libmatroska::KaxSegment &l0) {
  ui_show_element(l0);

  auto p        = p_func();
  auto l1       = std::shared_ptr<libebml::EbmlElement>{};
  auto kax_file = std::make_shared<kax_file_c>(*p->m_in);
  p->m_level    = 1;

  kax_file->set_segment_end(l0);

  // Prevent reporting "first timestamp after resync":
  kax_file->set_timestamp_scale(-1);

  while ((l1 = kax_file->read_next_level1_element())) {
    retain_element(l1);

    if (is_type<libmatroska::KaxCluster>(*l1) && !p->m_continue_at_cluster && !p->m_show_summary) {
      ui_show_element(*l1);
      return result_e::succeeded;

    } else
      handle_elements_generic(*l1);

    if (!p->m_in->setFilePointer2(l1->GetElementPosition() + kax_file->get_element_size(*l1)))
      break;

    auto in_parent = !l0.IsFiniteSize()
                  || (p->m_in->getFilePointer() < (l0.GetDataStart() + l0.GetSize()));
    if (!in_parent)
      break;

    if (p->m_abort)
      return result_e::aborted;

  } // while (l1)

  return result_e::succeeded;
}

bool
kax_info_c::run_generic_pre_processors(libebml::EbmlElement &e) {
  auto p = p_func();

  auto is_dummy = !!dynamic_cast<libebml::EbmlDummy *>(&e);
  if (is_dummy)
    return true;

  auto pre_processor = p->m_custom_element_pre_processors.find(get_ebml_id(e).GetValue());
  if (pre_processor != p->m_custom_element_pre_processors.end())
    if (!pre_processor->second(e))
      return false;

  return true;
}

void
kax_info_c::run_generic_post_processors(libebml::EbmlElement &e) {
  auto p = p_func();

  auto is_dummy = !!dynamic_cast<libebml::EbmlDummy *>(&e);
  if (is_dummy)
    return;

  auto post_processor = p->m_custom_element_post_processors.find(get_ebml_id(e).GetValue());
  if (post_processor != p->m_custom_element_post_processors.end())
    post_processor->second(e);
}

void
kax_info_c::handle_elements_generic(libebml::EbmlElement &e) {
  auto p = p_func();

  if (!run_generic_pre_processors(e))
    return;

  ui_show_element(e);

  if (dynamic_cast<libebml::EbmlMaster *>(&e)) {
    ++p->m_level;

    for (auto child : static_cast<libebml::EbmlMaster &>(e))
      handle_elements_generic(*child);

    --p->m_level;
  }

  run_generic_post_processors(e);
}

void
kax_info_c::display_track_info() {
  auto p = p_func();

  if (!p->m_show_track_info)
    return;

  for (auto &track : p->m_tracks) {
    track_info_t &tinfo  = p->m_track_info[track->tnum];

    if (!tinfo.m_min_timestamp)
      tinfo.m_min_timestamp = 0;
    if (!tinfo.m_max_timestamp)
      tinfo.m_max_timestamp = tinfo.m_min_timestamp;

    int64_t duration  = *tinfo.m_max_timestamp - *tinfo.m_min_timestamp;
    duration         += tinfo.m_add_duration_for_n_packets * track->default_duration;

    p->m_out->puts(fmt::format(FY("Statistics for track number {0}: number of blocks: {1}; size in bytes: {2}; duration in seconds: {3}; approximate bitrate in bits/second: {4}\n"),
                               track->tnum,
                               tinfo.m_blocks,
                               tinfo.m_size,
                               mtx::string::normalize_fmt_double_output(fmt::format("{0:.9f}", duration / 1000000000.0)),
                               static_cast<uint64_t>(duration == 0 ? 0 : tinfo.m_size * 8000000000.0 / duration)));
  }
}

void
kax_info_c::reset() {
  auto p        = p_func();
  p->m_ts_scale = TIMESTAMP_SCALE;
  p->m_tracks.clear();
  p->m_tracks_by_number.clear();
  p->m_track_info.clear();
  p->m_es.reset();
  p->m_in.reset();
}

kax_info_c::result_e
kax_info_c::process_file() {
  auto p         = p_func();

  p->m_file_size = p->m_in->get_size();
  p->m_es        = std::make_shared<libebml::EbmlStream>(*p->m_in);

  // Find the libebml::EbmlHead element. Must be the first one.
  auto l0 = ebml_element_cptr{ p->m_es->FindNextID(EBML_INFO(libebml::EbmlHead), 0xFFFFFFFFL) };
  if (!l0 || !is_type<libebml::EbmlHead>(*l0)) {
    ui_show_error(fmt::format("{0} {1}", Y("No EBML head found."), Y("This file is probably not a Matroska file.")));
    return result_e::failed;
  }

  int upper_lvl_el           = 0;
  libebml::EbmlElement *element_found = nullptr;

  read_master(static_cast<libebml::EbmlMaster *>(l0.get()), EBML_CONTEXT(l0.get()), upper_lvl_el, element_found);
  delete element_found;

  retain_element(l0);

  handle_elements_generic(*l0);
  l0->SkipData(*p->m_es, EBML_CONTEXT(l0.get()));

  while (true) {
    // NEXT element must be a segment
    l0 = ebml_element_cptr{ p->m_es->FindNextID(EBML_INFO(libmatroska::KaxSegment), 0xFFFFFFFFFFFFFFFFLL) };
    if (!l0)
      break;

    retain_element(l0);

    if (!is_type<libmatroska::KaxSegment>(*l0)) {
      handle_elements_generic(*l0);
      l0->SkipData(*p->m_es, EBML_CONTEXT(l0.get()));

      continue;
    }

    auto result = handle_segment(static_cast<libmatroska::KaxSegment &>(*l0));
    if (result == result_e::aborted)
      return result;

    l0->SkipData(*p->m_es, EBML_CONTEXT(l0.get()));

    if (!p->m_continue_at_cluster && !p->m_show_summary)
      break;
  }

  if (!p->m_use_gui && p->m_show_track_info)
    display_track_info();

  return result_e::succeeded;
}

kax_info_c::result_e
kax_info_c::open_and_process_file(std::string const &file_name) {
  p_func()->m_source_file_name = file_name;
  return open_and_process_file();
}

kax_info_c::result_e
kax_info_c::open_and_process_file() {
  auto p = p_func();

  at_scope_exit_c cleanup([this]() { reset(); });

  reset();

  // open input file
  try {
    p->m_in = std::make_shared<mm_read_buffer_io_c>(mm_file_io_c::open(p->m_source_file_name));
  } catch (mtx::mm_io::exception &ex) {
    ui_show_error(fmt::format(FY("Error: Couldn't open source file {0} ({1})."), p->m_source_file_name, ex));
    return result_e::failed;
  }

  at_scope_exit_c close_file([p]() {
    if (p->m_out)
      p->m_out->close();
    p->m_out.reset();
  });

  // open output file
  if (!p->m_destination_file_name.empty()) {
    try {
      p->m_out = std::make_shared<mm_write_buffer_io_c>(mm_file_io_c::open(p->m_destination_file_name, libebml::MODE_CREATE), 1024 * 1024);

    } catch (mtx::mm_io::exception &ex) {
      ui_show_error(fmt::format(FY("The file '{0}' could not be opened for writing: {1}."), p->m_destination_file_name, ex));
      return result_e::failed;
    }
  }

  try {
    auto result = process_file();
    return result;

  } catch (mtx::kax_info::exception &) {
    throw;

  } catch (std::exception &ex) {
    ui_show_error(fmt::format("{0}: {1}", Y("Caught exception"), ex.what()));
    return result_e::failed;

  } catch (...) {
    ui_show_error(Y("Caught exception"));
    return result_e::failed;
  }
}

void
kax_info_c::abort() {
  p_func()->m_abort = true;
}

void
kax_info_c::retain_element(std::shared_ptr<libebml::EbmlElement> const &element) {
  auto p = p_func();

  if (p->m_retain_elements)
    p->m_retained_elements.push_back(element);
}

void
kax_info_c::discard_retained_element(libebml::EbmlElement &element_to_remove) {
  auto p = p_func();

  p->m_retained_elements.erase(std::remove_if(p->m_retained_elements.begin(), p->m_retained_elements.end(),
                                              [&element_to_remove](auto const &retained_element) { return retained_element.get() == &element_to_remove; }),
                               p->m_retained_elements.end());
}

}
