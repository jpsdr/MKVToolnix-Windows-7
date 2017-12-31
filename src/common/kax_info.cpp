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
unsigned int kax_info_c::ms_bf_ebmlvoid                          = 0;
unsigned int kax_info_c::ms_bf_format_binary_1                   = 0;
unsigned int kax_info_c::ms_bf_format_binary_2                   = 0;
unsigned int kax_info_c::ms_bf_block_group_block_summary         = 0;
unsigned int kax_info_c::ms_bf_block_group_block_frame           = 0;
unsigned int kax_info_c::ms_bf_block_group_duration              = 0;
unsigned int kax_info_c::ms_bf_block_group_reference_1           = 0;
unsigned int kax_info_c::ms_bf_block_group_reference_2           = 0;
unsigned int kax_info_c::ms_bf_block_group_reference_priority    = 0;
unsigned int kax_info_c::ms_bf_block_group_virtual               = 0;
unsigned int kax_info_c::ms_bf_block_group_reference_virtual     = 0;
unsigned int kax_info_c::ms_bf_block_group_add_id                = 0;
unsigned int kax_info_c::ms_bf_block_group_additional            = 0;
unsigned int kax_info_c::ms_bf_block_group_slice_lace            = 0;
unsigned int kax_info_c::ms_bf_block_group_slice_frame           = 0;
unsigned int kax_info_c::ms_bf_block_group_slice_delay           = 0;
unsigned int kax_info_c::ms_bf_block_group_slice_duration        = 0;
unsigned int kax_info_c::ms_bf_block_group_slice_add_id          = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_position      = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_with_duration = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_no_duration   = 0;
unsigned int kax_info_c::ms_bf_block_group_summary_v2            = 0;
unsigned int kax_info_c::ms_bf_simple_block_basics               = 0;
unsigned int kax_info_c::ms_bf_simple_block_frame                = 0;
unsigned int kax_info_c::ms_bf_simple_block_summary              = 0;
unsigned int kax_info_c::ms_bf_simple_block_summary_v2           = 0;
unsigned int kax_info_c::ms_bf_cluster_timestamp                 = 0;
unsigned int kax_info_c::ms_bf_cluster_position                  = 0;
unsigned int kax_info_c::ms_bf_cluster_previous_size             = 0;
unsigned int kax_info_c::ms_bf_codec_state                       = 0;
unsigned int kax_info_c::ms_bf_at                                = 0;
unsigned int kax_info_c::ms_bf_size                              = 0;
unsigned int kax_info_c::ms_bf_block_group_discard_padding       = 0;
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
  BF_ADD(ms_bf_ebmlvoid,                          Y("EbmlVoid (size: %1%)"));
  BF_ADD(ms_bf_format_binary_1,                   Y("length %1%, data: %2%"));
  BF_ADD(ms_bf_format_binary_2,                   Y(" (adler: 0x%|1$08x|)"));
  BF_ADD(ms_bf_block_group_block_summary,         Y("Block (track number %1%, %2% frame(s), timestamp %|3$.3f|s = %4%)"));
  BF_ADD(ms_bf_block_group_block_frame,           Y("Frame with size %1%%2%%3%"));
  BF_ADD(ms_bf_block_group_duration,              Y("Block duration: %1%.%|2$06d|ms"));
  BF_ADD(ms_bf_block_group_reference_1,           Y("Reference block: -%1%.%|2$06d|ms"));
  BF_ADD(ms_bf_block_group_reference_2,           Y("Reference block: %1%.%|2$06d|ms"));
  BF_ADD(ms_bf_block_group_reference_priority,    Y("Reference priority: %1%"));
  BF_ADD(ms_bf_block_group_virtual,               Y("Block virtual: %1%"));
  BF_ADD(ms_bf_block_group_reference_virtual,     Y("Reference virtual: %1%"));
  BF_ADD(ms_bf_block_group_add_id,                Y("AdditionalID: %1%"));
  BF_ADD(ms_bf_block_group_additional,            Y("Block additional: %1%"));
  BF_ADD(ms_bf_block_group_slice_lace,            Y("Lace number: %1%"));
  BF_ADD(ms_bf_block_group_slice_frame,           Y("Frame number: %1%"));
  BF_ADD(ms_bf_block_group_slice_delay,           Y("Delay: %|1$.3f|ms"));
  BF_ADD(ms_bf_block_group_slice_duration,        Y("Duration: %|1$.3f|ms"));
  BF_ADD(ms_bf_block_group_slice_add_id,          Y("Block additional ID: %1%"));
  BF_ADD(ms_bf_block_group_summary_position,      Y(", position %1%"));
  BF_ADD(ms_bf_block_group_summary_with_duration, Y("%1% frame, track %2%, timestamp %3% (%4%), duration %|5$.3f|, size %6%, adler 0x%|7$08x|%8%%9%\n"));
  BF_ADD(ms_bf_block_group_summary_no_duration,   Y("%1% frame, track %2%, timestamp %3% (%4%), size %5%, adler 0x%|6$08x|%7%%8%\n"));
  BF_ADD(ms_bf_block_group_summary_v2,            Y("[%1% frame for track %2%, timestamp %3%]"));
  BF_ADD(ms_bf_simple_block_basics,               Y("SimpleBlock (%1%track number %2%, %3% frame(s), timestamp %|4$.3f|s = %5%)"));
  BF_ADD(ms_bf_simple_block_frame,                Y("Frame with size %1%%2%%3%"));
  BF_ADD(ms_bf_simple_block_summary,              Y("%1% frame, track %2%, timestamp %3% (%4%), size %5%, adler 0x%|6$08x|%7%\n"));
  BF_ADD(ms_bf_simple_block_summary_v2,           Y("[%1% frame for track %2%, timestamp %3%]"));
  BF_ADD(ms_bf_cluster_timestamp,                 Y("Cluster timestamp: %|1$.3f|s"));
  BF_ADD(ms_bf_cluster_position,                  Y("Cluster position: %1%"));
  BF_ADD(ms_bf_cluster_previous_size,             Y("Cluster previous size: %1%"));
  BF_ADD(ms_bf_codec_state,                       Y("Codec state: %1%"));
  BF_ADD(ms_bf_at,                                Y(" at %1%"));
  BF_ADD(ms_bf_size,                              Y(" size %1%"));
  BF_ADD(ms_bf_block_group_discard_padding,       Y("Discard padding: %|1$.3f|ms (%2%ns)"));
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

