/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   USF subtitle reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <sstream>

#include "common/codec.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/xml/ebml_converter.h"
#include "input/r_usf.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_textsubs.h"

bool
usf_reader_c::probe_file() {
  std::string line, content;

  while ((content.length() < 1000) && m_in->getline2(line, 1000))
    content += line;

  if (   (content.find("<?xml") == std::string::npos)
      && (content.find("<!--")  == std::string::npos))
    return false;

  auto doc = mtx::xml::load_file(m_in->get_file_name(), pugi::parse_default | pugi::parse_declaration | pugi::parse_doctype | pugi::parse_pi | pugi::parse_comments, 10 * 1024 * 1024);
  return doc && (std::string{ doc->document_element().name() } == "USFSubtitles");
}

void
usf_reader_c::read_headers() {
  try {
    auto doc = mtx::xml::load_file(m_in->get_file_name(), pugi::parse_default | pugi::parse_declaration | pugi::parse_doctype | pugi::parse_pi | pugi::parse_comments);

    parse_metadata(doc);
    parse_subtitles(doc);
    create_codec_private(doc);

    for (auto track : m_tracks) {
      std::stable_sort(track->m_entries.begin(), track->m_entries.end());
      track->m_current_entry = track->m_entries.begin();
      if (!m_longest_track || (m_longest_track->m_entries.size() < track->m_entries.size()))
        m_longest_track = track;

      if (m_default_language.is_valid() && !track->m_language.is_valid())
        track->m_language = m_default_language;
    }

  } catch (mtx::xml::exception &ex) {
    throw mtx::input::extended_x(ex.what());

  } catch (mtx::mm_io::exception &) {
    throw mtx::input::open_x();
  }

  show_demuxer_info();
}

void
usf_reader_c::parse_metadata(mtx::xml::document_cptr &doc) {
  auto attribute = doc->document_element().child("metadata").child("language").attribute("code");
  if (attribute && !std::string{attribute.value()}.empty()) {
    auto language = mtx::bcp47::language_c::parse(attribute.value());

    if (language.is_valid())
      m_default_language = language;

    else if (!g_identifying)
      mxwarn_fn(m_ti.m_fname, fmt::format(FY("The default language code '{0}' is not a valid ISO 639-2 language code and will be ignored.\n"), attribute.value()));
  }
}

void
usf_reader_c::parse_subtitles(mtx::xml::document_cptr &doc) {
  for (auto subtitles = doc->document_element().child("subtitles"); subtitles; subtitles = subtitles.next_sibling("subtitles")) {
    auto track = std::make_shared<usf_track_t>();
    m_tracks.push_back(track);

    auto attribute = subtitles.child("language").attribute("code");
    if (attribute && !std::string{attribute.value()}.empty()) {
      auto language = mtx::bcp47::language_c::parse(attribute.value());

      if (language.is_valid())
        track->m_language = language;

      else if (!g_identifying)
        mxwarn_tid(m_ti.m_fname, m_tracks.size() - 1, fmt::format(FY("The language code '{0}' is not a valid ISO 639-2 language code and will be ignored.\n"), attribute.value()));
    }

    for (auto subtitle = subtitles.child("subtitle"); subtitle; subtitle = subtitle.next_sibling("subtitle")) {
      usf_entry_t entry;
      int64_t duration = -1;

      attribute = subtitle.attribute("start");
      if (attribute)
        entry.m_start = try_to_parse_timestamp(attribute.value());

      attribute = subtitle.attribute("stop");
      if (attribute)
        entry.m_end = try_to_parse_timestamp(attribute.value());

      attribute = subtitle.attribute("duration");
      if (attribute)
        duration = try_to_parse_timestamp(attribute.value());

      if ((-1 == entry.m_end) && (-1 != entry.m_start) && (-1 != duration))
        entry.m_end = entry.m_start + duration;

      std::stringstream out;
      for (auto node : subtitle)
        node.print(out, "", pugi::format_default | pugi::format_raw);
      entry.m_text = out.str();

      track->m_entries.push_back(entry);
      track->m_byte_size += entry.m_text.size();
    }
  }
}

void
usf_reader_c::create_codec_private(mtx::xml::document_cptr &doc) {
  auto root = doc->document_element();
  while (auto subtitles = root.child("subtitles"))
    root.remove_child(subtitles);

  std::stringstream out;
  doc->save(out, "", pugi::format_default | pugi::format_raw);
  m_private_data = out.str();
}

void
usf_reader_c::create_packetizer(int64_t tid) {
  if ((0 > tid) || (m_tracks.size() <= static_cast<size_t>(tid)))
    return;

  auto track = m_tracks[tid];

  if (!demuxing_requested('s', tid, track->m_language) || (-1 != track->m_ptzr))
    return;

  m_bytes_to_process += track->m_byte_size;
  m_ti.m_private_data = memory_c::clone(m_private_data);
  m_ti.m_language     = track->m_language;
  track->m_ptzr       = add_packetizer(new textsubs_packetizer_c(this, m_ti, MKV_S_TEXTUSF));
  show_packetizer_info(tid, ptzr(track->m_ptzr));
}

void
usf_reader_c::create_packetizers() {
  size_t i;

  for (i = 0; m_tracks.size() > i; ++i)
    create_packetizer(i);
}

file_status_e
usf_reader_c::read(generic_packetizer_c *read_packetizer,
                   bool) {
  auto track_itr = std::find_if(m_tracks.begin(), m_tracks.end(), [this, read_packetizer](auto &tr) { return (-1 != tr->m_ptzr) && (&ptzr(tr->m_ptzr) == read_packetizer); });
  if (track_itr == m_tracks.end())
    return FILE_STATUS_DONE;

  auto &track = *track_itr;

  if (track->m_entries.end() == track->m_current_entry)
    return flush_packetizer(track->m_ptzr);

  auto &entry = *track->m_current_entry;
  ptzr(track->m_ptzr).process(std::make_shared<packet_t>(memory_c::clone(entry.m_text), entry.m_start, entry.m_end - entry.m_start));
  ++track->m_current_entry;

  m_bytes_processed += entry.m_text.size();

  if (track->m_entries.end() == track->m_current_entry)
    return flush_packetizer(track->m_ptzr);

  return FILE_STATUS_MOREDATA;
}

int64_t
usf_reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
usf_reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

int64_t
usf_reader_c::try_to_parse_timestamp(const char *s) {
  int64_t timestamp;

  if (!mtx::string::parse_timestamp(s, timestamp))
    throw mtx::xml::conversion_x{Y("Invalid start or stop timestamp")};

  return timestamp;
}

void
usf_reader_c::identify() {
  size_t i;

  id_result_container();

  for (i = 0; m_tracks.size() > i; ++i) {
    auto track = m_tracks[i];
    auto info  = mtx::id::info_c{};

    info.add(mtx::id::language, track->m_language.get_iso639_alpha_3_code());

    id_result_track(i, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_USF, "USF"), info.get());
  }
}
