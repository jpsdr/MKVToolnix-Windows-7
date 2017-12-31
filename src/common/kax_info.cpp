/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   retrieves and displays information about a Matroska file

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include <boost/date_time/posix_time/posix_time.hpp>

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
#include "common/at_scope_exit.h"
#include "common/avc.h"
#include "common/avcc.h"
#include "common/chapters/chapters.h"
#include "common/checksums/base.h"
#include "common/codec.h"
#include "common/command_line.h"
#include "common/date_time.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/fourcc.h"
#include "common/hevc.h"
#include "common/hevcc.h"
#include "common/kax_element_names.h"
#include "common/kax_file.h"
#include "common/kax_info.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/stereo_mode.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/translation.h"
#include "common/version.h"
#include "common/xml/ebml_chapters_converter.h"
#include "common/xml/ebml_tags_converter.h"

using namespace libmatroska;

#define in_parent(p) \
  (!p->IsFiniteSize() || \
   (m_in->getFilePointer() < \
    (p->GetElementPosition() + p->HeadSize() + p->GetSize())))

namespace mtx {

std::vector<boost::format> kax_info_c::ms_common_formats;
unsigned int kax_info_c::ms_bf_show_unknown_element              = 0;
unsigned int kax_info_c::ms_bf_format_binary_1                   = 0;
unsigned int kax_info_c::ms_bf_format_binary_2                   = 0;
unsigned int kax_info_c::ms_bf_block_group_block_summary         = 0;
unsigned int kax_info_c::ms_bf_block_group_block_frame           = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_position      = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_with_duration = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_no_duration   = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_v2            = 0;
unsigned int kax_info_c::ms_bf_simple_block_basics               = 0;
unsigned int kax_info_c::ms_bf_simple_block_frame                = 0;
unsigned int kax_info_c::ms_bf_simple_block_summary              = 0;
unsigned int kax_info_c::ms_bf_simple_block_summary_v2           = 0;
unsigned int kax_info_c::ms_bf_at                                = 0;
unsigned int kax_info_c::ms_bf_size                              = 0;
unsigned int kax_info_c::ms_bf_at_hex                            = 0;
unsigned int kax_info_c::ms_bf_block_group_block_adler           = 0;
unsigned int kax_info_c::ms_bf_simple_block_adler                = 0;
unsigned int kax_info_c::ms_bf_simple_block_position             = 0;
unsigned int kax_info_c::ms_bf_crc32_value                       = 0;
unsigned int kax_info_c::ms_bf_element_size                      = 0;

kax_info_c::kax_info_c()
  : m_out{g_mm_stdio}
{
  if (ms_common_formats.empty())
    init_common_formats();

  init_custom_element_value_formatters_and_processors();
}

kax_info_c::~kax_info_c() {
}

void
kax_info_c::set_use_gui(bool enable) {
  m_use_gui = enable;
}

void
kax_info_c::set_calc_checksums(bool enable) {
  m_calc_checksums = enable;
}

void
kax_info_c::set_show_summary(bool enable) {
  m_show_summary = enable;
}

void
kax_info_c::set_show_hexdump(bool enable) {
  m_show_hexdump = enable;
}

void
kax_info_c::set_show_size(bool enable) {
  m_show_size = enable;
}

void
kax_info_c::set_show_track_info(bool enable) {
  m_show_track_info = enable;
}

void
kax_info_c::set_hex_positions(bool enable) {
  m_hex_positions = enable;
}

void
kax_info_c::set_hexdump_max_size(int max_size) {
  m_hexdump_max_size = max_size;
}

void
kax_info_c::set_verbosity(int verbosity) {
  m_verbose = verbosity;
}

void
kax_info_c::set_destination_file_name(std::string const &file_name) {
  m_destination_file_name = file_name;
}

#define BF_ADD(idx, fmt) idx = ms_common_formats.size(); ms_common_formats.push_back(boost::format(fmt))

void
kax_info_c::init_common_formats() {
  ms_common_formats.clear();
  BF_ADD(ms_bf_show_unknown_element,              Y("(Unknown element: %1%; ID: 0x%2% size: %3%)"));
  BF_ADD(ms_bf_format_binary_1,                   Y("length %1%, data: %2%"));
  BF_ADD(ms_bf_format_binary_2,                   Y(" (adler: 0x%|1$08x|)"));
  BF_ADD(ms_bf_block_group_block_summary,         Y("track number %1%, %2% frame(s), timestamp %3%"));
  BF_ADD(ms_bf_block_group_block_frame,           Y("Frame with size %1%%2%%3%"));
  BF_ADD(ms_bf_block_group_summary_position,      Y(", position %1%"));
  BF_ADD(ms_bf_block_group_summary_with_duration, Y("%1% frame, track %2%, timestamp %3%, duration %4%, size %5%, adler 0x%|6$08x|%7%%8%\n"));
  BF_ADD(ms_bf_block_group_summary_no_duration,   Y("%1% frame, track %2%, timestamp %3%, size %4%, adler 0x%|5$08x|%6%%7%\n"));
  BF_ADD(ms_bf_block_group_summary_v2,            Y("[%1% frame for track %2%, timestamp %3%]"));
  BF_ADD(ms_bf_simple_block_basics,               Y("%1%track number %2%, %3% frame(s), timestamp %4%"));
  BF_ADD(ms_bf_simple_block_frame,                Y("Frame with size %1%%2%%3%"));
  BF_ADD(ms_bf_simple_block_summary,              Y("%1% frame, track %2%, timestamp %3%, size %4%, adler 0x%|5$08x|%6%\n"));
  BF_ADD(ms_bf_simple_block_summary_v2,           Y("[%1% frame for track %2%, timestamp %3%]"));
  BF_ADD(ms_bf_at,                                Y(" at %1%"));
  BF_ADD(ms_bf_size,                              Y(" size %1%"));
  BF_ADD(ms_bf_at_hex,                            Y(" at 0x%|1$x|"));
  BF_ADD(ms_bf_crc32_value,                         "0x%|1$08x|");
  BF_ADD(ms_bf_element_size,                      Y("size %1%"));

  ms_bf_block_group_block_adler = ms_bf_format_binary_2;
  ms_bf_simple_block_adler      = ms_bf_format_binary_2;
  ms_bf_simple_block_position   = ms_bf_block_group_summary_position;
}

#undef BF_ADD

void
kax_info_c::ui_show_element_info(int level,
                                 std::string const &text,
                                 int64_t position,
                                 int64_t size) {
  std::string level_buffer(level, ' ');
  level_buffer[0] = '|';

  m_out->write((boost::format("%1%+ %2%\n") % level_buffer % create_element_text(text, position, size)).str());
}

void
kax_info_c::ui_show_element(EbmlElement &e) {
  if (m_show_summary)
    return;

  std::string level_buffer(m_level, ' ');
  level_buffer[0] = '|';

  m_out->write((boost::format("%1%+ %2%\n") % level_buffer % create_text_representation(e)).str());
}

void
kax_info_c::ui_show_error(const std::string &error) {
  throw kax_info_x{(boost::format("(MKVInfo) %1%\n") % error).str()};
}

void
kax_info_c::ui_show_progress(int /* percentage */,
                             std::string const &/* text */) {
}

void
kax_info_c::show_unknown_element(EbmlElement *e,
                                 int level) {
  show_element(e, level, create_unknown_element_text(*e), true);
}

void
kax_info_c::show_element(EbmlElement *l,
                         int level,
                         const std::string &info,
                         bool skip) {
  if (m_show_summary)
    return;

  ui_show_element_info(level, info,
                         !l                 ? -1
                       :                      static_cast<int64_t>(l->GetElementPosition()),
                         !l                 ? -1
                       : !l->IsFiniteSize() ? -2
                       :                      static_cast<int64_t>(l->GetSizeLength() + EBML_ID_LENGTH(static_cast<const EbmlId &>(*l)) + l->GetSize()));

  if (!l || !skip)
    return;

  // Dump unknown elements recursively.
  auto *m = dynamic_cast<EbmlMaster *>(l);
  if (m)
    for (auto child : *m)
      show_unknown_element(child, level + 1);

  l->SkipData(*m_es, EBML_CONTEXT(l));
}

void
kax_info_c::show_element(EbmlElement *l,
                         int level,
                         boost::format const &info,
                         bool skip) {
  show_element(l, level, info.str(), skip);
}

std::string
kax_info_c::format_ebml_id_as_hex(EbmlId const &id) {
  return (boost::format((boost::format("%%|1$0%1%x|") % EBML_ID_LENGTH(id)).str()) % EBML_ID_VALUE(id)).str();
}

std::string
kax_info_c::format_ebml_id_as_hex(EbmlElement &e) {
  return format_ebml_id_as_hex(static_cast<const EbmlId &>(e));
}

std::string
kax_info_c::create_unknown_element_text(EbmlElement &e) {
  return (ms_common_formats[ms_bf_show_unknown_element] % EBML_NAME(&e) % format_ebml_id_as_hex(e) % (e.GetSize() + e.HeadSize())).str();
}

std::string
kax_info_c::create_known_element_but_not_allowed_here_text(EbmlElement &e) {
  return (boost::format(Y("(Known element, but invalid at this position: %1%; ID: 0x%2% size: %3%)")) % kax_element_names_c::get(e) % format_ebml_id_as_hex(e) % (e.GetSize() + e.HeadSize())).str();
}

std::string
kax_info_c::create_element_text(const std::string &text,
                                int64_t position,
                                int64_t size) {
  std::string additional_text;

  if ((1 < m_verbose) && (0 <= position))
    additional_text += ((m_hex_positions ? ms_common_formats[ms_bf_at_hex] : ms_common_formats[ms_bf_at]) % position).str();

  if (m_show_size && (-1 != size)) {
    if (-2 != size)
      additional_text += (ms_common_formats[ms_bf_size] % size).str();
    else
      additional_text += Y(" size is unknown");
  }

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
      text += std::string{": "} + value;
  }