bool
kax_info_c::is_global(EbmlElement *l,
                      int level) {
  if (Is<EbmlVoid>(l)) {
    show_element(l, level, (ms_common_formats[ms_bf_ebmlvoid] % (l->ElementSize() - l->HeadSize())).str());
    return true;

  } else if (Is<EbmlCrc32>(l)) {
    show_element(l, level, "EbmlCrc32");
    return true;
  }

  return false;
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

#define FMT(Class, Formatter)  m_custom_element_value_formatters.insert({ Class::ClassInfos.GlobalId.GetValue(), Formatter });
#define PROC(Class, Processor) m_custom_element_processors.insert({ Class::ClassInfos.GlobalId.GetValue(), Processor });

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

void
kax_info_c::init_custom_element_value_formatters_and_processors() {
  m_custom_element_value_formatters.clear();
  m_custom_element_processors.clear();

  // Simple processors:
  PROC(KaxInfo, ([this](EbmlElement &e) -> bool { this->m_ts_scale = FindChildValue<KaxTimecodeScale, uint64_t>(static_cast<KaxInfo &>(e), TIMESTAMP_SCALE); return true; }));

  // More complex processors:
  PROC(KaxSeekHead, [this](EbmlElement &e) -> bool {
    if ((m_verbose < 2) && !m_use_gui) {
      show_element(&e, 1, Y("Seek head (subentries will be skipped)"));
      return false;
    }

    return true;
  });

  // Simple formatters:
  FMT(KaxSegmentUID,       &kax_info_c::format_binary_as_hex);
  FMT(KaxSegmentFamily,    &kax_info_c::format_binary_as_hex);
  FMT(KaxNextUID,          &kax_info_c::format_binary_as_hex);
  FMT(KaxSegmentFamily,    &kax_info_c::format_binary_as_hex);
  FMT(KaxPrevUID,          &kax_info_c::format_binary_as_hex);
  FMT(KaxSegment,          &kax_info_c::format_element_size);
  FMT(KaxFileData,         &kax_info_c::format_element_size);
  FMT(KaxChapterTimeStart, &kax_info_c::format_unsigned_integer_as_timestamp);
  FMT(KaxChapterTimeEnd,   &kax_info_c::format_unsigned_integer_as_timestamp);

  // More complex formatters:
  FMT(KaxDuration, [this](EbmlElement &e) -> std::string {
    auto duration = static_cast<KaxDuration &>(e).GetValue();
    return (boost::format("%|1$.3f|s (%2%)") % (duration * m_ts_scale / 1000000000.0) % format_timestamp(duration * m_ts_scale, 3)).str();
  });

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
}

#undef FMT
#undef PROC

void
kax_info_c::handle_audio_track(EbmlElement *&l3,
                               std::vector<std::string> &summary) {
  show_element(l3, 3, "Audio track");

  for (auto l4 : *static_cast<EbmlMaster *>(l3))
    if (Is<KaxAudioSamplingFreq>(l4)) {
      KaxAudioSamplingFreq &freq = *static_cast<KaxAudioSamplingFreq *>(l4);
      show_element(l4, 4, boost::format(Y("Sampling frequency: %1%")) % freq.GetValue());
      summary.push_back((boost::format(Y("sampling freq: %1%")) % freq.GetValue()).str());

    } else if (Is<KaxAudioOutputSamplingFreq>(l4)) {
      KaxAudioOutputSamplingFreq &ofreq = *static_cast<KaxAudioOutputSamplingFreq *>(l4);
      show_element(l4, 4, boost::format(Y("Output sampling frequency: %1%")) % ofreq.GetValue());
      summary.push_back((boost::format(Y("output sampling freq: %1%")) % ofreq.GetValue()).str());

    } else if (Is<KaxAudioChannels>(l4)) {
      KaxAudioChannels &channels = *static_cast<KaxAudioChannels *>(l4);
      show_element(l4, 4, boost::format(Y("Channels: %1%")) % channels.GetValue());
      summary.push_back((boost::format(Y("channels: %1%")) % channels.GetValue()).str());

    } else if (Is<KaxAudioPosition>(l4)) {
      KaxAudioPosition &positions = *static_cast<KaxAudioPosition *>(l4);
      show_element(l4, 4, boost::format(Y("Channel positions: %1%")) % format_binary(positions));

    } else if (Is<KaxAudioBitDepth>(l4)) {
      KaxAudioBitDepth &bps = *static_cast<KaxAudioBitDepth *>(l4);
      show_element(l4, 4, boost::format(Y("Bit depth: %1%")) % bps.GetValue());
      summary.push_back((boost::format(Y("bits per sample: %1%")) % bps.GetValue()).str());

    } else if (!is_global(l4, 4))
      show_unknown_element(l4, 4);
}

void
kax_info_c::handle_video_colour_master_meta(EbmlElement *&l5) {
  show_element(l5, 5, Y("Video colour mastering metadata"));

  for (auto l6: *static_cast<EbmlMaster *>(l5)) {
    if (Is<KaxVideoRChromaX>(l6))
      show_element(l6, 6, boost::format(Y("Red colour coordinate x: %1%")) % static_cast<KaxVideoRChromaX *>(l6)->GetValue());

    else if (Is<KaxVideoRChromaY>(l6))
      show_element(l6, 6, boost::format(Y("Red colour coordinate y: %1%")) % static_cast<KaxVideoRChromaY *>(l6)->GetValue());

    else if (Is<KaxVideoGChromaX>(l6))
      show_element(l6, 6, boost::format(Y("Green colour coordinate x: %1%")) % static_cast<KaxVideoGChromaX *>(l6)->GetValue());

    else if (Is<KaxVideoGChromaY>(l6))
      show_element(l6, 6, boost::format(Y("Green colour coordinate y: %1%")) % static_cast<KaxVideoGChromaY *>(l6)->GetValue());

    else if (Is<KaxVideoBChromaX>(l6))
      show_element(l6, 6, boost::format(Y("Blue colour coordinate x: %1%")) % static_cast<KaxVideoBChromaX *>(l6)->GetValue());

    else if (Is<KaxVideoBChromaY>(l6))
      show_element(l6, 6, boost::format(Y("Blue colour coordinate y: %1%")) % static_cast<KaxVideoBChromaY *>(l6)->GetValue());

    else if (Is<KaxVideoWhitePointChromaX>(l6))
      show_element(l6, 6, boost::format(Y("White colour coordinate x: %1%")) % static_cast<KaxVideoWhitePointChromaX *>(l6)->GetValue());

    else if (Is<KaxVideoWhitePointChromaY>(l6))
      show_element(l6, 6, boost::format(Y("White colour coordinate y: %1%")) % static_cast<KaxVideoWhitePointChromaY *>(l6)->GetValue());

    else if (Is<KaxVideoLuminanceMax>(l6))
      show_element(l6, 6, boost::format(Y("Max luminance: %1%")) % static_cast<KaxVideoLuminanceMax *>(l6)->GetValue());

    else if (Is<KaxVideoLuminanceMin>(l6))
      show_element(l6, 6, boost::format(Y("Min luminance: %1%")) % static_cast<KaxVideoLuminanceMin *>(l6)->GetValue());

    else if (!is_global(l6, 6))
      show_unknown_element(l6, 6);
  }
}

void
kax_info_c::handle_video_colour(EbmlElement *&l4) {
  show_element(l4, 4, Y("Video colour information"));

  for (auto l5 : *static_cast<EbmlMaster *>(l4)) {
    if (Is<KaxVideoColourMatrix>(l5))
      show_element(l5, 5, boost::format(Y("Colour matrix coefficients: %1%")) % static_cast<KaxVideoColourMatrix *>(l5)->GetValue());

    else if (Is<KaxVideoBitsPerChannel>(l5))
      show_element(l5, 5, boost::format(Y("Bits per channel: %1%")) % static_cast<KaxVideoBitsPerChannel *>(l5)->GetValue());

    else if (Is<KaxVideoChromaSubsampHorz>(l5))
      show_element(l5, 5, boost::format(Y("Horizontal chroma subsample: %1%")) % static_cast<KaxVideoChromaSubsampHorz *>(l5)->GetValue());

    else if (Is<KaxVideoChromaSubsampVert>(l5))
      show_element(l5, 5, boost::format(Y("Vertical chroma subsample: %1%")) % static_cast<KaxVideoChromaSubsampVert *>(l5)->GetValue());

    else if (Is<KaxVideoCbSubsampHorz>(l5))
      show_element(l5, 5, boost::format(Y("Horizontal Cb subsample: %1%")) % static_cast<KaxVideoCbSubsampHorz *>(l5)->GetValue());

    else if (Is<KaxVideoCbSubsampVert>(l5))
      show_element(l5, 5, boost::format(Y("Vertical Cb subsample: %1%")) % static_cast<KaxVideoCbSubsampVert *>(l5)->GetValue());

    else if (Is<KaxVideoChromaSitHorz>(l5))
      show_element(l5, 5, boost::format(Y("Horizontal chroma siting: %1%")) % static_cast<KaxVideoChromaSitHorz *>(l5)->GetValue());

    else if (Is<KaxVideoChromaSitVert>(l5))
      show_element(l5, 5, boost::format(Y("Vertical chroma siting: %1%")) % static_cast<KaxVideoChromaSitVert *>(l5)->GetValue());

    else if (Is<KaxVideoColourRange>(l5))
      show_element(l5, 5, boost::format(Y("Colour range: %1%")) % static_cast<KaxVideoColourRange *>(l5)->GetValue());

    else if (Is<KaxVideoColourTransferCharacter>(l5))
      show_element(l5, 5, boost::format(Y("Colour transfer: %1%")) % static_cast<KaxVideoColourTransferCharacter *>(l5)->GetValue());

    else if (Is<KaxVideoColourPrimaries>(l5))
      show_element(l5, 5, boost::format(Y("Colour primaries: %1%")) % static_cast<KaxVideoColourPrimaries *>(l5)->GetValue());

    else if (Is<KaxVideoColourMaxCLL>(l5))
      show_element(l5, 5, boost::format(Y("Max content light: %1%")) % static_cast<KaxVideoColourMaxCLL *>(l5)->GetValue());

    else if (Is<KaxVideoColourMaxFALL>(l5))
      show_element(l5, 5, boost::format(Y("Max frame light: %1%")) % static_cast<KaxVideoColourMaxFALL *>(l5)->GetValue());

    else if (Is<KaxVideoColourMasterMeta>(l5))
      handle_video_colour_master_meta(l5);

    else if (!is_global(l5, 5))
      show_unknown_element(l5, 5);
  }
}

void
kax_info_c::handle_video_projection(EbmlElement *&l4) {
  show_element(l4, 4, Y("Video projection"));

  for (auto l5 : *static_cast<EbmlMaster *>(l4)) {
    if (Is<KaxVideoProjectionType>(l5)) {
      auto value       = static_cast<KaxVideoProjectionType *>(l5)->GetValue();
      auto description = 0 == value ? Y("rectangular")
                       : 1 == value ? Y("equirectangular")
                       : 2 == value ? Y("cubemap")
                       : 3 == value ? Y("mesh")
                       :              Y("unknown");

      show_element(l5, 5, boost::format(Y("Projection type: %1% (%2%)")) % value % description);

    } else if (Is<KaxVideoProjectionPrivate>(l5))
      show_element(l5, 5, boost::format(Y("Projection's private data: %1%")) % to_hex(static_cast<KaxVideoProjectionPrivate *>(l5)));

    else if (Is<KaxVideoProjectionPoseYaw>(l5))
      show_element(l5, 5, boost::format(Y("Projection's yaw rotation: %1%")) % static_cast<KaxVideoProjectionPoseYaw *>(l5)->GetValue());

    else if (Is<KaxVideoProjectionPosePitch>(l5))
      show_element(l5, 5, boost::format(Y("Projection's pitch rotation: %1%")) % static_cast<KaxVideoProjectionPosePitch *>(l5)->GetValue());

    else if (Is<KaxVideoProjectionPoseRoll>(l5))
      show_element(l5, 5, boost::format(Y("Projection's roll rotation: %1%")) % static_cast<KaxVideoProjectionPoseRoll *>(l5)->GetValue());

    else if (!is_global(l5, 5))
      show_unknown_element(l5, 5);
  }
}

void
kax_info_c::handle_video_track(EbmlElement *&l3,
                               std::vector<std::string> &summary) {
  show_element(l3, 3, Y("Video track"));

  for (auto l4 : *static_cast<EbmlMaster *>(l3))
    if (Is<KaxVideoPixelWidth>(l4)) {
      KaxVideoPixelWidth &width = *static_cast<KaxVideoPixelWidth *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel width: %1%")) % width.GetValue());
      summary.push_back((boost::format(Y("pixel width: %1%")) % width.GetValue()).str());

    } else if (Is<KaxVideoPixelHeight>(l4)) {
      KaxVideoPixelHeight &height = *static_cast<KaxVideoPixelHeight *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel height: %1%")) % height.GetValue());
      summary.push_back((boost::format(Y("pixel height: %1%")) % height.GetValue()).str());

    } else if (Is<KaxVideoDisplayWidth>(l4)) {
      KaxVideoDisplayWidth &width = *static_cast<KaxVideoDisplayWidth *>(l4);
      show_element(l4, 4, boost::format(Y("Display width: %1%")) % width.GetValue());
      summary.push_back((boost::format(Y("display width: %1%")) % width.GetValue()).str());

    } else if (Is<KaxVideoDisplayHeight>(l4)) {
      KaxVideoDisplayHeight &height = *static_cast<KaxVideoDisplayHeight *>(l4);
      show_element(l4, 4, boost::format(Y("Display height: %1%")) % height.GetValue());
      summary.push_back((boost::format(Y("display height: %1%")) % height.GetValue()).str());

    } else if (Is<KaxVideoPixelCropLeft>(l4)) {
      KaxVideoPixelCropLeft &left = *static_cast<KaxVideoPixelCropLeft *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel crop left: %1%")) % left.GetValue());
      summary.push_back((boost::format(Y("pixel crop left: %1%")) % left.GetValue()).str());

    } else if (Is<KaxVideoPixelCropTop>(l4)) {
      KaxVideoPixelCropTop &top = *static_cast<KaxVideoPixelCropTop *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel crop top: %1%")) % top.GetValue());
      summary.push_back((boost::format(Y("pixel crop top: %1%")) % top.GetValue()).str());

    } else if (Is<KaxVideoPixelCropRight>(l4)) {
      KaxVideoPixelCropRight &right = *static_cast<KaxVideoPixelCropRight *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel crop right: %1%")) % right.GetValue());
      summary.push_back((boost::format(Y("pixel crop right: %1%")) % right.GetValue()).str());

    } else if (Is<KaxVideoPixelCropBottom>(l4)) {
      KaxVideoPixelCropBottom &bottom = *static_cast<KaxVideoPixelCropBottom *>(l4);
      show_element(l4, 4, boost::format(Y("Pixel crop bottom: %1%")) % bottom.GetValue());
      summary.push_back((boost::format(Y("pixel crop bottom: %1%")) % bottom.GetValue()).str());

    } else if (Is<KaxVideoDisplayUnit>(l4)) {
      auto unit = static_cast<KaxVideoDisplayUnit *>(l4)->GetValue();
      show_element(l4, 4,
                   boost::format(Y("Display unit: %1%%2%"))
                   % unit
                   % (  0 == unit ? Y(" (pixels)")
                      : 1 == unit ? Y(" (centimeters)")
                      : 2 == unit ? Y(" (inches)")
                      : 3 == unit ? Y(" (aspect ratio)")
                      :               ""));

    } else if (Is<KaxVideoGamma>(l4))
      show_element(l4, 4, boost::format(Y("Gamma: %1%")) % static_cast<KaxVideoGamma *>(l4)->GetValue());

    else if (Is<KaxVideoFlagInterlaced>(l4))
      show_element(l4, 4, boost::format(Y("Interlaced: %1%")) % static_cast<KaxVideoFlagInterlaced *>(l4)->GetValue());

    else if (Is<KaxVideoFieldOrder>(l4)) {
      auto field_order = static_cast<KaxVideoFieldOrder *>(l4)->GetValue();
      show_element(l4, 4,
                   boost::format(Y("Field order: %1% (%2%)"))
                   % field_order
                   % (  0  == field_order ? Y("progressive")
                      : 1  == field_order ? Y("top field displayed first, top field stored first")
                      : 2  == field_order ? Y("unspecified")
                      : 6  == field_order ? Y("bottom field displayed first, bottom field stored first")
                      : 9  == field_order ? Y("bottom field displayed first, top field stored first")
                      : 14 == field_order ? Y("top field displayed first, bottom field stored first")
                      :                       "unknown"));

    } else if (Is<KaxVideoStereoMode>(l4)) {
      auto stereo_mode = static_cast<KaxVideoStereoMode *>(l4)->GetValue();
      show_element(l4, 4,
                   boost::format(Y("Stereo mode: %1% (%2%)"))
                   % stereo_mode
                   % stereo_mode_c::translate(static_cast<stereo_mode_c::mode>(stereo_mode)));

    } else if (Is<KaxVideoAspectRatio>(l4)) {
      auto ar_type = static_cast<KaxVideoAspectRatio *>(l4)->GetValue();
      show_element(l4, 4,
                   boost::format(Y("Aspect ratio type: %1%%2%"))
                   % ar_type
                   % (  0 == ar_type ? Y(" (free resizing)")
                      : 1 == ar_type ? Y(" (keep aspect ratio)")
                      : 2 == ar_type ? Y(" (fixed)")
                      :                  ""));
    } else if (Is<KaxVideoColourSpace>(l4))
      show_element(l4, 4, boost::format(Y("Colour space: %1%")) % format_binary(*static_cast<KaxVideoColourSpace *>(l4)));

    else if (Is<KaxVideoFrameRate>(l4))
      show_element(l4, 4, boost::format(Y("Frame rate: %1%")) % static_cast<KaxVideoFrameRate *>(l4)->GetValue());

    else if (Is<KaxVideoColour>(l4))
      handle_video_colour(l4);

    else if (Is<KaxVideoProjection>(l4))
      handle_video_projection(l4);

    else if (!is_global(l4, 4))
      show_unknown_element(l4, 4);
}

void
kax_info_c::handle_content_encodings(EbmlElement *&l3) {
  show_element(l3, 3, Y("Content encodings"));

  for (auto l4 : *static_cast<EbmlMaster *>(l3))
    if (Is<KaxContentEncoding>(l4)) {
      show_element(l4, 4, Y("Content encoding"));

      for (auto l5 : *static_cast<EbmlMaster *>(l4))
        if (Is<KaxContentEncodingOrder>(l5))
          show_element(l5, 5, boost::format(Y("Order: %1%")) % static_cast<KaxContentEncodingOrder *>(l5)->GetValue());

        else if (Is<KaxContentEncodingScope>(l5)) {
          std::vector<std::string> scope;
          auto ce_scope = static_cast<KaxContentEncodingScope *>(l5)->GetValue();

          if ((ce_scope & 0x01) == 0x01)
            scope.push_back(Y("1: all frames"));
          if ((ce_scope & 0x02) == 0x02)
            scope.push_back(Y("2: codec private data"));
          if ((ce_scope & 0xfc) != 0x00)
            scope.push_back(Y("rest: unknown"));
          if (scope.empty())
            scope.push_back(Y("unknown"));
          show_element(l5, 5, boost::format(Y("Scope: %1% (%2%)")) % ce_scope % boost::join(scope, ", "));

        } else if (Is<KaxContentEncodingType>(l5)) {
          auto ce_type = static_cast<KaxContentEncodingType *>(l5)->GetValue();
          show_element(l5, 5,
                       boost::format(Y("Type: %1% (%2%)"))
                       % ce_type
                       % (  0 == ce_type ? Y("compression")
                          : 1 == ce_type ? Y("encryption")
                          :                Y("unknown")));

        } else if (Is<KaxContentCompression>(l5)) {
          show_element(l5, 5, Y("Content compression"));

          for (auto l6 : *static_cast<EbmlMaster *>(l5))
            if (Is<KaxContentCompAlgo>(l6)) {
              auto c_algo = static_cast<KaxContentCompAlgo *>(l6)->GetValue();
              show_element(l6, 6,
                           boost::format(Y("Algorithm: %1% (%2%)"))
                           % c_algo
                           % (  0 == c_algo ?   "ZLIB"
                              : 1 == c_algo ?   "bzLib"
                              : 2 == c_algo ?   "lzo1x"
                              : 3 == c_algo ? Y("header removal")
                              :               Y("unknown")));

            } else if (Is<KaxContentCompSettings>(l6))
              show_element(l6, 6, boost::format(Y("Settings: %1%")) % format_binary(*static_cast<KaxContentCompSettings *>(l6)));

            else if (!is_global(l6, 6))
              show_unknown_element(l6, 6);
        } else if (Is<KaxContentEncryption>(l5)) {
          show_element(l5, 5, Y("Content encryption"));

          for (auto l6 : *static_cast<EbmlMaster *>(l5))
            if (Is<KaxContentEncAlgo>(l6)) {
              auto e_algo = static_cast<KaxContentEncAlgo *>(l6)->GetValue();
              show_element(l6, 6,
                           boost::format(Y("Encryption algorithm: %1% (%2%)"))
                           % e_algo
                           % (  0 == e_algo ? Y("no encryption")
                              : 1 == e_algo ?   "DES"
                              : 2 == e_algo ?   "3DES"
                              : 3 == e_algo ?   "Twofish"
                              : 4 == e_algo ?   "Blowfish"
                              : 5 == e_algo ?   "AES"
                              :               Y("unknown")));

            } else if (Is<KaxContentEncKeyID>(l6))
              show_element(l6, 6, boost::format(Y("Encryption key ID: %1%")) % format_binary(*static_cast<KaxContentEncKeyID *>(l6)));

            else if (Is<KaxContentSigAlgo>(l6)) {
              auto s_algo = static_cast<KaxContentSigAlgo *>(l6)->GetValue();
              show_element(l6, 6,
                           boost::format(Y("Signature algorithm: %1% (%2%)"))
                           % s_algo
                           % (  0 == s_algo ? Y("no signature algorithm")
                              : 1 == s_algo ? Y("RSA")
                              :               Y("unknown")));

            } else if (Is<KaxContentSigHashAlgo>(l6)) {
              auto s_halgo = static_cast<KaxContentSigHashAlgo *>(l6)->GetValue();
              show_element(l6, 6,
                           boost::format(Y("Signature hash algorithm: %1% (%2%)"))
                           % s_halgo
                           % (  0 == s_halgo ? Y("no signature hash algorithm")
                              : 1 == s_halgo ? Y("SHA1-160")
                              : 2 == s_halgo ? Y("MD5")
                              :                Y("unknown")));

            } else if (Is<KaxContentSigKeyID>(l6))
              show_element(l6, 6, boost::format(Y("Signature key ID: %1%")) % format_binary(*static_cast<KaxContentSigKeyID *>(l6)));

            else if (Is<KaxContentSignature>(l6))
              show_element(l6, 6, boost::format(Y("Signature: %1%")) % format_binary(*static_cast<KaxContentSignature *>(l6)));

            else if (!is_global(l6, 6))
              show_unknown_element(l6, 6);

        } else if (!is_global(l5, 5))
          show_unknown_element(l5, 5);

    } else if (!is_global(l4, 4))
      show_unknown_element(l4, 4);
}

void
kax_info_c::handle_tracks(int &upper_lvl_el,
                          EbmlElement *&l1) {
  // Yep, we've found our KaxTracks element. Now find all tracks
  // contained in this segment.
  show_element(l1, 1, Y("Segment tracks"));

  m_mkvmerge_track_id        = 0;
  upper_lvl_el               = 0;
  EbmlElement *element_found = nullptr;
  auto m1                    = static_cast<EbmlMaster *>(l1);
  read_master(m1, EBML_CONTEXT(l1), upper_lvl_el, element_found);

  for (auto l2 : *m1)
    if (Is<KaxTrackEntry>(l2)) {
      // We actually found a track entry :) We're happy now.
      show_element(l2, 2, Y("A track"));

      std::vector<std::string> summary;
      std::string kax_codec_id, fourcc_buffer;
      auto track = std::make_shared<track_t>();

      for (auto l3 : *static_cast<EbmlMaster *>(l2))
        // Now evaluate the data belonging to this track
        if (Is<KaxTrackAudio>(l3))
          handle_audio_track(l3, summary);

        else if (Is<KaxTrackVideo>(l3))
          handle_video_track(l3, summary);

        else if (Is<KaxTrackNumber>(l3)) {
          track->tnum = static_cast<KaxTrackNumber *>(l3)->GetValue();

          auto existing_track = find_track(track->tnum);
          auto track_id       = m_mkvmerge_track_id;
          if (!existing_track) {
            track->mkvmerge_track_id = m_mkvmerge_track_id;
            ++m_mkvmerge_track_id;
            add_track(track);

          } else
            track_id = existing_track->mkvmerge_track_id;

          show_element(l3, 3, boost::format(Y("Track number: %1% (track ID for mkvmerge & mkvextract: %2%)")) % track->tnum % track_id);
          summary.push_back((boost::format(Y("mkvmerge/mkvextract track ID: %1%"))                            % track_id).str());

        } else if (Is<KaxTrackUID>(l3)) {
          track->tuid = static_cast<KaxTrackUID *>(l3)->GetValue();
          show_element(l3, 3, boost::format(Y("Track UID: %1%"))                                              % track->tuid);

        } else if (Is<KaxTrackType>(l3)) {
          auto ttype  = static_cast<KaxTrackType *>(l3)->GetValue();
          track->type = track_audio    == ttype ? 'a'
                      : track_video    == ttype ? 'v'
                      : track_subtitle == ttype ? 's'
                      : track_buttons  == ttype ? 'b'
                      :                           '?';
          show_element(l3, 3,
                       boost::format(Y("Track type: %1%"))
                       % (  'a' == track->type ? "audio"
                          : 'v' == track->type ? "video"
                          : 's' == track->type ? "subtitles"
                          : 'b' == track->type ? "buttons"
                          :                      "unknown"));

        } else if (Is<KaxTrackFlagEnabled>(l3))
          show_element(l3, 3, boost::format(Y("Enabled: %1%"))                % static_cast<KaxTrackFlagEnabled *>(l3)->GetValue());

        else if (Is<KaxTrackName>(l3))
          show_element(l3, 3, boost::format(Y("Name: %1%"))                   % static_cast<KaxTrackName *>(l3)->GetValueUTF8());

        else if (Is<KaxCodecID>(l3)) {
          kax_codec_id = static_cast<KaxCodecID *>(l3)->GetValue();
          show_element(l3, 3, boost::format(Y("Codec ID: %1%"))               % kax_codec_id);

        } else if (Is<KaxCodecPrivate>(l3)) {
          KaxCodecPrivate &c_priv = *static_cast<KaxCodecPrivate *>(l3);
          fourcc_buffer = create_codec_dependent_private_info(c_priv, track->type, kax_codec_id);

          if (m_calc_checksums && !m_show_summary)
            fourcc_buffer += (boost::format(Y(" (adler: 0x%|1$08x|)"))        % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, c_priv.GetBuffer(), c_priv.GetSize())).str();

          if (m_show_hexdump)
            fourcc_buffer += create_hexdump(c_priv.GetBuffer(), c_priv.GetSize());

          show_element(l3, 3, boost::format(Y("CodecPrivate, length %1%%2%")) % c_priv.GetSize() % fourcc_buffer);

        } else if (Is<KaxCodecName>(l3))
          show_element(l3, 3, boost::format(Y("Codec name: %1%"))             % static_cast<KaxCodecName *>(l3)->GetValueUTF8());

        else if (Is<KaxCodecSettings>(l3))
          show_element(l3, 3, boost::format(Y("Codec settings: %1%"))         % static_cast<KaxCodecSettings *>(l3)->GetValueUTF8());

        else if (Is<KaxCodecInfoURL>(l3))
          show_element(l3, 3, boost::format(Y("Codec info URL: %1%"))         % static_cast<KaxCodecInfoURL *>(l3)->GetValue());

        else if (Is<KaxCodecDownloadURL>(l3))
          show_element(l3, 3, boost::format(Y("Codec download URL: %1%"))     % static_cast<KaxCodecDownloadURL *>(l3)->GetValue());

        else if (Is<KaxCodecDecodeAll>(l3))
          show_element(l3, 3, boost::format(Y("Codec decode all: %1%"))       % static_cast<KaxCodecDecodeAll *>(l3)->GetValue());

        else if (Is<KaxTrackOverlay>(l3))
          show_element(l3, 3, boost::format(Y("Track overlay: %1%"))          % static_cast<KaxTrackOverlay *>(l3)->GetValue());

        else if (Is<KaxTrackMinCache>(l3))
          show_element(l3, 3, boost::format(Y("MinCache: %1%"))               % static_cast<KaxTrackMinCache *>(l3)->GetValue());

        else if (Is<KaxTrackMaxCache>(l3))
          show_element(l3, 3, boost::format(Y("MaxCache: %1%"))               % static_cast<KaxTrackMaxCache *>(l3)->GetValue());

        else if (Is<KaxTrackDefaultDuration>(l3)) {
          track->default_duration = static_cast<KaxTrackDefaultDuration *>(l3)->GetValue();
          show_element(l3, 3,
                       boost::format(Y("Default duration: %|1$.3f|ms (%|2$.3f| frames/fields per second for a video track)"))
                       % (static_cast<double>(track->default_duration) / 1000000.0)
                       % (1000000000.0 / static_cast<double>(track->default_duration)));
          summary.push_back((boost::format(Y("default duration: %|1$.3f|ms (%|2$.3f| frames/fields per second for a video track)"))
                             % (static_cast<double>(track->default_duration) / 1000000.0)
                             % (1000000000.0 / static_cast<double>(track->default_duration))
                             ).str());

        } else if (Is<KaxTrackFlagLacing>(l3))
          show_element(l3, 3, boost::format(Y("Lacing flag: %1%"))          % static_cast<KaxTrackFlagLacing *>(l3)->GetValue());

        else if (Is<KaxTrackFlagDefault>(l3))
          show_element(l3, 3, boost::format(Y("Default flag: %1%"))         % static_cast<KaxTrackFlagDefault *>(l3)->GetValue());

        else if (Is<KaxTrackFlagForced>(l3))
          show_element(l3, 3, boost::format(Y("Forced flag: %1%"))          % static_cast<KaxTrackFlagForced *>(l3)->GetValue());

        else if (Is<KaxTrackLanguage>(l3)) {
          auto language = static_cast<KaxTrackLanguage *>(l3)->GetValue();
          show_element(l3, 3, boost::format(Y("Language: %1%"))             % language);
          summary.push_back((boost::format(Y("language: %1%"))              % language).str());

        } else if (Is<KaxTrackTimecodeScale>(l3))
          show_element(l3, 3, boost::format(Y("Timestamp scale: %1%"))      % static_cast<KaxTrackTimecodeScale *>(l3)->GetValue());

        else if (Is<KaxMaxBlockAdditionID>(l3))
          show_element(l3, 3, boost::format(Y("Max BlockAddition ID: %1%")) % static_cast<KaxMaxBlockAdditionID *>(l3)->GetValue());

        else if (Is<KaxContentEncodings>(l3))
          handle_content_encodings(l3);

        else if (Is<KaxCodecDelay>(l3)) {
          auto value = static_cast<KaxCodecDelay *>(l3)->GetValue();
          show_element(l3, 3, boost::format(Y("Codec delay: %|1$.3f|ms (%2%ns)")) % (static_cast<double>(value) / 1000000.0) % value);

        } else if (Is<KaxSeekPreRoll>(l3)) {
          auto value = static_cast<KaxSeekPreRoll *>(l3)->GetValue();
          show_element(l3, 3, boost::format(Y("Seek pre-roll: %|1$.3f|ms (%2%ns)")) % (static_cast<double>(value) / 1000000.0) % value);

        } else if (!is_global(l3, 3))
          show_unknown_element(l3, 3);

      if (m_show_summary)
        m_out->write((boost::format(Y("Track %1%: %2%, codec ID: %3%%4%%5%%6%\n"))
                      % track->tnum
                      % (  'a' == track->type ? Y("audio")
                           : 'v' == track->type ? Y("video")
                           : 's' == track->type ? Y("subtitles")
                           : 'b' == track->type ? Y("buttons")
                           :                      Y("unknown"))
                      % kax_codec_id
                      % fourcc_buffer
                      % (summary.empty() ? "" : ", ")
                      % boost::join(summary, ", ")).str());

    } else if (!is_global(l2, 2))
      show_unknown_element(l2, 2);
}

