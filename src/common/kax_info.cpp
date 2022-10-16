/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include <QDateTime>

#include <ebml/EbmlDummy.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>
#include <ebml/EbmlCrc32.h>
#include <matroska/FileKax.h>

#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxContentEncoding.h>
#include <matroska/KaxCues.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTag.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>
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

using namespace libmatroska;
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
kax_info_c::ui_show_element(EbmlElement &e) {
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
kax_info_c::show_element(EbmlElement *l,
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
                       :                      static_cast<int64_t>(l->GetSizeLength() + EBML_ID_LENGTH(static_cast<const EbmlId &>(*l)) + l->GetSize()),
                         !l                 ? std::optional<int64_t>{}
                       : !l->IsFiniteSize() ? std::optional<int64_t>{}
                       :                      l->GetSize());
}

std::string
kax_info_c::format_ebml_id_as_hex(uint32_t id) {
  return fmt::format("{0:x}", id);
}

std::string
kax_info_c::format_ebml_id_as_hex(EbmlId const &id) {
  return format_ebml_id_as_hex(EBML_ID_VALUE(id));
}

std::string
kax_info_c::format_ebml_id_as_hex(EbmlElement &e) {
  return format_ebml_id_as_hex(static_cast<const EbmlId &>(e));
}

std::string
kax_info_c::create_unknown_element_text(EbmlElement &e) {
  return fmt::format(Y("(Unknown element: {0}; ID: 0x{1} size: {2})"), EBML_NAME(&e), format_ebml_id_as_hex(e), (e.GetSize() + e.HeadSize()));
}

std::string
kax_info_c::create_known_element_but_not_allowed_here_text(EbmlElement &e) {
  return fmt::format(Y("(Known element, but invalid at this position: {0}; ID: 0x{1} size: {2})"), kax_element_names_c::get(e), format_ebml_id_as_hex(e), e.GetSize() + e.HeadSize());
}

std::string
kax_info_c::create_element_text(const std::string &text,
                                std::optional<int64_t> position,
                                std::optional<int64_t> size,
                                std::optional<int64_t> data_size) {
  auto p = p_func();

  std::string additional_text;

  if (position && p->m_show_positions)
    additional_text += fmt::format(p->m_hex_positions ? Y(" at 0x{0:x}") : Y(" at {0}"), *position);

  if (p->m_show_size && size) {
    if (-2 != *size)
      additional_text += fmt::format(Y(" size {0}"), *size);
    else
      additional_text += Y(" size is unknown");
  }

  if (p->m_show_size && data_size)
    additional_text += fmt::format(Y(" data size {0}"), *data_size);

  return text + additional_text;
}

std::string
kax_info_c::create_text_representation(EbmlElement &e) {
  auto text = kax_element_names_c::get(e);

  if (text.empty())
    text = create_unknown_element_text(e);

  else if (dynamic_cast<EbmlDummy *>(&e))
    text = create_known_element_but_not_allowed_here_text(e);

  else {
    auto value = format_element_value(e);
    if (!value.empty())
      text += ": "s + value;
  }

  auto size      = e.IsFiniteSize() ? e.HeadSize() + e.GetSize()          : -2ll;
  auto data_size = e.IsFiniteSize() ? std::optional<int64_t>{e.GetSize()} : std::optional<int64_t>{};

  return create_element_text(text, e.GetElementPosition(), size, data_size);
}

std::string
kax_info_c::format_element_value(EbmlElement &e) {
  auto p = p_func();

  auto formatter = p->m_custom_element_value_formatters.find(EbmlId(e).GetValue());

  return (formatter == p->m_custom_element_value_formatters.end()) || dynamic_cast<EbmlDummy *>(&e) ? format_element_value_default(e) : formatter->second(e);
}

std::string
kax_info_c::format_element_value_default(EbmlElement &e) {
  if (Is<EbmlVoid>(e))
    return format_element_size(e);

  if (Is<EbmlCrc32>(e))
    return fmt::format("0x{0:08x}", static_cast<EbmlCrc32 &>(e).GetCrc32());

  if (dynamic_cast<EbmlUInteger *>(&e))
    return fmt::to_string(static_cast<EbmlUInteger &>(e).GetValue());

  if (dynamic_cast<EbmlSInteger *>(&e))
    return fmt::to_string(static_cast<EbmlSInteger &>(e).GetValue());

  if (dynamic_cast<EbmlString *>(&e))
    return static_cast<EbmlString &>(e).GetValue();

  if (dynamic_cast<EbmlUnicodeString *>(&e))
    return static_cast<EbmlUnicodeString &>(e).GetValueUTF8();

  if (dynamic_cast<EbmlBinary *>(&e))
    return format_binary(static_cast<EbmlBinary &>(e));

  if (dynamic_cast<EbmlDate *>(&e))
    return mtx::date_time::format(QDateTime::fromSecsSinceEpoch(static_cast<EbmlDate &>(e).GetEpochDate(), Qt::UTC), "%Y-%m-%d %H:%M:%S UTC");

  if (dynamic_cast<EbmlMaster *>(&e))
    return {};

  if (dynamic_cast<EbmlFloat *>(&e))
    return mtx::string::normalize_fmt_double_output(static_cast<EbmlFloat &>(e).GetValue());

  throw std::invalid_argument{"format_element_value: unsupported EbmlElement type"};
}