  return create_element_text(text, e.GetElementPosition(), e.HeadSize() + e.GetSize());
}

std::string
kax_info_c::format_element_value(EbmlElement &e) {
  auto formatter = m_custom_element_value_formatters.find(EbmlId(e).GetValue());

  return (formatter == m_custom_element_value_formatters.end()) || dynamic_cast<EbmlDummy *>(&e) ? format_element_value_default(e) : formatter->second(e);
}

std::string
kax_info_c::format_element_value_default(EbmlElement &e) {
  if (Is<EbmlVoid>(e))
    return format_element_size(e);

  if (Is<EbmlCrc32>(e))
    return (ms_common_formats[ms_bf_crc32_value] % static_cast<EbmlCrc32 &>(e).GetCrc32()).str();

  if (dynamic_cast<EbmlUInteger *>(&e))
    return to_string(static_cast<EbmlUInteger &>(e).GetValue());

  if (dynamic_cast<EbmlSInteger *>(&e))
    return to_string(static_cast<EbmlSInteger &>(e).GetValue());

  if (dynamic_cast<EbmlString *>(&e))
    return static_cast<EbmlString &>(e).GetValue();

  if (dynamic_cast<EbmlUnicodeString *>(&e))
    return static_cast<EbmlUnicodeString &>(e).GetValueUTF8();

  if (dynamic_cast<EbmlBinary *>(&e))
    return format_binary(static_cast<EbmlBinary &>(e));

  if (dynamic_cast<EbmlDate *>(&e))
    return mtx::date_time::to_string(boost::posix_time::from_time_t(static_cast<EbmlDate &>(e).GetEpochDate()), "%a %b %d %H:%M:%S %Y UTC");

  if (dynamic_cast<EbmlMaster *>(&e))
    return {};

  if (dynamic_cast<EbmlFloat *>(&e))
    return to_string(static_cast<EbmlFloat &>(e).GetValue());

  throw std::invalid_argument{"format_element_value: unsupported EbmlElement type"};
}