void
kax_info_c::handle_cues(int &upper_lvl_el,
                        EbmlElement *&l1) {
  if (m_verbose < 2) {
    show_element(l1, 1, Y("Cues (subentries will be skipped)"));
    return;
  }

  show_element(l1, 1, "Cues");

  upper_lvl_el               = 0;
  EbmlElement *element_found = nullptr;
  auto m1                    = static_cast<EbmlMaster *>(l1);
  read_master(m1, EBML_CONTEXT(l1), upper_lvl_el, element_found);

  for (auto l2 : *m1)
    if (Is<KaxCuePoint>(l2)) {
      show_element(l2, 2, Y("Cue point"));

      for (auto l3 : *static_cast<EbmlMaster *>(l2))
        if (Is<KaxCueTime>(l3))
          show_element(l3, 3, boost::format(Y("Cue time: %|1$.3f|s")) % (m_ts_scale * static_cast<double>(static_cast<KaxCueTime *>(l3)->GetValue()) / 1000000000.0));

        else if (Is<KaxCueTrackPositions>(l3)) {
          show_element(l3, 3, Y("Cue track positions"));

          for (auto l4 : *static_cast<EbmlMaster *>(l3))
            if (Is<KaxCueTrack>(l4))
              show_element(l4, 4, boost::format(Y("Cue track: %1%"))            % static_cast<KaxCueTrack *>(l4)->GetValue());

            else if (Is<KaxCueClusterPosition>(l4))
              show_element(l4, 4, boost::format(Y("Cue cluster position: %1%")) % static_cast<KaxCueClusterPosition *>(l4)->GetValue());

            else if (Is<KaxCueRelativePosition>(l4))
              show_element(l4, 4, boost::format(Y("Cue relative position: %1%")) % static_cast<KaxCueRelativePosition *>(l4)->GetValue());

            else if (Is<KaxCueDuration>(l4))
              show_element(l4, 4, boost::format(Y("Cue duration: %1%"))         % format_timestamp(static_cast<KaxCueDuration *>(l4)->GetValue() * m_ts_scale));

            else if (Is<KaxCueBlockNumber>(l4))
              show_element(l4, 4, boost::format(Y("Cue block number: %1%"))     % static_cast<KaxCueBlockNumber *>(l4)->GetValue());

            else if (Is<KaxCueCodecState>(l4))
              show_element(l4, 4, boost::format(Y("Cue codec state: %1%"))      % static_cast<KaxCueCodecState *>(l4)->GetValue());

            else if (Is<KaxCueReference>(l4)) {
              show_element(l4, 4, Y("Cue reference"));

              for (auto l5 : *static_cast<EbmlMaster *>(l4))
                if (Is<KaxCueRefTime>(l5))
                  show_element(l5, 5, boost::format(Y("Cue ref time: %|1$.3f|s"))  % m_ts_scale % (static_cast<KaxCueRefTime *>(l5)->GetValue() / 1000000000.0));

                else if (Is<KaxCueRefCluster>(l5))
                  show_element(l5, 5, boost::format(Y("Cue ref cluster: %1%"))     % static_cast<KaxCueRefCluster *>(l5)->GetValue());

                else if (Is<KaxCueRefNumber>(l5))
                  show_element(l5, 5, boost::format(Y("Cue ref number: %1%"))      % static_cast<KaxCueRefNumber *>(l5)->GetValue());

                else if (Is<KaxCueRefCodecState>(l5))
                  show_element(l5, 5, boost::format(Y("Cue ref codec state: %1%")) % static_cast<KaxCueRefCodecState *>(l5)->GetValue());

                else if (!is_global(l5, 5))
                  show_unknown_element(l5, 5);

            } else if (!is_global(l4, 4))
              show_unknown_element(l4, 4);

        } else if (!is_global(l3, 3))
          show_unknown_element(l3, 3);

    } else if (!is_global(l2, 2))
      show_unknown_element(l2, 2);
}