std::string
kax_info_c::create_hexdump(unsigned char const *buf,
                           int size) {
  std::string hex(" hexdump");
  int bmax = std::min(size, p_func()->m_hexdump_max_size);
  int b;

  for (b = 0; b < bmax; ++b)
    hex += fmt::format(" {0:02x}", static_cast<int>(buf[b]));

  return hex;
}

std::string
kax_info_c::create_codec_dependent_private_info(KaxCodecPrivate &c_priv,
                                                char track_type,
                                                std::string const &codec_id) {
  if ((codec_id == MKV_V_MSCOMP) && ('v' == track_type) && (c_priv.GetSize() >= sizeof(alBITMAPINFOHEADER))) {
    auto bih = reinterpret_cast<alBITMAPINFOHEADER *>(c_priv.GetBuffer());
    return fmt::format(Y(" (FourCC: {0})"), fourcc_c{&bih->bi_compression}.description());

  } else if ((codec_id == MKV_A_ACM) && ('a' == track_type) && (c_priv.GetSize() >= sizeof(alWAVEFORMATEX))) {
    auto wfe = reinterpret_cast<alWAVEFORMATEX *>(c_priv.GetBuffer());
    return fmt::format(Y(" (format tag: 0x{0:04x})"), get_uint16_le(&wfe->w_format_tag));

  } else if ((codec_id == MKV_V_MPEG4_AVC) && ('v' == track_type) && (c_priv.GetSize() >= 4)) {
    auto avcc = mtx::avc::avcc_c::unpack(memory_c::borrow(c_priv.GetBuffer(), c_priv.GetSize()));

    return fmt::format(Y(" (H.264 profile: {0} @L{1}.{2})"),
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

    return fmt::format(Y(" (HEVC profile: {0} @L{1}.{2})"),
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
kax_info_c::read_master(EbmlMaster *m,
                        EbmlSemanticContext const &ctx,
                        int &upper_lvl_el,
                        EbmlElement *&l2) {
  m->Read(*p_func()->m_es, ctx, upper_lvl_el, l2, true);
  if (m->ListSize() == 0)
    return;

  std::sort(m->begin(), m->end(), [](auto const *a, auto const *b) { return a->GetElementPosition() < b->GetElementPosition(); });
}

std::string
kax_info_c::format_binary(EbmlBinary &bin) {
  auto &p      = *p_func();
  auto len     = std::min<std::size_t>(p.m_hexdump_max_size, bin.GetSize());
  auto const b = bin.GetBuffer();
  auto result  = fmt::format(Y("length {0}, data: {1}"), bin.GetSize(), mtx::string::to_hex(b, len));

  if (len < bin.GetSize())
    result += u8"…";

  if (p.m_calc_checksums)
    result += fmt::format(Y(" (adler: 0x{0:08x})"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, bin.GetBuffer(), bin.GetSize()));

  mtx::string::strip(result);

  return result;
}

std::string
kax_info_c::format_binary_as_hex(EbmlElement &e) {
  return mtx::string::to_hex(static_cast<EbmlBinary &>(e));
}

std::string
kax_info_c::format_element_size(EbmlElement &e) {
  if (e.IsFiniteSize())
    return fmt::format(Y("size {0}"), e.GetSize());
  return Y("size unknown");
}

std::string
kax_info_c::format_unsigned_integer_as_timestamp(EbmlElement &e) {
  return mtx::string::format_timestamp(static_cast<EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_unsigned_integer_as_scaled_timestamp(EbmlElement &e) {
  return mtx::string::format_timestamp(p_func()->m_ts_scale * static_cast<EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_timestamp(EbmlElement &e) {
  return mtx::string::format_timestamp(static_cast<EbmlSInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_scaled_timestamp(EbmlElement &e) {
  return mtx::string::format_timestamp(p_func()->m_ts_scale * static_cast<EbmlSInteger &>(e).GetValue());
}

void
kax_info_c::init_custom_element_value_formatters_and_processors() {
  using pre_mem_fn_t  = bool (kax_info_c::*)(EbmlElement &);
  using post_mem_fn_t = void (kax_info_c::*)(EbmlElement &);
  using fmt_mem_fn_t  = std::string (kax_info_c::*)(EbmlElement &);

  auto p = p_func();

  auto add_pre = [&p](EbmlId const &id, std::function<bool(EbmlElement &)> const &processor) {
    p->m_custom_element_pre_processors.insert({ id.GetValue(), processor });
  };

  auto add_pre_mem = [this, &p](EbmlId const &id, pre_mem_fn_t processor) {
    p->m_custom_element_pre_processors.insert({ id.GetValue(), std::bind(processor, this, std::placeholders::_1) });
  };

  auto add_post = [&p](EbmlId const &id, std::function<void(EbmlElement &)> const &processor) {
    p->m_custom_element_post_processors.insert({ id.GetValue(), processor });
  };

  auto add_post_mem = [this, &p](EbmlId const &id, post_mem_fn_t processor) {
    p->m_custom_element_post_processors.insert({ id.GetValue(), std::bind(processor, this, std::placeholders::_1) });
  };

  auto add_fmt = [&p](EbmlId const &id, std::function<std::string(EbmlElement &)> const &formatter) {
    p->m_custom_element_value_formatters.insert({ id.GetValue(), formatter });
  };

  auto add_fmt_mem = [this, &p](EbmlId const &id, fmt_mem_fn_t formatter) {
    p->m_custom_element_value_formatters.insert({ id.GetValue(), std::bind(formatter, this, std::placeholders::_1) });
  };

  p->m_custom_element_value_formatters.clear();
  p->m_custom_element_pre_processors.clear();
  p->m_custom_element_post_processors.clear();

  // Simple processors:
  add_pre(    EBML_ID(KaxInfo),        [p](EbmlElement &e) -> bool { p->m_ts_scale = FindChildValue<KaxTimecodeScale>(static_cast<KaxInfo &>(e), TIMESTAMP_SCALE); return true; });
  add_pre(    EBML_ID(KaxTracks),      [p](EbmlElement &)  -> bool { p->m_mkvmerge_track_id = 0; return true; });
  add_pre_mem(EBML_ID(KaxSimpleBlock), &kax_info_c::pre_simple_block);
  add_pre_mem(EBML_ID(KaxBlockGroup),  &kax_info_c::pre_block_group);
  add_pre_mem(EBML_ID(KaxBlock),       &kax_info_c::pre_block);

  // More complex processors:
  add_pre(EBML_ID(KaxSeekHead), ([this, p](EbmlElement &e) -> bool {
    if (!p->m_show_all_elements && !p->m_use_gui)
      show_element(&e, p->m_level, Y("Seek head (subentries will be skipped)"));
    return p->m_show_all_elements || p->m_use_gui;
  }));

  add_pre(EBML_ID(KaxCues), ([this, p](EbmlElement &e) -> bool {
    if (!p->m_show_all_elements && !p->m_use_gui)
      show_element(&e, p->m_level, Y("Cues (subentries will be skipped)"));
    return p->m_show_all_elements || p->m_use_gui;
  }));

  add_pre(EBML_ID(KaxTrackEntry), [p](EbmlElement &e) -> bool {
    p->m_summary.clear();

    auto &master                 = static_cast<EbmlMaster &>(e);
    p->m_track                   = std::make_shared<kax_info::track_t>();
    p->m_track->tuid             = FindChildValue<KaxTrackUID>(master);
    p->m_track->codec_id         = FindChildValue<KaxCodecID>(master);
    p->m_track->default_duration = FindChildValue<KaxTrackDefaultDuration>(master);

    return true;
  });

  add_pre(EBML_ID(KaxTrackNumber), ([this, p](EbmlElement &e) -> bool {
    p->m_track->tnum    = static_cast<KaxTrackNumber &>(e).GetValue();
    auto existing_track = find_track(p->m_track->tnum);
    auto track_id       = p->m_mkvmerge_track_id;

    if (!existing_track) {
      p->m_track->mkvmerge_track_id = p->m_mkvmerge_track_id;
      ++p->m_mkvmerge_track_id;
      add_track(p->m_track);

    } else
      track_id = existing_track->mkvmerge_track_id;

    p->m_summary.push_back(fmt::format(Y("mkvmerge/mkvextract track ID: {0}"), track_id));

    p->m_track_by_element[&e] = p->m_track;

    return true;
  }));

  add_pre(EBML_ID(KaxTrackType), [p](EbmlElement &e) -> bool {
    auto ttype       = static_cast<KaxTrackType &>(e).GetValue();
    p->m_track->type = track_audio    == ttype ? 'a'
                     : track_video    == ttype ? 'v'
                     : track_subtitle == ttype ? 's'
                     : track_buttons  == ttype ? 'b'
                     :                           '?';
    return true;
  });

  add_pre(EBML_ID(KaxCodecPrivate), ([this, p](EbmlElement &e) -> bool {
    auto &c_priv        = static_cast<KaxCodecPrivate &>(e);
    p->m_track-> fourcc = create_codec_dependent_private_info(c_priv, p->m_track->type, p->m_track->codec_id);

    if (p->m_calc_checksums && !p->m_show_summary)
      p->m_track->fourcc += fmt::format(Y(" (adler: 0x{0:08x})"), mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, c_priv.GetBuffer(), c_priv.GetSize()));

    if (p->m_show_hexdump)
      p->m_track->fourcc += create_hexdump(c_priv.GetBuffer(), c_priv.GetSize());

    p->m_track_by_element[&e] = p->m_track;

    return true;
  }));

  add_pre(EBML_ID(KaxCluster), ([this, p](EbmlElement &e) -> bool {
    p->m_cluster = static_cast<KaxCluster *>(&e);
    p->m_cluster->InitTimecode(FindChildValue<KaxClusterTimecode>(p->m_cluster), p->m_ts_scale);

    ui_show_progress(100 * p->m_cluster->GetElementPosition() / p->m_file_size, Y("Parsing file"));

    return true;
  }));

  add_post(EBML_ID(KaxAudioSamplingFreq),       [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("sampling freq: {0}"), mtx::string::normalize_fmt_double_output(static_cast<KaxAudioSamplingFreq &>(e).GetValue()))); });
  add_post(EBML_ID(KaxAudioOutputSamplingFreq), [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("output sampling freq: {0}"), mtx::string::normalize_fmt_double_output(static_cast<KaxAudioOutputSamplingFreq &>(e).GetValue()))); });
  add_post(EBML_ID(KaxAudioChannels),           [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("channels: {0}"), static_cast<KaxAudioChannels &>(e).GetValue())); });
  add_post(EBML_ID(KaxAudioBitDepth),           [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("bits per sample: {0}"), static_cast<KaxAudioBitDepth &>(e).GetValue())); });

  add_post(EBML_ID(KaxVideoPixelWidth),         [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel width: {0}"), static_cast<KaxVideoPixelWidth &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoPixelHeight),        [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel height: {0}"), static_cast<KaxVideoPixelHeight &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoDisplayWidth),       [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("display width: {0}"), static_cast<KaxVideoDisplayWidth &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoDisplayHeight),      [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("display height: {0}"), static_cast<KaxVideoDisplayHeight &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoPixelCropLeft),      [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel crop left: {0}"), static_cast<KaxVideoPixelCropLeft &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoPixelCropTop),       [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel crop top: {0}"), static_cast<KaxVideoPixelCropTop &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoPixelCropRight),     [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel crop right: {0}"), static_cast<KaxVideoPixelCropRight &>(e).GetValue())); });
  add_post(EBML_ID(KaxVideoPixelCropBottom),    [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("pixel crop bottom: {0}"), static_cast<KaxVideoPixelCropBottom &>(e).GetValue())); });
  add_post(EBML_ID(KaxTrackLanguage),           [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("language: {0}"), static_cast<KaxTrackLanguage &>(e).GetValue())); });
  add_post(EBML_ID(KaxLanguageIETF),            [p](EbmlElement &e) { p->m_summary.push_back(fmt::format(Y("language (IETF BCP 47): {0}"), static_cast<KaxLanguageIETF &>(e).GetValue())); });
  add_post(EBML_ID(KaxBlockDuration),           [p](EbmlElement &e) { p->m_block_duration = static_cast<double>(static_cast<KaxBlockDuration &>(e).GetValue()) * p->m_ts_scale; });
  add_post(EBML_ID(KaxReferenceBlock),          [p](EbmlElement &)  { ++p->m_num_references; });

  add_post_mem(EBML_ID(KaxSimpleBlock),         &kax_info_c::post_simple_block);
  add_post_mem(EBML_ID(KaxBlock),               &kax_info_c::post_block);
  add_post_mem(EBML_ID(KaxBlockGroup),          &kax_info_c::post_block_group);

  add_post(EBML_ID(KaxTrackDefaultDuration),    [p](EbmlElement &) {
    p->m_summary.push_back(fmt::format(Y("default duration: {0:.3f}ms ({1:.3f} frames/fields per second for a video track)"),
                                       static_cast<double>(p->m_track->default_duration) / 1'000'000.0,
                                       1'000'000'000.0 / static_cast<double>(p->m_track->default_duration)));
  });

  add_post(EBML_ID(KaxTrackEntry), [p](EbmlElement &) {
    if (!p->m_show_summary)
      return;

    p->m_out->puts(fmt::format(Y("Track {0}: {1}, codec ID: {2}{3}{4}{5}\n"),
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

  // Simple formatters:
  add_fmt_mem(EBML_ID(KaxSegmentUID),       &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(KaxSegmentFamily),    &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(KaxNextUID),          &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(KaxSegmentFamily),    &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(KaxPrevUID),          &kax_info_c::format_binary_as_hex);
  add_fmt_mem(EBML_ID(KaxSegment),          &kax_info_c::format_element_size);
  add_fmt_mem(EBML_ID(KaxFileData),         &kax_info_c::format_element_size);
  add_fmt_mem(EBML_ID(KaxChapterTimeStart), &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(KaxChapterTimeEnd),   &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(KaxCueDuration),      &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxCueTime),          &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxCueRefTime),       &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxCodecDelay),       &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(KaxSeekPreRoll),      &kax_info_c::format_unsigned_integer_as_timestamp);
  add_fmt_mem(EBML_ID(KaxClusterTimecode),  &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxSliceDelay),       &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxSliceDuration),    &kax_info_c::format_signed_integer_as_timestamp);
  add_fmt_mem(EBML_ID(KaxSimpleBlock),      &kax_info_c::format_simple_block);
  add_fmt_mem(EBML_ID(KaxBlock),            &kax_info_c::format_block);
  add_fmt_mem(EBML_ID(KaxBlockDuration),    &kax_info_c::format_unsigned_integer_as_scaled_timestamp);
  add_fmt_mem(EBML_ID(KaxReferenceBlock),   &kax_info_c::format_signed_integer_as_scaled_timestamp);

  add_fmt(EBML_ID(KaxDuration),             [p](EbmlElement &e) { return mtx::string::format_timestamp(static_cast<int64_t>(static_cast<EbmlFloat &>(e).GetValue() * p->m_ts_scale)); });

  // More complex formatters:
  add_fmt(EBML_ID(KaxSeekID), [](EbmlElement &e) -> std::string {
    auto &seek_id = static_cast<KaxSeekID &>(e);
    EbmlId id(seek_id.GetBuffer(), seek_id.GetSize());

    return fmt::format("{0} ({1})",
                       mtx::string::to_hex(seek_id),
                         Is<KaxInfo>(id)        ? "KaxInfo"
                       : Is<KaxCluster>(id)     ? "KaxCluster"
                       : Is<KaxTracks>(id)      ? "KaxTracks"
                       : Is<KaxCues>(id)        ? "KaxCues"
                       : Is<KaxAttachments>(id) ? "KaxAttachments"
                       : Is<KaxChapters>(id)    ? "KaxChapters"
                       : Is<KaxTags>(id)        ? "KaxTags"
                       : Is<KaxSeekHead>(id)    ? "KaxSeekHead"
                       :                          "unknown");
  });

  add_fmt(EBML_ID(KaxEmphasis), [](EbmlElement &e) -> std::string {
    auto audio_emphasis = static_cast<KaxEmphasis &>(e).GetValue();
    return fmt::format("{0} ({1})", audio_emphasis, audio_emphasis_c::translate(static_cast<audio_emphasis_c::mode_e>(audio_emphasis)));
  });

  add_fmt(EBML_ID(KaxVideoProjectionType), [](EbmlElement &e) -> std::string {
    auto value       = static_cast<KaxVideoProjectionType &>(e).GetValue();
    auto description = 0 == value ? Y("rectangular")
                     : 1 == value ? Y("equirectangular")
                     : 2 == value ? Y("cubemap")
                     : 3 == value ? Y("mesh")
                     :              Y("unknown");

    return fmt::format("{0} ({1})", value, description);
  });

  add_fmt(EBML_ID(KaxVideoDisplayUnit), [](EbmlElement &e) -> std::string {
      auto unit = static_cast<KaxVideoDisplayUnit &>(e).GetValue();
      return fmt::format("{0}{1}",
                         unit,
                           0 == unit ? Y(" (pixels)")
                         : 1 == unit ? Y(" (centimeters)")
                         : 2 == unit ? Y(" (inches)")
                         : 3 == unit ? Y(" (aspect ratio)")
                         :               "");
  });

  add_fmt(EBML_ID(KaxVideoFieldOrder), [](EbmlElement &e) -> std::string {
    auto field_order = static_cast<KaxVideoFieldOrder &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       field_order,
                          0 == field_order ? Y("progressive")
                       :  1 == field_order ? Y("top field displayed first, top field stored first")
                       :  2 == field_order ? Y("unspecified")
                       :  6 == field_order ? Y("bottom field displayed first, bottom field stored first")
                       :  9 == field_order ? Y("bottom field displayed first, top field stored first")
                       : 14 == field_order ? Y("top field displayed first, bottom field stored first")
                       :                     Y("unknown"));
  });

  add_fmt(EBML_ID(KaxVideoStereoMode), [](EbmlElement &e) -> std::string {
    auto stereo_mode = static_cast<KaxVideoStereoMode &>(e).GetValue();
    return fmt::format("{0} ({1})", stereo_mode, stereo_mode_c::translate(static_cast<stereo_mode_c::mode>(stereo_mode)));
  });

  add_fmt(EBML_ID(KaxVideoAspectRatio), [](EbmlElement &e) -> std::string {
    auto ar_type = static_cast<KaxVideoAspectRatio &>(e).GetValue();
    return fmt::format("{0}{1}",
                       ar_type,
                         0 == ar_type ? Y(" (free resizing)")
                       : 1 == ar_type ? Y(" (keep aspect ratio)")
                       : 2 == ar_type ? Y(" (fixed)")
                       :                  "");
  });

  add_fmt(EBML_ID(KaxContentEncodingScope), [](EbmlElement &e) -> std::string {
    std::vector<std::string> scope;
    auto ce_scope = static_cast<KaxContentEncodingScope &>(e).GetValue();

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

  add_fmt(EBML_ID(KaxContentEncodingType), [](EbmlElement &e) -> std::string {
    auto ce_type = static_cast<KaxContentEncodingType &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       ce_type,
                         0 == ce_type ? Y("compression")
                       : 1 == ce_type ? Y("encryption")
                       :                Y("unknown"));
  });

  add_fmt(EBML_ID(KaxContentCompAlgo), [](EbmlElement &e) -> std::string {
    auto c_algo = static_cast<KaxContentCompAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       c_algo,
                         0 == c_algo ?   "ZLIB"
                       : 1 == c_algo ?   "bzLib"
                       : 2 == c_algo ?   "lzo1x"
                       : 3 == c_algo ? Y("header removal")
                       :               Y("unknown"));
  });

  add_fmt(EBML_ID(KaxContentEncAlgo), [](EbmlElement &e) -> std::string {
    auto e_algo = static_cast<KaxContentEncAlgo &>(e).GetValue();
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

  add_fmt(EBML_ID(KaxContentSigAlgo), [](EbmlElement &e) -> std::string {
    auto s_algo = static_cast<KaxContentSigAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       s_algo,
                         0 == s_algo ? Y("no signature algorithm")
                       : 1 == s_algo ? Y("RSA")
                       :               Y("unknown"));
  });

  add_fmt(EBML_ID(KaxContentSigHashAlgo), [](EbmlElement &e) -> std::string {
    auto s_halgo = static_cast<KaxContentSigHashAlgo &>(e).GetValue();
    return fmt::format("{0} ({1})",
                       s_halgo,
                         0 == s_halgo ? Y("no signature hash algorithm")
                       : 1 == s_halgo ? Y("SHA1-160")
                       : 2 == s_halgo ? Y("MD5")
                       :                Y("unknown"));
  });

  add_fmt(EBML_ID(KaxTrackNumber), [p](EbmlElement &e) -> std::string { return fmt::format(Y("{0} (track ID for mkvmerge & mkvextract: {1})"), p->m_track_by_element[&e]->tnum, p->m_track_by_element[&e]->mkvmerge_track_id); });

  add_fmt(EBML_ID(KaxTrackType), [](EbmlElement &e) -> std::string {
    auto ttype = static_cast<KaxTrackType &>(e).GetValue();
    return track_audio    == ttype ? Y("audio")
         : track_video    == ttype ? Y("video")
         : track_subtitle == ttype ? Y("subtitles")
         : track_buttons  == ttype ? Y("buttons")
         :                           Y("unknown");
  });

  add_fmt(EBML_ID(KaxCodecPrivate), [p](EbmlElement &e) -> std::string {
    return fmt::format(Y("size {0}"), e.GetSize()) + p->m_track_by_element[&e]->fourcc;
  });

  add_fmt(EBML_ID(KaxTrackDefaultDuration), [](EbmlElement &e) -> std::string {
    auto default_duration = static_cast<KaxTrackDefaultDuration &>(e).GetValue();
    return fmt::format(Y("{0} ({1:.3f} frames/fields per second for a video track)"),
                       mtx::string::format_timestamp(default_duration),
                       1'000'000'000.0 / static_cast<double>(default_duration));
  });

  add_fmt(EBML_ID(KaxBlockAddIDType), [](auto &e) -> std::string {
    auto value = static_cast<EbmlUInteger &>(e).GetValue();
    if (value < std::numeric_limits<uint32_t>::max())
      return fmt::format("{0} ({1})", value, fourcc_c{static_cast<uint32_t>(value)});
    return fmt::format("{0}", value);
  });

  add_fmt(EBML_ID(KaxChapterSkipType), [](EbmlElement &e) -> std::string {
    auto value       = static_cast<EbmlUInteger &>(e).GetValue();
    auto description = value == MATROSKA_CHAPTERSKIPTYPE_NO_SKIPPING     ? Y("no skipping")
                     : value == MATROSKA_CHAPTERSKIPTYPE_OPENING_CREDITS ? Y("opening credits")
                     : value == MATROSKA_CHAPTERSKIPTYPE_END_CREDITS     ? Y("end credits")
                     : value == MATROSKA_CHAPTERSKIPTYPE_RECAP           ? Y("recap")
                     : value == MATROSKA_CHAPTERSKIPTYPE_NEXT_PREVIEW    ? Y("preview of next episode")
                     : value == MATROSKA_CHAPTERSKIPTYPE_PREVIEW         ? Y("preview of current episode")
                     : value == MATROSKA_CHAPTERSKIPTYPE_ADVERTISEMENT   ? Y("advertisement")
                     :                                                     Y("unknown");

    return fmt::format("{0} ({1})", value, description);
  });
}

bool
kax_info_c::pre_block(EbmlElement &e) {
  auto p = p_func();

  auto &block = static_cast<KaxBlock &>(e);

  block.SetParent(*p->m_cluster);

  p->m_lf_timestamp   = block.GlobalTimecode();
  p->m_lf_tnum        = block.TrackNum();
  p->m_block_duration = std::nullopt;

  return true;
}

std::string
kax_info_c::format_block(EbmlElement &e) {
  auto p = p_func();

  auto &block = static_cast<KaxBlock &>(e);

  return fmt::format(Y("track number {0}, {1} frame(s), timestamp {2}"), block.TrackNum(), block.NumberFrames(), mtx::string::format_timestamp(p->m_lf_timestamp));
}

void
kax_info_c::post_block(EbmlElement &e) {
  auto p         = p_func();
  auto &block    = static_cast<KaxBlock &>(e);
  auto frame_pos = e.GetElementPosition() + e.ElementSize();

  for (int i = 0, num_frames = block.NumberFrames(); i < num_frames; ++i)
    frame_pos -= block.GetBuffer(i).Size();

  for (int i = 0, num_frames = block.NumberFrames(); i < num_frames; ++i) {
    auto &data = block.GetBuffer(i);
    auto adler = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (p->m_calc_checksums)
      adler_str = fmt::format(Y(" (adler: 0x{0:08x})"), adler);

    std::string hex;
    if (p->m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    auto text = p->m_show_size ? fmt::format(Y("Frame{0}{1}"),                            adler_str, hex)
              :                  fmt::format(Y("Frame with size {0}{1}{2}"), data.Size(), adler_str, hex);
    show_element(nullptr, p->m_level + 1, text, frame_pos, data.Size());

    p->m_frame_sizes.push_back(data.Size());
    p->m_frame_adlers.push_back(adler);
    p->m_frame_hexdumps.push_back(hex);

    frame_pos += data.Size();
  }
}

void
kax_info_c::show_frame_summary(EbmlElement &e) {
  auto p         = p_func();
  auto frame_pos = e.GetElementPosition() + e.ElementSize();

  for (auto size : p->m_frame_sizes)
    frame_pos -= size;

  for (auto fidx = 0u; fidx < p->m_frame_sizes.size(); fidx++) {
    std::string position;

    if (p->m_show_positions) {
      position   = fmt::format(p->m_hex_positions ? Y(", position 0x{0:x}") : Y(", position {0}"), frame_pos);
      frame_pos += p->m_frame_sizes[fidx];
    }

    if (p->m_block_duration)
      p->m_out->puts(fmt::format(Y("{0} frame, track {1}, timestamp {2}, duration {3}, size {4}, adler 0x{5:08x}{6}{7}\n"),
                                 (p->m_num_references >= 2 ? 'B' : p->m_num_references == 1 ? 'P' : 'I'),
                                 p->m_lf_tnum,
                                 mtx::string::format_timestamp(p->m_lf_timestamp),
                                 mtx::string::format_timestamp(*p->m_block_duration),
                                 p->m_frame_sizes[fidx],
                                 p->m_frame_adlers[fidx],
                                 p->m_frame_hexdumps[fidx],
                                 position));
    else
      p->m_out->puts(fmt::format(Y("{0} frame, track {1}, timestamp {2}, size {3}, adler 0x{4:08x}{5}{6}\n"),
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
kax_info_c::pre_block_group(EbmlElement &) {
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
kax_info_c::post_block_group(EbmlElement &e) {
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
kax_info_c::pre_simple_block(EbmlElement &e) {
  auto p = p_func();

  p->m_frame_sizes.clear();
  p->m_frame_adlers.clear();
  p->m_frame_hexdumps.clear();

  static_cast<KaxSimpleBlock &>(e).SetParent(*p->m_cluster);

  return true;
}

std::string
kax_info_c::format_simple_block(EbmlElement &e) {
  auto &block       = static_cast<KaxSimpleBlock &>(e);
  auto timestamp_ns = mtx::math::to_signed(block.GlobalTimecode());

  std::string info;
  if (block.IsKeyframe())
    info = Y("key, ");
  if (block.IsDiscardable())
    info += Y("discardable, ");

  return fmt::format(Y("{0}track number {1}, {2} frame(s), timestamp {3}"),
                     info,
                     block.TrackNum(),
                     block.NumberFrames(),
                     mtx::string::format_timestamp(timestamp_ns));
}

void
kax_info_c::post_simple_block(EbmlElement &e) {
  auto p            = p_func();

  auto &block       = static_cast<KaxSimpleBlock &>(e);
  auto &tinfo       = p->m_track_info[block.TrackNum()];
  auto timestamp_ns = mtx::math::to_signed(block.GlobalTimecode());
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
      adler_str = fmt::format(Y(" (adler: 0x{0:08x})"), adler);

    std::string hex;
    if (p->m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    auto text = p->m_show_size ? fmt::format(Y("Frame{0}{1}"),                            adler_str, hex)
              :                  fmt::format(Y("Frame with size {0}{1}{2}"), data.Size(), adler_str, hex);
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
        position   = fmt::format(p->m_hex_positions ? Y(", position 0x{0:x}") : Y(", position {0}"), frame_pos);
        frame_pos += p->m_frame_sizes[idx];
      }

      p->m_out->puts(fmt::format(Y("{0} frame, track {1}, timestamp {2}, size {3}, adler 0x{4:08x}{5}\n"),
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
kax_info_c::handle_segment(EbmlElement *l0) {
  ui_show_element(*l0);

  auto p        = p_func();
  auto l1       = std::shared_ptr<EbmlElement>{};
  auto kax_file = std::make_shared<kax_file_c>(*p->m_in);
  p->m_level    = 1;

  kax_file->set_segment_end(*l0);

  // Prevent reporting "first timestamp after resync":
  kax_file->set_timestamp_scale(-1);

  while ((l1 = kax_file->read_next_level1_element())) {
    retain_element(l1);

    if (Is<KaxCluster>(*l1) && !p->m_continue_at_cluster && !p->m_show_summary) {
      ui_show_element(*l1);
      return result_e::succeeded;

    } else
      handle_elements_generic(*l1);

    if (!p->m_in->setFilePointer2(l1->GetElementPosition() + kax_file->get_element_size(*l1)))
      break;

    auto in_parent = !l0->IsFiniteSize()
                  || (p->m_in->getFilePointer() < (l0->GetElementPosition() + l0->HeadSize() + l0->GetSize()));
    if (!in_parent)
      break;

    if (p->m_abort)
      return result_e::aborted;

  } // while (l1)

  return result_e::succeeded;
}

bool
kax_info_c::run_generic_pre_processors(EbmlElement &e) {
  auto p = p_func();

  auto is_dummy = !!dynamic_cast<EbmlDummy *>(&e);
  if (is_dummy)
    return true;

  auto pre_processor = p->m_custom_element_pre_processors.find(EbmlId(e).GetValue());
  if (pre_processor != p->m_custom_element_pre_processors.end())
    if (!pre_processor->second(e))
      return false;

  return true;
}

void
kax_info_c::run_generic_post_processors(EbmlElement &e) {
  auto p = p_func();

  auto is_dummy = !!dynamic_cast<EbmlDummy *>(&e);
  if (is_dummy)
    return;

  auto post_processor = p->m_custom_element_post_processors.find(EbmlId(e).GetValue());
  if (post_processor != p->m_custom_element_post_processors.end())
    post_processor->second(e);
}

void
kax_info_c::handle_elements_generic(EbmlElement &e) {
  auto p = p_func();

  if (!run_generic_pre_processors(e))
    return;

  ui_show_element(e);

  if (dynamic_cast<EbmlMaster *>(&e)) {
    ++p->m_level;

    for (auto child : static_cast<EbmlMaster &>(e))
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

    p->m_out->puts(fmt::format(Y("Statistics for track number {0}: number of blocks: {1}; size in bytes: {2}; duration in seconds: {3}; approximate bitrate in bits/second: {4}\n"),
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
  p->m_es        = std::make_shared<EbmlStream>(*p->m_in);

  // Find the EbmlHead element. Must be the first one.
  auto l0 = ebml_element_cptr{ p->m_es->FindNextID(EBML_INFO(EbmlHead), 0xFFFFFFFFL) };
  if (!l0 || !Is<EbmlHead>(*l0)) {
    ui_show_error(fmt::format("{0} {1}", Y("No EBML head found."), Y("This file is probably not a Matroska file.")));
    return result_e::failed;
  }

  int upper_lvl_el           = 0;
  EbmlElement *element_found = nullptr;

  read_master(static_cast<EbmlMaster *>(l0.get()), EBML_CONTEXT(l0), upper_lvl_el, element_found);
  delete element_found;

  retain_element(l0);

  handle_elements_generic(*l0);
  l0->SkipData(*p->m_es, EBML_CONTEXT(l0));

  while (true) {
    // NEXT element must be a segment
    l0 = ebml_element_cptr{ p->m_es->FindNextID(EBML_INFO(KaxSegment), 0xFFFFFFFFFFFFFFFFLL) };
    if (!l0)
      break;

    retain_element(l0);

    if (!Is<KaxSegment>(*l0)) {
      show_element(l0.get(), 0, Y("Unknown element"));
      l0->SkipData(*p->m_es, EBML_CONTEXT(l0));

      continue;
    }

    auto result = handle_segment(l0.get());
    if (result == result_e::aborted)
      return result;

    l0->SkipData(*p->m_es, EBML_CONTEXT(l0));

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
    ui_show_error(fmt::format(Y("Error: Couldn't open source file {0} ({1})."), p->m_source_file_name, ex));
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
      p->m_out = std::make_shared<mm_write_buffer_io_c>(mm_file_io_c::open(p->m_destination_file_name, MODE_CREATE), 1024 * 1024);

    } catch (mtx::mm_io::exception &ex) {
      ui_show_error(fmt::format(Y("The file '{0}' could not be opened for writing: {1}."), p->m_destination_file_name, ex));
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
kax_info_c::retain_element(std::shared_ptr<EbmlElement> const &element) {
  auto p = p_func();

  if (p->m_retain_elements)
    p->m_retained_elements.push_back(element);
}

void
kax_info_c::discard_retained_element(EbmlElement &element_to_remove) {
  auto p = p_func();

  p->m_retained_elements.erase(std::remove_if(p->m_retained_elements.begin(), p->m_retained_elements.end(),
                                              [&element_to_remove](auto const &retained_element) { return retained_element.get() == &element_to_remove; }),
                               p->m_retained_elements.end());
}

}