std::string
kax_info_c::create_hexdump(unsigned char const *buf,
                           int size) {
  static boost::format s_bf_create_hexdump(" %|1$02x|");

  std::string hex(" hexdump");
  int bmax = std::min(size, m_hexdump_max_size);
  int b;

  for (b = 0; b < bmax; ++b)
    hex += (s_bf_create_hexdump % static_cast<int>(buf[b])).str();

  return hex;
}

std::string
kax_info_c::create_codec_dependent_private_info(KaxCodecPrivate &c_priv,
                                                char track_type,
                                                std::string const &codec_id) {
  if ((codec_id == MKV_V_MSCOMP) && ('v' == track_type) && (c_priv.GetSize() >= sizeof(alBITMAPINFOHEADER))) {
    auto bih = reinterpret_cast<alBITMAPINFOHEADER *>(c_priv.GetBuffer());
    return (boost::format(Y(" (FourCC: %1%)")) % fourcc_c{&bih->bi_compression}.description()).str();

  } else if ((codec_id == MKV_A_ACM) && ('a' == track_type) && (c_priv.GetSize() >= sizeof(alWAVEFORMATEX))) {
    alWAVEFORMATEX *wfe     = reinterpret_cast<alWAVEFORMATEX *>(c_priv.GetBuffer());
    return (boost::format(Y(" (format tag: 0x%|1$04x|)")) % get_uint16_le(&wfe->w_format_tag)).str();

  } else if ((codec_id == MKV_V_MPEG4_AVC) && ('v' == track_type) && (c_priv.GetSize() >= 4)) {
    auto avcc = mtx::avc::avcc_c::unpack(memory_cptr{new memory_c(c_priv.GetBuffer(), c_priv.GetSize(), false)});

    return (boost::format(Y(" (h.264 profile: %1% @L%2%.%3%)"))
            % (  avcc.m_profile_idc ==  44 ? "CAVLC 4:4:4 Intra"
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
               :                             Y("Unknown"))
            % (avcc.m_level_idc / 10) % (avcc.m_level_idc % 10)).str();
  } else if ((codec_id == MKV_V_MPEGH_HEVC) && ('v' == track_type) && (c_priv.GetSize() >= 4)) {
    auto hevcc = mtx::hevc::hevcc_c::unpack(std::make_shared<memory_c>(c_priv.GetBuffer(), c_priv.GetSize(), false));

    return (boost::format(Y(" (HEVC profile: %1% @L%2%.%3%)"))
            % (  hevcc.m_general_profile_idc == 1 ? "Main"
               : hevcc.m_general_profile_idc == 2 ? "Main 10"
               : hevcc.m_general_profile_idc == 3 ? "Main Still Picture"
               :                                    Y("Unknown"))
            % (hevcc.m_general_level_idc / 3 / 10) % (hevcc.m_general_level_idc / 3 % 10)).str();
  }

  return "";
}

void
kax_info_c::add_track(kax_info_c::track_cptr const &t) {
  m_tracks.push_back(t);
  m_tracks_by_number[t->tnum] = t;
}

kax_info_c::track_t *
kax_info_c::find_track(int tnum) {
  return m_tracks_by_number[tnum].get();
}

void
kax_info_c::read_master(EbmlMaster *m,
                        EbmlSemanticContext const &ctx,
                        int &upper_lvl_el,
                        EbmlElement *&l2) {
  m->Read(*m_es, ctx, upper_lvl_el, l2, true);
  if (m->ListSize() == 0)
    return;

  brng::sort(m->GetElementList(), [](EbmlElement const *a, EbmlElement const *b) { return a->GetElementPosition() < b->GetElementPosition(); });
}

std::string
kax_info_c::format_binary(EbmlBinary &bin,
                          std::size_t max_len) {
  auto len     = std::min(max_len, static_cast<std::size_t>(bin.GetSize()));
  auto const b = bin.GetBuffer();
  auto result  = (ms_common_formats[ms_bf_format_binary_1] % bin.GetSize() % to_hex(b, len)).str();

  if (len < bin.GetSize())
    result += "...";

  if (m_calc_checksums)
    result += (ms_common_formats[ms_bf_format_binary_2] % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, bin.GetBuffer(), bin.GetSize())).str();

  strip(result);

  return result;
}

std::string
kax_info_c::format_binary_as_hex(EbmlElement &e) {
  return to_hex(static_cast<EbmlBinary &>(e));
}

std::string
kax_info_c::format_element_size(EbmlElement &e) {
  if (e.IsFiniteSize())
    return (ms_common_formats[ms_bf_element_size] % e.GetSize()).str();
  return Y("size unknown");
}