void
kax_info_c::handle_silent_track(EbmlElement *&l2) {
  show_element(l2, 2, "Silent Tracks");

  for (auto l3 : *static_cast<EbmlMaster *>(l2))
    if (Is<KaxClusterSilentTrackNumber>(l3))
      show_element(l3, 3, boost::format(Y("Silent Track Number: %1%")) % static_cast<KaxClusterSilentTrackNumber *>(l3)->GetValue());

    else if (!is_global(l3, 3))
      show_unknown_element(l3, 3);
}

void
kax_info_c::handle_block_group(EbmlElement *&l2,
                               KaxCluster *&cluster) {
  show_element(l2, 2, Y("Block group"));

  std::vector<int> frame_sizes;
  std::vector<uint32_t> frame_adlers;
  std::vector<std::string> frame_hexdumps;

  auto num_references  = 0u;
  int64_t lf_timestamp = 0;
  int64_t lf_tnum      = 0;
  int64_t frame_pos    = 0;

  float bduration      = -1.0;

  for (auto l3 : *static_cast<EbmlMaster *>(l2))
    if (Is<KaxBlock>(l3)) {
      KaxBlock &block = *static_cast<KaxBlock *>(l3);
      block.SetParent(*cluster);

      lf_timestamp = block.GlobalTimecode();
      lf_tnum      = block.TrackNum();
      bduration    = -1.0;
      frame_pos    = block.GetElementPosition() + block.ElementSize();

      show_element(l3, 3,
                   ms_common_formats[ms_bf_block_group_block_summary]
                   % block.TrackNum()
                   % block.NumberFrames()
                   % (static_cast<double>(lf_timestamp) / 1000000000.0)
                   % format_timestamp(lf_timestamp, 3));

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

        frame_sizes.push_back(data.Size());
        frame_adlers.push_back(adler);
        frame_hexdumps.push_back(hex);
        frame_pos -= data.Size();
      }

    } else if (Is<KaxBlockDuration>(l3)) {
      auto duration = static_cast<KaxBlockDuration *>(l3)->GetValue();
      bduration     = static_cast<double>(duration) * m_ts_scale / 1000000.0;
      show_element(l3, 3, ms_common_formats[ms_bf_block_group_duration] % (duration * m_ts_scale / 1000000) % (duration * m_ts_scale % 1000000));

    } else if (Is<KaxReferenceBlock>(l3)) {
      ++num_references;

      int64_t reference = static_cast<KaxReferenceBlock *>(l3)->GetValue() * m_ts_scale;

      if (0 >= reference)
        show_element(l3, 3, ms_common_formats[ms_bf_block_group_reference_1] % (std::abs(reference) / 1000000) % (std::abs(reference) % 1000000));

      else if (0 < reference)
        show_element(l3, 3, ms_common_formats[ms_bf_block_group_reference_2] % (reference / 1000000) % (reference % 1000000));

    } else if (Is<KaxReferencePriority>(l3))
      show_element(l3, 3, ms_common_formats[ms_bf_block_group_reference_priority] % static_cast<KaxReferencePriority *>(l3)->GetValue());

    else if (Is<KaxBlockVirtual>(l3))
      show_element(l3, 3, ms_common_formats[ms_bf_block_group_virtual]            % format_binary(*static_cast<KaxBlockVirtual *>(l3)));

    else if (Is<KaxReferenceVirtual>(l3))
      show_element(l3, 3, ms_common_formats[ms_bf_block_group_reference_virtual]  % static_cast<KaxReferenceVirtual *>(l3)->GetValue());

    else if (Is<KaxCodecState>(l3))
      show_element(l3, 3, ms_common_formats[ms_bf_codec_state]                    % format_binary(*static_cast<KaxCodecState *>(l3)));

    else if (Is<KaxDiscardPadding>(l3)) {
      auto value = static_cast<KaxDiscardPadding *>(l3)->GetValue();
      show_element(l3, 3, ms_common_formats[ms_bf_block_group_discard_padding]    % (static_cast<double>(value) / 1000000.0) % value);
    }

    else if (Is<KaxBlockAdditions>(l3)) {
      show_element(l3, 3, Y("Additions"));

      for (auto l4 : *static_cast<EbmlMaster *>(l3))
        if (Is<KaxBlockMore>(l4)) {
          show_element(l4, 4, Y("More"));

          for (auto l5 : *static_cast<EbmlMaster *>(l4))
            if (Is<KaxBlockAddID>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_add_id]     % static_cast<KaxBlockAddID *>(l5)->GetValue());

            else if (Is<KaxBlockAdditional>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_additional] % format_binary(*static_cast<KaxBlockAdditional *>(l5)));

            else if (!is_global(l5, 5))
              show_unknown_element(l5, 5);

        } else if (!is_global(l4, 4))
          show_unknown_element(l4, 4);

    } else if (Is<KaxSlices>(l3)) {
      show_element(l3, 3, Y("Slices"));

      for (auto l4 : *static_cast<EbmlMaster *>(l3))
        if (Is<KaxTimeSlice>(l4)) {
          show_element(l4, 4, Y("Time slice"));

          for (auto l5 : *static_cast<EbmlMaster *>(l4))
            if (Is<KaxSliceLaceNumber>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_slice_lace]     % static_cast<KaxSliceLaceNumber *>(l5)->GetValue());

            else if (Is<KaxSliceFrameNumber>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_slice_frame]    % static_cast<KaxSliceFrameNumber *>(l5)->GetValue());

            else if (Is<KaxSliceDelay>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_slice_delay]    % (static_cast<double>(static_cast<KaxSliceDelay *>(l5)->GetValue()) * m_ts_scale / 1000000.0));

            else if (Is<KaxSliceDuration>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_slice_duration] % (static_cast<double>(static_cast<KaxSliceDuration *>(l5)->GetValue()) * m_ts_scale / 1000000.0));

            else if (Is<KaxSliceBlockAddID>(l5))
              show_element(l5, 5, ms_common_formats[ms_bf_block_group_slice_add_id]   % static_cast<KaxSliceBlockAddID *>(l5)->GetValue());

            else if (!is_global(l5, 5))
              show_unknown_element(l5, 5);

        } else if (!is_global(l4, 4))
          show_unknown_element(l4, 4);

    } else if (!is_global(l3, 3))
      show_unknown_element(l3, 3);

  if (m_show_summary) {
    std::string position;
    std::size_t fidx;

    for (fidx = 0; fidx < frame_sizes.size(); fidx++) {
      if (1 <= m_verbose) {
        position   = (ms_common_formats[ms_bf_block_group_summary_position] % frame_pos).str();
        frame_pos += frame_sizes[fidx];
      }

      if (bduration != -1.0)
        m_out->write((ms_common_formats[ms_bf_block_group_summary_with_duration]
                      % (num_references >= 2 ? 'B' : num_references == 1 ? 'P' : 'I')
                      % lf_tnum
                      % std::llround(lf_timestamp / 1000000.0)
                      % format_timestamp(lf_timestamp, 3)
                      % bduration
                      % frame_sizes[fidx]
                      % frame_adlers[fidx]
                      % frame_hexdumps[fidx]
                      % position).str());
      else
        m_out->write((ms_common_formats[ms_bf_block_group_summary_no_duration]
                      % (num_references >= 2 ? 'B' : num_references == 1 ? 'P' : 'I')
                      % lf_tnum
                      % std::llround(lf_timestamp / 1000000.0)
                      % format_timestamp(lf_timestamp, 3)
                      % frame_sizes[fidx]
                      % frame_adlers[fidx]
                      % frame_hexdumps[fidx]
                      % position).str());
    }

  } else if (m_verbose > 2)
    show_element(nullptr, 2,
                 ms_common_formats[ms_bf_block_group_summary_v2]
                 % (num_references >= 2 ? 'B' : num_references == 1 ? 'P' : 'I')
                 % lf_tnum
                 % std::llround(lf_timestamp / 1000000.0));

  auto &tinfo = m_track_info[lf_tnum];

  tinfo.m_blocks                                          += frame_sizes.size();
  tinfo.m_blocks_by_ref_num[std::min(num_references, 2u)] += frame_sizes.size();
  tinfo.m_min_timestamp                                    = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : lf_timestamp, lf_timestamp);
  tinfo.m_size                                            += boost::accumulate(frame_sizes, 0);

  if (tinfo.m_max_timestamp && (*tinfo.m_max_timestamp >= lf_timestamp))
    return;

  tinfo.m_max_timestamp = lf_timestamp;

  if (-1 == bduration)
    tinfo.m_add_duration_for_n_packets  = frame_sizes.size();
  else {
    *tinfo.m_max_timestamp             += bduration * 1000000.0;
    tinfo.m_add_duration_for_n_packets  = 0;
  }
}