std::string
kax_info_c::format_unsigned_integer_as_timestamp(EbmlElement &e) {
  return format_timestamp(static_cast<EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_unsigned_integer_as_scaled_timestamp(EbmlElement &e) {
  return format_timestamp(m_ts_scale * static_cast<EbmlUInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_timestamp(EbmlElement &e) {
  return format_timestamp(static_cast<EbmlSInteger &>(e).GetValue());
}

std::string
kax_info_c::format_signed_integer_as_scaled_timestamp(EbmlElement &e) {
  return format_timestamp(m_ts_scale * static_cast<EbmlSInteger &>(e).GetValue());
}

#define PRE(  Class, Processor) m_custom_element_pre_processors.insert(  { Class::ClassInfos.GlobalId.GetValue(), Processor });
#define POST( Class, Processor) m_custom_element_post_processors.insert( { Class::ClassInfos.GlobalId.GetValue(), Processor });
#define FMT(  Class, Formatter) m_custom_element_value_formatters.insert({ Class::ClassInfos.GlobalId.GetValue(), Formatter });
#define PREM( Class, Processor) PRE(Class, std::bind(&kax_info_c::Processor, this, std::placeholders::_1));
#define POSTM(Class, Processor) POST(Class, std::bind(&kax_info_c::Processor, this, std::placeholders::_1));
#define FMTM( Class, Formatter) FMT(Class, std::bind(&kax_info_c::Formatter, this, std::placeholders::_1));

void
kax_info_c::init_custom_element_value_formatters_and_processors() {
  m_custom_element_value_formatters.clear();
  m_custom_element_pre_processors.clear();
  m_custom_element_post_processors.clear();

  // Simple processors:
  PRE(KaxInfo,         [this](EbmlElement &e) -> bool { m_ts_scale = FindChildValue<KaxTimecodeScale>(static_cast<KaxInfo &>(e), TIMESTAMP_SCALE); return true; });
  PRE(KaxTracks,       [this](EbmlElement &)  -> bool { m_mkvmerge_track_id = 0; return true; });
  PREM(KaxSimpleBlock, pre_simple_block);
  PREM(KaxBlockGroup,  pre_block_group);
  PREM(KaxBlock,       pre_block);

  // More complex processors:
  PRE(KaxSeekHead, [this](EbmlElement &e) -> bool {
    if ((m_verbose < 2) && !m_use_gui)
      show_element(&e, 1, Y("Seek head (subentries will be skipped)"));
    return (m_verbose >= 2) || m_use_gui;
  });

  PRE(KaxCues, [this](EbmlElement &e) -> bool {
    if (m_verbose < 2)
      show_element(&e, 1, Y("Cues (subentries will be skipped)"));
    return m_verbose >= 2;
  });

  PRE(KaxTrackEntry, [this](EbmlElement &e) -> bool {
    m_summary.clear();

    auto &master              = static_cast<EbmlMaster &>(e);
    m_track                   = std::make_shared<track_t>();
    m_track->tuid             = FindChildValue<KaxTrackUID>(master);
    m_track->codec_id         = FindChildValue<KaxCodecID>(master);
    m_track->default_duration = FindChildValue<KaxTrackDefaultDuration>(master);

    return true;
  });

  PRE(KaxTrackNumber, [this](EbmlElement &e) -> bool {
    m_track->tnum = static_cast<KaxTrackNumber &>(e).GetValue();

    auto existing_track = find_track(m_track->tnum);
    auto track_id       = m_mkvmerge_track_id;
    if (!existing_track) {
      m_track->mkvmerge_track_id = m_mkvmerge_track_id;
      ++m_mkvmerge_track_id;
      add_track(m_track);

    } else
      track_id = existing_track->mkvmerge_track_id;

    m_summary.push_back((boost::format(Y("mkvmerge/mkvextract track ID: %1%")) % track_id).str());

    return true;
  });

  PRE(KaxTrackType, [this](EbmlElement &e) -> bool {
    auto ttype    = static_cast<KaxTrackType &>(e).GetValue();
    m_track->type = track_audio    == ttype ? 'a'
                  : track_video    == ttype ? 'v'
                  : track_subtitle == ttype ? 's'
                  : track_buttons  == ttype ? 'b'
                  :                           '?';
    return true;
  });

  PRE(KaxCodecPrivate, [this](EbmlElement &e) -> bool {
    auto &c_priv     = static_cast<KaxCodecPrivate &>(e);
    m_track-> fourcc = create_codec_dependent_private_info(c_priv, m_track->type, m_track->codec_id);

    if (m_calc_checksums && !m_show_summary)
      m_track->fourcc += (boost::format(Y(" (adler: 0x%|1$08x|)")) % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, c_priv.GetBuffer(), c_priv.GetSize())).str();

    if (m_show_hexdump)
      m_track->fourcc += create_hexdump(c_priv.GetBuffer(), c_priv.GetSize());

    return true;
  });

  PRE(KaxCluster, [this](EbmlElement &e) -> bool {
    m_cluster = static_cast<KaxCluster *>(&e);

    if (m_use_gui)
      ui_show_progress(100 * m_cluster->GetElementPosition() / m_file_size, Y("Parsing file"));

    m_cluster->InitTimecode(FindChildValue<KaxClusterTimecode>(m_cluster), m_ts_scale);

    return true;
  });

  POST(KaxAudioSamplingFreq,       [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("sampling freq: %1%")) % static_cast<KaxAudioSamplingFreq &>(e).GetValue()).str()); });
  POST(KaxAudioOutputSamplingFreq, [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("output sampling freq: %1%")) % static_cast<KaxAudioOutputSamplingFreq &>(e).GetValue()).str()); });
  POST(KaxAudioChannels,           [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("channels: %1%")) % static_cast<KaxAudioChannels &>(e).GetValue()).str()); });
  POST(KaxAudioBitDepth,           [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("bits per sample: %1%")) % static_cast<KaxAudioBitDepth &>(e).GetValue()).str()); });

  POST(KaxVideoPixelWidth,         [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel width: %1%")) % static_cast<KaxVideoPixelWidth &>(e).GetValue()).str()); });
  POST(KaxVideoPixelHeight,        [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel height: %1%")) % static_cast<KaxVideoPixelHeight &>(e).GetValue()).str()); });
  POST(KaxVideoDisplayWidth,       [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("display width: %1%")) % static_cast<KaxVideoDisplayWidth &>(e).GetValue()).str()); });
  POST(KaxVideoDisplayHeight,      [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("display height: %1%")) % static_cast<KaxVideoDisplayHeight &>(e).GetValue()).str()); });
  POST(KaxVideoPixelCropLeft,      [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel crop left: %1%")) % static_cast<KaxVideoPixelCropLeft &>(e).GetValue()).str()); });
  POST(KaxVideoPixelCropTop,       [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel crop top: %1%")) % static_cast<KaxVideoPixelCropTop &>(e).GetValue()).str()); });
  POST(KaxVideoPixelCropRight,     [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel crop right: %1%")) % static_cast<KaxVideoPixelCropRight &>(e).GetValue()).str()); });
  POST(KaxVideoPixelCropBottom,    [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("pixel crop bottom: %1%")) % static_cast<KaxVideoPixelCropBottom &>(e).GetValue()).str()); });
  POST(KaxTrackLanguage,           [this](EbmlElement &e) { m_summary.push_back((boost::format(Y("language: %1%")) % static_cast<KaxTrackLanguage &>(e).GetValue()).str()); });
  POST(KaxBlockDuration,           [this](EbmlElement &e) { m_block_duration = static_cast<double>(static_cast<KaxBlockDuration &>(e).GetValue()) * m_ts_scale; });
  POST(KaxReferenceBlock,          [this](EbmlElement &)  { ++m_num_references; });

  POSTM(KaxSimpleBlock,            post_simple_block);
  POSTM(KaxBlock,                  post_block);
  POSTM(KaxBlockGroup,             post_block_group);

  POST(KaxTrackDefaultDuration,    [this](EbmlElement &) {
    m_summary.push_back((boost::format(Y("default duration: %|1$.3f|ms (%|2$.3f| frames/fields per second for a video track)"))
                         % (static_cast<double>(m_track->default_duration) / 1000000.0)
                         % (1000000000.0 / static_cast<double>(m_track->default_duration))
                         ).str());
  });

  POST(KaxTrackEntry, [this](EbmlElement &) {
    if (!m_show_summary)
      return;

    m_out->write((boost::format(Y("Track %1%: %2%, codec ID: %3%%4%%5%%6%\n"))
                  % m_track->tnum
                  % (  'a' == m_track->type ? Y("audio")
                     : 'v' == m_track->type ? Y("video")
                     : 's' == m_track->type ? Y("subtitles")
                     : 'b' == m_track->type ? Y("buttons")
                     :                        Y("unknown"))
                  % m_track->codec_id
                  % m_track->fourcc
                  % (m_summary.empty() ? "" : ", ")
                  % boost::join(m_summary, ", ")).str());
  });

  // Simple formatters:
  FMTM(KaxSegmentUID,       format_binary_as_hex);
  FMTM(KaxSegmentFamily,    format_binary_as_hex);
  FMTM(KaxNextUID,          format_binary_as_hex);
  FMTM(KaxSegmentFamily,    format_binary_as_hex);
  FMTM(KaxPrevUID,          format_binary_as_hex);
  FMTM(KaxSegment,          format_element_size);
  FMTM(KaxFileData,         format_element_size);
  FMTM(KaxChapterTimeStart, format_unsigned_integer_as_timestamp);
  FMTM(KaxChapterTimeEnd,   format_unsigned_integer_as_timestamp);
  FMTM(KaxCueDuration,      format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxCueTime,          format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxCueRefTime,       format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxCodecDelay,       format_unsigned_integer_as_timestamp);
  FMTM(KaxSeekPreRoll,      format_unsigned_integer_as_timestamp);
  FMTM(KaxClusterTimecode,  format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxSliceDelay,       format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxSliceDuration,    format_signed_integer_as_timestamp);
  FMTM(KaxSimpleBlock,      format_simple_block);
  FMTM(KaxBlock,            format_block);
  FMTM(KaxBlockDuration,    format_unsigned_integer_as_scaled_timestamp);
  FMTM(KaxReferenceBlock,   format_signed_integer_as_scaled_timestamp);

  FMT(KaxDuration,          [this](EbmlElement &e) { return format_timestamp(static_cast<int64_t>(static_cast<EbmlFloat &>(e).GetValue() * m_ts_scale)); });

  // More complex formatters:
  FMT(KaxSeekID, [](EbmlElement &e) -> std::string {
    auto &seek_id = static_cast<KaxSeekID &>(e);
    EbmlId id(seek_id.GetBuffer(), seek_id.GetSize());

    return (boost::format("%1% (%2%)")
            % to_hex(seek_id)
            % (  Is<KaxInfo>(id)        ? "KaxInfo"
               : Is<KaxCluster>(id)     ? "KaxCluster"
               : Is<KaxTracks>(id)      ? "KaxTracks"
               : Is<KaxCues>(id)        ? "KaxCues"
               : Is<KaxAttachments>(id) ? "KaxAttachments"
               : Is<KaxChapters>(id)    ? "KaxChapters"
               : Is<KaxTags>(id)        ? "KaxTags"
               : Is<KaxSeekHead>(id)    ? "KaxSeekHead"
               :                          "unknown")).str();
  });

  FMT(KaxVideoProjectionType, [](EbmlElement &e) -> std::string {
    auto value       = static_cast<KaxVideoProjectionType &>(e).GetValue();
    auto description = 0 == value ? Y("rectangular")
                     : 1 == value ? Y("equirectangular")
                     : 2 == value ? Y("cubemap")
                     : 3 == value ? Y("mesh")
                     :              Y("unknown");

    return (boost::format("%1% (%2%)") % value % description).str();
  });

  FMT(KaxVideoDisplayUnit, [](EbmlElement &e) -> std::string {
      auto unit = static_cast<KaxVideoDisplayUnit &>(e).GetValue();
      return (boost::format("%1%%2%")
              % unit
              % (  0 == unit ? Y(" (pixels)")
                   : 1 == unit ? Y(" (centimeters)")
                   : 2 == unit ? Y(" (inches)")
                   : 3 == unit ? Y(" (aspect ratio)")
                   :               "")).str();
  });

  FMT(KaxVideoFieldOrder, [](EbmlElement &e) -> std::string {
    auto field_order = static_cast<KaxVideoFieldOrder &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % field_order
            % (  0  == field_order ? Y("progressive")
               : 1  == field_order ? Y("top field displayed first, top field stored first")
               : 2  == field_order ? Y("unspecified")
               : 6  == field_order ? Y("bottom field displayed first, bottom field stored first")
               : 9  == field_order ? Y("bottom field displayed first, top field stored first")
               : 14 == field_order ? Y("top field displayed first, bottom field stored first")
               :                     Y("unknown"))).str();
  });

  FMT(KaxVideoStereoMode, [](EbmlElement &e) -> std::string {
    auto stereo_mode = static_cast<KaxVideoStereoMode &>(e).GetValue();
    return (boost::format("%1% (%2%)") % stereo_mode % stereo_mode_c::translate(static_cast<stereo_mode_c::mode>(stereo_mode))).str();
  });

  FMT(KaxVideoAspectRatio, [](EbmlElement &e) -> std::string {
    auto ar_type = static_cast<KaxVideoAspectRatio &>(e).GetValue();
    return (boost::format("%1%%2%")
            % ar_type
            % (  0 == ar_type ? Y(" (free resizing)")
               : 1 == ar_type ? Y(" (keep aspect ratio)")
               : 2 == ar_type ? Y(" (fixed)")
               :                  "")).str();
  });

  FMT(KaxContentEncodingScope, [](EbmlElement &e) -> std::string {
    std::vector<std::string> scope;
    auto ce_scope = static_cast<KaxContentEncodingScope &>(e).GetValue();

    if ((ce_scope & 0x01) == 0x01)
      scope.push_back(Y("1: all frames"));
    if ((ce_scope & 0x02) == 0x02)
      scope.push_back(Y("2: codec private data"));
    if ((ce_scope & 0xfc) != 0x00)
      scope.push_back(Y("rest: unknown"));
    if (scope.empty())
      scope.push_back(Y("unknown"));

    return (boost::format("%1% (%2%)") % ce_scope % boost::join(scope, ", ")).str();
  });

  FMT(KaxContentEncodingType, [](EbmlElement &e) -> std::string {
    auto ce_type = static_cast<KaxContentEncodingType &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % ce_type
            % (  0 == ce_type ? Y("compression")
               : 1 == ce_type ? Y("encryption")
               :                Y("unknown"))).str();
  });

  FMT(KaxContentCompAlgo, [](EbmlElement &e) -> std::string {
    auto c_algo = static_cast<KaxContentCompAlgo &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % c_algo
            % (  0 == c_algo ?   "ZLIB"
               : 1 == c_algo ?   "bzLib"
               : 2 == c_algo ?   "lzo1x"
               : 3 == c_algo ? Y("header removal")
               :               Y("unknown"))).str();
  });

  FMT(KaxContentEncAlgo, [](EbmlElement &e) -> std::string {
    auto e_algo = static_cast<KaxContentEncAlgo &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % e_algo
            % (  0 == e_algo ? Y("no encryption")
               : 1 == e_algo ?   "DES"
               : 2 == e_algo ?   "3DES"
               : 3 == e_algo ?   "Twofish"
               : 4 == e_algo ?   "Blowfish"
               : 5 == e_algo ?   "AES"
               :               Y("unknown"))).str();
  });

  FMT(KaxContentSigAlgo, [](EbmlElement &e) -> std::string {
    auto s_algo = static_cast<KaxContentSigAlgo &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % s_algo
            % (  0 == s_algo ? Y("no signature algorithm")
               : 1 == s_algo ? Y("RSA")
               :               Y("unknown"))).str();
  });

  FMT(KaxContentSigHashAlgo, [](EbmlElement &e) -> std::string {
    auto s_halgo = static_cast<KaxContentSigHashAlgo &>(e).GetValue();
    return (boost::format("%1% (%2%)")
            % s_halgo
            % (  0 == s_halgo ? Y("no signature hash algorithm")
               : 1 == s_halgo ? Y("SHA1-160")
               : 2 == s_halgo ? Y("MD5")
               :                Y("unknown"))).str();
  });

  FMT(KaxTrackNumber, [this](EbmlElement &) -> std::string { return (boost::format(Y("%1% (track ID for mkvmerge & mkvextract: %2%)")) % m_track->tnum % m_track->mkvmerge_track_id).str(); });

  FMT(KaxTrackType, [this](EbmlElement &) -> std::string {
    return 'a' == m_track->type ? "audio"
         : 'v' == m_track->type ? "video"
         : 's' == m_track->type ? "subtitles"
         : 'b' == m_track->type ? "buttons"
         :                        "unknown";
  });

  FMT(KaxCodecPrivate, [this](EbmlElement &e) -> std::string {
    return (ms_common_formats[ms_bf_element_size] % e.GetSize()).str() + m_track->fourcc;
  });

  FMT(KaxTrackDefaultDuration, [this](EbmlElement &) -> std::string {
      return (boost::format(Y("%1% (%|2$.3f| frames/fields per second for a video track)"))
              % format_timestamp(m_track->default_duration)
              % (1000000000.0 / static_cast<double>(m_track->default_duration))).str();
  });
}

#undef FMT
#undef PRE
#undef POST
#undef FMTM
#undef PREM
#undef POSTM

bool
kax_info_c::pre_block(EbmlElement &e) {
  auto &block = static_cast<KaxBlock &>(e);

  block.SetParent(*m_cluster);

  m_lf_timestamp   = block.GlobalTimecode();
  m_lf_tnum        = block.TrackNum();
  m_block_duration = boost::none;

  return true;
}

std::string
kax_info_c::format_block(EbmlElement &e) {
  auto &block = static_cast<KaxBlock &>(e);

  return (ms_common_formats[ms_bf_block_group_block_summary]
          % block.TrackNum()
          % block.NumberFrames()
          % format_timestamp(m_lf_timestamp)).str();
}

void
kax_info_c::post_block(EbmlElement &e) {
  auto &block = static_cast<KaxBlock &>(e);

  for (int i = 0, num_frames = block.NumberFrames(); i < num_frames; ++i) {
    auto &data = block.GetBuffer(i);
    auto adler = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (m_calc_checksums)
      adler_str = (ms_common_formats[ms_bf_block_group_block_adler] % adler).str();

    std::string hex;
    if (m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    show_element(nullptr, 4, ms_common_formats[ms_bf_block_group_block_frame] % data.Size() % adler_str % hex);

    m_frame_sizes.push_back(data.Size());
    m_frame_adlers.push_back(adler);
    m_frame_hexdumps.push_back(hex);
  }
}

bool
kax_info_c::pre_block_group(EbmlElement &) {
  m_num_references = 0;
  m_lf_timestamp   = 0;
  m_lf_tnum        = 0;

  m_frame_sizes.clear();
  m_frame_adlers.clear();
  m_frame_hexdumps.clear();

  return true;
}

void
kax_info_c::post_block_group(EbmlElement &e) {
  if (m_show_summary) {
    std::string position;
    std::size_t fidx;
    auto frame_pos = e.GetElementPosition() + e.HeadSize();

    for (fidx = 0; fidx < m_frame_sizes.size(); fidx++) {
      if (1 <= m_verbose) {
        position   = (ms_common_formats[ms_bf_block_group_summary_position] % frame_pos).str();
        frame_pos += m_frame_sizes[fidx];
      }

      if (m_block_duration)
        m_out->write((ms_common_formats[ms_bf_block_group_summary_with_duration]
                      % (m_num_references >= 2 ? 'B' : m_num_references == 1 ? 'P' : 'I')
                      % m_lf_tnum
                      % format_timestamp(m_lf_timestamp)
                      % format_timestamp(*m_block_duration)
                      % m_frame_sizes[fidx]
                      % m_frame_adlers[fidx]
                      % m_frame_hexdumps[fidx]
                      % position).str());
      else
        m_out->write((ms_common_formats[ms_bf_block_group_summary_no_duration]
                      % (m_num_references >= 2 ? 'B' : m_num_references == 1 ? 'P' : 'I')
                      % m_lf_tnum
                      % format_timestamp(m_lf_timestamp)
                      % m_frame_sizes[fidx]
                      % m_frame_adlers[fidx]
                      % m_frame_hexdumps[fidx]
                      % position).str());
    }

  } else if (m_verbose > 2)
    show_element(nullptr, 2,
                 ms_common_formats[ms_bf_block_group_summary_v2]
                 % (m_num_references >= 2 ? 'B' : m_num_references == 1 ? 'P' : 'I')
                 % m_lf_tnum
                 % format_timestamp(m_lf_timestamp));

  auto &tinfo = m_track_info[m_lf_tnum];

  tinfo.m_blocks                                                    += m_frame_sizes.size();
  tinfo.m_blocks_by_ref_num[std::min<int64_t>(m_num_references, 2)] += m_frame_sizes.size();
  tinfo.m_min_timestamp                                              = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : m_lf_timestamp, m_lf_timestamp);
  tinfo.m_size                                                      += boost::accumulate(m_frame_sizes, 0);

  if (tinfo.m_max_timestamp && (*tinfo.m_max_timestamp >= m_lf_timestamp))
    return;

  tinfo.m_max_timestamp = m_lf_timestamp;

  if (!m_block_duration)
    tinfo.m_add_duration_for_n_packets  = m_frame_sizes.size();
  else {
    *tinfo.m_max_timestamp             += *m_block_duration;
    tinfo.m_add_duration_for_n_packets  = 0;
  }
}