void
kax_info_c::handle_simple_block(EbmlElement *&l2,
                                KaxCluster *&cluster) {
  std::vector<int> frame_sizes;
  std::vector<uint32_t> frame_adlers;

  KaxSimpleBlock &block = *static_cast<KaxSimpleBlock *>(l2);
  block.SetParent(*cluster);

  int64_t frame_pos   = block.GetElementPosition() + block.ElementSize();
  auto timestamp_ns   = mtx::math::to_signed(block.GlobalTimecode());
  auto timestamp_ms   = std::llround(static_cast<double>(timestamp_ns) / 1000000.0);
  track_info_t &tinfo = m_track_info[block.TrackNum()];

  std::string info;
  if (block.IsKeyframe())
    info = Y("key, ");
  if (block.IsDiscardable())
    info += Y("discardable, ");

  show_element(l2, 2,
               ms_common_formats[ms_bf_simple_block_basics]
               % info
               % block.TrackNum()
               % block.NumberFrames()
               % (timestamp_ns / 1000000000.0)
               % format_timestamp(timestamp_ns, 3));

  int i;
  for (i = 0; i < (int)block.NumberFrames(); i++) {
    DataBuffer &data = block.GetBuffer(i);
    uint32_t adler   = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, data.Buffer(), data.Size());

    std::string adler_str;
    if (m_calc_checksums)
      adler_str = (ms_common_formats[ms_bf_simple_block_adler] % adler).str();

    std::string hex;
    if (m_show_hexdump)
      hex = create_hexdump(data.Buffer(), data.Size());

    show_element(nullptr, 3, ms_common_formats[ms_bf_simple_block_frame] % data.Size() % adler_str % hex);

    frame_sizes.push_back(data.Size());
    frame_adlers.push_back(adler);
    frame_pos -= data.Size();
  }

  if (m_show_summary) {
    std::string position;
    std::size_t fidx;

    for (fidx = 0; fidx < frame_sizes.size(); fidx++) {
      if (1 <= m_verbose) {
        position   = (ms_common_formats[ms_bf_simple_block_position] % frame_pos).str();
        frame_pos += frame_sizes[fidx];
      }

      m_out->write((ms_common_formats[ms_bf_simple_block_summary]
                    % (block.IsKeyframe() ? 'I' : block.IsDiscardable() ? 'B' : 'P')
                    % block.TrackNum()
                    % timestamp_ms
                    % format_timestamp(timestamp_ns, 3)
                    % frame_sizes[fidx]
                    % frame_adlers[fidx]
                    % position).str());
    }

  } else if (m_verbose > 2)
    show_element(nullptr, 2,
                 ms_common_formats[ms_bf_simple_block_summary_v2]
                 % (block.IsKeyframe() ? 'I' : block.IsDiscardable() ? 'B' : 'P')
                 % block.TrackNum()
                 % timestamp_ms);

  tinfo.m_blocks                                                                    += block.NumberFrames();
  tinfo.m_blocks_by_ref_num[block.IsKeyframe() ? 0 : block.IsDiscardable() ? 2 : 1] += block.NumberFrames();
  tinfo.m_min_timestamp                                                              = std::min(tinfo.m_min_timestamp ? *tinfo.m_min_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_max_timestamp                                                              = std::max(tinfo.m_max_timestamp ? *tinfo.m_max_timestamp : static_cast<int64_t>(timestamp_ns), static_cast<int64_t>(timestamp_ns));
  tinfo.m_add_duration_for_n_packets                                                 = block.NumberFrames();
  tinfo.m_size                                                                      += boost::accumulate(frame_sizes, 0);
}