bool
kax_info_c::pre_simple_block(EbmlElement &e) {
  m_frame_sizes.clear();
  m_frame_adlers.clear();
  m_frame_hexdumps.clear();

  static_cast<KaxSimpleBlock &>(e).SetParent(*m_cluster);

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

  return (ms_common_formats[ms_bf_simple_block_basics]
          % info
          % block.TrackNum()
          % block.NumberFrames()
          % format_timestamp(timestamp_ns)).str();
}

void
kax_info_c::post_simple_block(EbmlElement &e) {
  auto &block       = static_cast<KaxSimpleBlock &>(e);
  auto &tinfo       = m_track_info[block.TrackNum()];
  auto timestamp_ns = mtx::math::to_signed(block.GlobalTimecode());
  int num_frames    = block.NumberFrames();
  int64_t frame_pos = block.GetElementPosition() + block.ElementSize();

  for (int idx = 0; idx < num_frames; ++idx) {
    auto &data = block.GetBuffer(idx);
    auto adler = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (m_calc_checksums)
      adler_str = (ms_common_formats[ms_bf_simple_block_adler] % adler).str();

    std::string hex;
    if (m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    show_element(nullptr, 3, ms_common_formats[ms_bf_simple_block_frame] % data.Size() % adler_str % hex);

    m_frame_sizes.push_back(data.Size());
    m_frame_adlers.push_back(adler);
    frame_pos -= data.Size();
  }

  if (m_show_summary) {
    std::string position;

    for (int idx = 0; idx < num_frames; idx++) {
      if (1 <= m_verbose) {
        position   = (ms_common_formats[ms_bf_simple_block_position] % frame_pos).str();
        frame_pos += m_frame_sizes[idx];
      }

      m_out->write((ms_common_formats[ms_bf_simple_block_summary]
                    % (block.IsKeyframe() ? 'I' : block.IsDiscardable() ? 'B' : 'P')
                    % block.TrackNum()
                    % format_timestamp(timestamp_ns)
                    % m_frame_sizes[idx]
                    % m_frame_adlers[idx]
                    % position).str());
    }

  } else if (m_verbose > 2)
    show_element(nullptr, 2,
                 ms_common_formats[ms_bf_simple_block_summary_v2]
                 % (block.IsKeyframe() ? 'I' : block.IsDiscardable() ? 'B' : 'P')
                 % block.TrackNum()
                 % timestamp_ns);

  tinfo.m_blocks                                                                    += block.NumberFrames();
  tinfo.m_blocks_by_ref_num[block.IsKeyframe() ? 0 : block.IsDiscardable() ? 2 : 1] += block.NumberFrames();
  tinfo.m_min_timestamp                                                              = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_max_timestamp                                                              = std::max(tinfo.m_max_timestamp ? *tinfo.m_max_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_add_duration_for_n_packets                                                 = block.NumberFrames();
  tinfo.m_size                                                                      += boost::accumulate(m_frame_sizes, 0);
}

kax_info_c::result_e
kax_info_c::handle_segment(EbmlElement *l0) {
  ui_show_element(*l0);

  m_file_size   = m_in->get_size();
  auto l1       = static_cast<EbmlElement *>(nullptr);
  auto kax_file = std::make_shared<kax_file_c>(*m_in);
  m_level       = 1;

  kax_file->set_segment_end(*l0);


  // Prevent reporting "first timestamp after resync":
  kax_file->set_timestamp_scale(-1);

  while ((l1 = kax_file->read_next_level1_element())) {
    std::shared_ptr<EbmlElement> af_l1(l1);

    if (Is<KaxCluster>(l1) && (m_verbose == 0) && !m_show_summary) {
      ui_show_element(*l1);
      return result_e::succeeded;

    } else if (Is<EbmlVoid>(l1) || Is<EbmlCrc32>(l1) || Is<KaxInfo>(l1) || Is<KaxSeekHead>(l1) || Is<KaxAttachments>(l1) || Is<KaxChapters>(l1) || Is<KaxTags>(l1) || Is<KaxCues>(l1) || Is<KaxTracks>(l1) || Is<KaxCluster>(l1))
      handle_elements_generic(*l1);

    else
      show_unknown_element(l1, 1);

    if (!m_in->setFilePointer2(l1->GetElementPosition() + kax_file->get_element_size(l1)))
      break;
    if (!in_parent(l0))
      break;
    if (m_abort)
      return result_e::aborted;

  } // while (l1)

  return result_e::succeeded;
}

void
kax_info_c::handle_elements_generic(EbmlElement &e) {
  auto is_dummy = !!dynamic_cast<EbmlDummy *>(&e);
  if (!is_dummy) {
    auto pre_processor = m_custom_element_pre_processors.find(EbmlId(e).GetValue());
    if (pre_processor != m_custom_element_pre_processors.end())
      if (!pre_processor->second(e))
        return;
  }

  ui_show_element(e);

  if (dynamic_cast<EbmlMaster *>(&e)) {
    ++m_level;

    for (auto child : static_cast<EbmlMaster &>(e))
      handle_elements_generic(*child);

    --m_level;
  }

  if (is_dummy)
    return;

  auto post_processor = m_custom_element_post_processors.find(EbmlId(e).GetValue());
  if (post_processor != m_custom_element_post_processors.end())
    post_processor->second(e);
}

void
kax_info_c::display_track_info() {
  if (!m_show_track_info)
    return;

  for (auto &track : m_tracks) {
    track_info_t &tinfo  = m_track_info[track->tnum];

    if (!tinfo.m_min_timestamp)
      tinfo.m_min_timestamp = 0;
    if (!tinfo.m_max_timestamp)
      tinfo.m_max_timestamp = tinfo.m_min_timestamp;

    int64_t duration  = *tinfo.m_max_timestamp - *tinfo.m_min_timestamp;
    duration         += tinfo.m_add_duration_for_n_packets * track->default_duration;

    m_out->write((boost::format(Y("Statistics for track number %1%: number of blocks: %2%; size in bytes: %3%; duration in seconds: %4%; approximate bitrate in bits/second: %5%\n"))
                  % track->tnum
                  % tinfo.m_blocks
                  % tinfo.m_size
                  % (duration / 1000000000.0)
                  % static_cast<uint64_t>(duration == 0 ? 0 : tinfo.m_size * 8000000000.0 / duration)).str());
  }
}

void
kax_info_c::reset() {
  m_ts_scale = TIMESTAMP_SCALE;
  m_tracks.clear();
  m_tracks_by_number.clear();
  m_track_info.clear();
  m_es.reset();
  m_in.reset();
}

kax_info_c::result_e
kax_info_c::process_file(std::string const &file_name) {
  at_scope_exit_c cleanup([this]() { reset(); });

  reset();

  // open input file
  try {
    m_in = mm_file_io_c::open(file_name);
  } catch (mtx::mm_io::exception &ex) {
    ui_show_error((boost::format(Y("Error: Couldn't open source file %1% (%2%).")) % file_name % ex).str());
    return result_e::failed;
  }

  // open output file
  if (!m_destination_file_name.empty()) {
    try {
      m_out = std::make_shared<mm_file_io_c>(m_destination_file_name, MODE_CREATE);

    } catch (mtx::mm_io::exception &ex) {
      ui_show_error((boost::format(Y("The file '%1%' could not be opened for writing: %2%.")) % m_destination_file_name % ex).str());
      return result_e::failed;
    }
  }

  try {
    m_es = std::make_shared<EbmlStream>(*m_in);

    // Find the EbmlHead element. Must be the first one.
    auto l0 = ebml_element_cptr{ m_es->FindNextID(EBML_INFO(EbmlHead), 0xFFFFFFFFL) };
    if (!l0 || !Is<EbmlHead>(*l0)) {
      ui_show_error(Y("No EBML head found."));
      return result_e::failed;
    }

    int upper_lvl_el           = 0;
    EbmlElement *element_found = nullptr;

    read_master(static_cast<EbmlMaster *>(l0.get()), EBML_CONTEXT(l0), upper_lvl_el, element_found);
    delete element_found;

    handle_elements_generic(*l0);
    l0->SkipData(*m_es, EBML_CONTEXT(l0));

    while (1) {
      // NEXT element must be a segment
      l0 = ebml_element_cptr{ m_es->FindNextID(EBML_INFO(KaxSegment), 0xFFFFFFFFFFFFFFFFLL) };
      if (!l0)
        break;

      if (!Is<KaxSegment>(*l0)) {
        show_element(l0.get(), 0, Y("Unknown element"));
        l0->SkipData(*m_es, EBML_CONTEXT(l0));

        continue;
      }

      auto result = handle_segment(l0.get());
      if (result == result_e::aborted)
        return result;

      l0->SkipData(*m_es, EBML_CONTEXT(l0));

      if ((m_verbose == 0) && !m_show_summary)
        break;
    }

    if (!m_use_gui && m_show_track_info)
      display_track_info();

    return result_e::succeeded;

  } catch (mtx::kax_info_x &) {
    throw;

  } catch (std::exception &ex) {
    ui_show_error((boost::format("%1%: %2%") % Y("Caught exception") % ex.what()).str());
    return result_e::failed;

  } catch (...) {
    ui_show_error(Y("Caught exception"));
    return result_e::failed;
  }

  return result_e::succeeded;
}

void
kax_info_c::abort() {
  m_abort = true;
}

}