void
kax_info_c::handle_cluster(int &upper_lvl_el,
                           EbmlElement *&l1,
                           int64_t file_size) {
  auto cluster = static_cast<KaxCluster *>(l1);

  if (m_use_gui)
    ui_show_progress(100 * cluster->GetElementPosition() / file_size, Y("Parsing file"));

  upper_lvl_el               = 0;
  EbmlElement *element_found = nullptr;
  auto m1                    = static_cast<EbmlMaster *>(l1);
  read_master(m1, EBML_CONTEXT(l1), upper_lvl_el, element_found);

  cluster->InitTimecode(FindChildValue<KaxClusterTimecode>(m1), m_ts_scale);

  for (auto l2 : *m1)
    if (Is<KaxClusterTimecode>(l2))
      show_element(l2, 2, ms_common_formats[ms_bf_cluster_timestamp]     % (static_cast<double>(static_cast<KaxClusterTimecode *>(l2)->GetValue()) * m_ts_scale / 1000000000.0));

    else if (Is<KaxClusterPosition>(l2))
      show_element(l2, 2, ms_common_formats[ms_bf_cluster_position]      % static_cast<KaxClusterPosition *>(l2)->GetValue());

    else if (Is<KaxClusterPrevSize>(l2))
      show_element(l2, 2, ms_common_formats[ms_bf_cluster_previous_size] % static_cast<KaxClusterPrevSize *>(l2)->GetValue());

    else if (Is<KaxClusterSilentTracks>(l2))
      handle_silent_track(l2);

    else if (Is<KaxBlockGroup>(l2))
      handle_block_group(l2, cluster);

    else if (Is<KaxSimpleBlock>(l2))
      handle_simple_block(l2, cluster);

    else if (!is_global(l2, 2))
      show_unknown_element(l2, 2);
}

kax_info_c::result_e
kax_info_c::handle_segment(EbmlElement *l0) {
  if (!l0->IsFiniteSize())
    show_element(l0, 0, Y("Segment, size unknown"));
  else
    show_element(l0, 0, boost::format(Y("Segment, size %1%")) % l0->GetSize());
  // ui_show_element(*l0);

  auto file_size         = m_in->get_size();
  auto l1                = static_cast<EbmlElement *>(nullptr);
  auto upper_lvl_el      = 0;
  auto kax_file          = std::make_shared<kax_file_c>(*m_in);
  m_level                = 1;

  kax_file->set_segment_end(*l0);


  // Prevent reporting "first timestamp after resync":
  kax_file->set_timestamp_scale(-1);

  while ((l1 = kax_file->read_next_level1_element())) {
    std::shared_ptr<EbmlElement> af_l1(l1);

    if (Is<KaxTracks>(l1))
      handle_tracks(upper_lvl_el, l1);

    else if (Is<KaxCluster>(l1)) {
      show_element(l1, 1, Y("Cluster"));
      if ((m_verbose == 0) && !m_show_summary)
        return result_e::succeeded;
      handle_cluster(upper_lvl_el, l1, file_size);

    } else if (Is<KaxCues>(l1))
      handle_cues(upper_lvl_el, l1);

    else if (Is<EbmlVoid>(l1) || Is<EbmlCrc32>(l1) || Is<KaxInfo>(l1) || Is<KaxSeekHead>(l1) || Is<KaxAttachments>(l1) || Is<KaxChapters>(l1) || Is<KaxTags>(l1))
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
  auto processor = m_custom_element_processors.find(EbmlId(e).GetValue());
  if (processor != m_custom_element_processors.end())
    if (!processor->second(e))
      return;

  ui_show_element(e);

  if (!dynamic_cast<EbmlMaster *>(&e))
    return;

  ++m_level;

  for (auto child : static_cast<EbmlMaster &>(e))
    handle_elements_generic(*child);

  --m_level;
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
