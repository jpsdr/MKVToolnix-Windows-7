/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the generic_reader_c implementation

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/mm_proxy_io.h"
#include "common/tags/tags.h"
#include "merge/generic_packetizer.h"
#include "merge/generic_reader.h"
#include "merge/output_control.h"

static mtx_mp_rational_t s_probe_range_percentage{3, 10}; // 0.3%

// ----------------------------------------------------------------------

template<typename T>
void
add_all_requested_track_ids(generic_reader_c &reader,
                            T const &container) {
  for (auto const &pair : container)
    reader.add_requested_track_id(pair.first);
}

template<typename T>
void
add_all_requested_track_ids(generic_reader_c &reader,
                            std::vector<T> const &container) {
  for (auto const &key : container)
    reader.add_requested_track_id(key);
}

void
generic_reader_c::set_timestamp_restrictions(timestamp_c const &min,
                                             timestamp_c const &max) {
  m_restricted_timestamps_min = min;
  m_restricted_timestamps_max = max;
}

timestamp_c const &
generic_reader_c::get_timestamp_restriction_min()
  const {
  return m_restricted_timestamps_min;
}

timestamp_c const &
generic_reader_c::get_timestamp_restriction_max()
  const {
  return m_restricted_timestamps_max;
}

void
generic_reader_c::read_all() {
  for (auto &packetizer : m_reader_packetizers)
    while (read(packetizer.get(), true) != FILE_STATUS_DONE)
      ;
}

file_status_e
generic_reader_c::read_next(generic_packetizer_c *packetizer,
                            bool force) {
  auto prior_progrss = get_progress();
  auto result        = read(packetizer, force);
  auto new_progress  = get_progress();

  add_to_progress(new_progress - prior_progrss);

  return result;
}

bool
generic_reader_c::demuxing_requested(char type,
                                     int64_t id,
                                     mtx::bcp47::language_c const &language)
  const {
  static debugging_option_c s_debug{"demuxing_requested"};

  auto const &tracks = 'v' == type ? m_ti.m_vtracks
                     : 'a' == type ? m_ti.m_atracks
                     : 's' == type ? m_ti.m_stracks
                     : 'b' == type ? m_ti.m_btracks
                     :               m_ti.m_track_tags;

  auto result = tracks.selected(id, language.is_valid() ? language : mtx::bcp47::language_c::parse("und"));

  mxdebug_if(s_debug, fmt::format("demuxing_requested? {4} type {0} id {1} language {2} item_selector {3}\n", type, id, language, tracks, result ? "yes" : "no"));

  return result;
}

attach_mode_e
generic_reader_c::attachment_requested(int64_t id) {
  if (m_ti.m_attach_mode_list.none())
    return ATTACH_MODE_SKIP;

  if (m_ti.m_attach_mode_list.empty())
    return ATTACH_MODE_TO_ALL_FILES;

  if (m_ti.m_attach_mode_list.selected(id))
    return m_ti.m_attach_mode_list.reversed() ? ATTACH_MODE_TO_ALL_FILES : m_ti.m_attach_mode_list.get(id);

  if (!m_ti.m_attach_mode_list.reversed() && m_ti.m_attach_mode_list.selected(-1))
    return m_ti.m_attach_mode_list.get(-1);

  return ATTACH_MODE_SKIP;
}

int
generic_reader_c::add_packetizer(generic_packetizer_c *packetizer) {
  if (outputting_webm() && !packetizer->is_compatible_with(OC_WEBM))
    mxerror(fmt::format(FY("The codec type '{0}' cannot be used in a WebM compliant file.\n"), packetizer->get_format_name()));

  m_reader_packetizers.emplace_back(packetizer);
  m_used_track_ids.push_back(packetizer->m_ti.m_id);
  if (!m_appending)
    add_packetizer_globally(packetizer);

  return m_reader_packetizers.size() - 1;
}

size_t
generic_reader_c::get_num_packetizers()
  const
{
  return m_reader_packetizers.size();
}

generic_packetizer_c *
generic_reader_c::find_packetizer_by_id(int64_t id)
  const {
  auto itr = std::find_if(m_reader_packetizers.begin(), m_reader_packetizers.end(), [id](auto p) { return p->m_ti.m_id == id; });

  return itr != m_reader_packetizers.end() ? (*itr).get() : nullptr;
}

void
generic_reader_c::set_timestamp_offset(int64_t offset) {
  m_max_timestamp_seen = offset;

  for (auto ptzr : m_reader_packetizers)
    ptzr->m_correction_timestamp_offset = offset;
}

void
generic_reader_c::set_headers() {
  for (auto ptzr : m_reader_packetizers)
    ptzr->set_headers();
}

void
generic_reader_c::set_headers_for_track(int64_t tid) {
  for (auto ptzr : m_reader_packetizers)
    if (ptzr->m_ti.m_id == tid) {
      ptzr->set_headers();
      break;
    }
}

void
generic_reader_c::check_track_ids_and_packetizers() {
  add_available_track_ids();

  auto const available_ids = std::unordered_set<int64_t>{m_available_track_ids.begin(), m_available_track_ids.end()};
  auto const not_found     = available_ids.end();

  for (auto requested_id : m_requested_track_ids)
    if (available_ids.find(requested_id) == not_found)
      mxwarn_fn(m_ti.m_fname, fmt::format(FY("A track with the ID {0} was requested but not found in the file. The corresponding option will be ignored.\n"), requested_id));
}

void
generic_reader_c::add_requested_track_id(int64_t id) {
  if (-1 != id)
    m_requested_track_ids.insert(id);
}

int64_t
generic_reader_c::get_queued_bytes()
  const {
  int64_t bytes = 0;

  for (auto ptzr : m_reader_packetizers)
    bytes += ptzr->get_queued_bytes();

  return bytes;
}

file_status_e
generic_reader_c::flush_packetizer(int num) {
  return flush_packetizer(&ptzr(num));
}

file_status_e
generic_reader_c::flush_packetizer(generic_packetizer_c *packetizer) {
  packetizer->flush();

  return FILE_STATUS_DONE;
}

file_status_e
generic_reader_c::flush_packetizers() {
  for (auto ptzr : m_reader_packetizers)
    ptzr->flush();

  return FILE_STATUS_DONE;
}

translatable_string_c
generic_reader_c::get_format_name()
  const {
  return mtx::file_type_t::get_name(get_format_type());
}

void
generic_reader_c::id_result_container(mtx::id::verbose_info_t const &verbose_info) {
  auto type                           = get_format_type();
  m_id_results_container.info         = mtx::file_type_t::get_name(type).get_translated();
  m_id_results_container.verbose_info = verbose_info;
  m_id_results_container.verbose_info.emplace_back("container_type",          static_cast<int>(type));
  m_id_results_container.verbose_info.emplace_back("is_providing_timestamps", is_providing_timestamps());
}

void
generic_reader_c::id_result_track(int64_t track_id,
                                  std::string const &type,
                                  std::string const &info,
                                  mtx::id::verbose_info_t const &verbose_info) {
  id_result_t result(track_id, type, info, {}, 0);
  result.verbose_info = verbose_info;
  m_id_results_tracks.push_back(result);
}

void
generic_reader_c::id_result_attachment(int64_t attachment_id,
                                       std::string const &type,
                                       int size,
                                       std::string const &file_name,
                                       std::string const &description,
                                       std::optional<uint64_t> id) {
  id_result_t result(attachment_id, type, file_name, description, size);
  if (id)
    result.verbose_info.emplace_back("uid", *id);
  m_id_results_attachments.push_back(result);
}

void
generic_reader_c::id_result_chapters(int num_entries) {
  id_result_t result(0, ID_RESULT_CHAPTERS, {}, {}, num_entries);
  m_id_results_chapters.push_back(result);
}

void
generic_reader_c::id_result_tags(int64_t track_id,
                                 int num_entries) {
  id_result_t result(track_id, ID_RESULT_TAGS, {}, {}, num_entries);
  m_id_results_tags.push_back(result);
}

void
generic_reader_c::display_identification_results() {
  if (identification_output_format_e::json == g_identification_output_format)
    display_identification_results_as_json();
  else
    display_identification_results_as_text();
}

void
generic_reader_c::display_identification_results_as_text() {
  mxinfo(fmt::format(FY("File '{0}': container: {1}"), m_ti.m_fname, m_id_results_container.info));
  mxinfo("\n");

  for (auto &result : m_id_results_tracks) {
    mxinfo(fmt::format(FY("Track ID {0}: {1} ({2})"), result.id, result.type, result.info));
    mxinfo("\n");
  }

  for (auto &result : m_id_results_attachments) {
    mxinfo(fmt::format(FY("Attachment ID {0}: type '{1}', size {2} bytes"), result.id, result.type, result.size));

    if (!result.description.empty())
      mxinfo(fmt::format(FY(", description '{0}'"), result.description));

    if (!result.info.empty())
      mxinfo(fmt::format(FY(", file name '{0}'"), result.info));

    mxinfo("\n");
  }

  for (auto &result : m_id_results_chapters) {
    mxinfo(fmt::format(FNY("Chapters: {0} entry", "Chapters: {0} entries", result.size), result.size));
    mxinfo("\n");
  }

  for (auto &result : m_id_results_tags) {
    if (ID_RESULT_GLOBAL_TAGS_ID == result.id)
      mxinfo(fmt::format(FNY("Global tags: {0} entry", "Global tags: {0} entries", result.size), result.size));

    else
      mxinfo(fmt::format(FNY("Tags for track ID {0}: {1} entry", "Tags for track ID {0}: {1} entries", result.size), result.id, result.size));

    mxinfo("\n");
  }
}

void
generic_reader_c::display_identification_results_as_json() {
  auto verbose_info_to_object = [](mtx::id::verbose_info_t const &verbose_info) -> nlohmann::json {
    auto object = nlohmann::json{};
    for (auto const &property : verbose_info)
      object[property.first] = property.second;

    return object.is_null() ? nlohmann::json::object() : object;
  };

  auto json = nlohmann::json{
    { "identification_format_version", ID_JSON_FORMAT_VERSION  },
    { "file_name",                     m_ti.m_fname            },
    { "tracks",                        nlohmann::json::array() },
    { "attachments",                   nlohmann::json::array() },
    { "chapters",                      nlohmann::json::array() },
    { "global_tags",                   nlohmann::json::array() },
    { "track_tags",                    nlohmann::json::array() },
    { "container", {
        { "recognized", true                                                        },
        { "supported",  true                                                        },
        { "type",       m_id_results_container.info                                 },
        { "properties", verbose_info_to_object(m_id_results_container.verbose_info) },
      } },
  };

  for (auto const &result : m_id_results_tracks)
    json["tracks"] += nlohmann::json{
      { "id",         result.id                                   },
      { "type",       result.type                                 },
      { "codec",      result.info                                 },
      { "properties", verbose_info_to_object(result.verbose_info) },
    };

  for (auto const &result : m_id_results_attachments)
    json["attachments"] += nlohmann::json{
      { "id",           result.id                                   },
      { "content_type", result.type                                 },
      { "size",         result.size                                 },
      { "description",  result.description                          },
      { "file_name",    result.info                                 },
      { "properties",   verbose_info_to_object(result.verbose_info) },
    };

  for (auto const &result : m_id_results_chapters)
    json["chapters"] += nlohmann::json{
      { "num_entries", result.size },
    };

  for (auto const &result : m_id_results_tags) {
    if (ID_RESULT_GLOBAL_TAGS_ID == result.id)
      json["global_tags"] += nlohmann::json{
        { "num_entries", result.size },
      };
    else
      json["track_tags"] += nlohmann::json{
        { "track_id",    result.id   },
        { "num_entries", result.size },
      };
  }

  display_json_output(json);
}

void
generic_reader_c::add_available_track_id(int64_t id) {
  m_available_track_ids.push_back(id);
}

void
generic_reader_c::add_available_track_ids() {
  add_available_track_id(0);
}

void
generic_reader_c::add_available_track_id_range(int64_t start,
                                               int64_t end) {
  for (int64_t id = start; id <= end; ++id)
    add_available_track_id(id);
}

int64_t
generic_reader_c::get_progress() {
  return m_in->getFilePointer();
}

int64_t
generic_reader_c::get_maximum_progress() {
  return m_in->get_size();
}

mm_io_c *
generic_reader_c::get_underlying_input(mm_io_c *actual_in)
  const {
  if (!actual_in)
    actual_in = m_in.get();

  while (dynamic_cast<mm_proxy_io_c *>(actual_in))
    actual_in = static_cast<mm_proxy_io_c *>(actual_in)->get_proxied();
  return actual_in;
}

void
generic_reader_c::set_probe_range_percentage(mtx_mp_rational_t const &probe_range_percentage) {
  s_probe_range_percentage = probe_range_percentage;
}

int64_t
generic_reader_c::calculate_probe_range(int64_t file_size,
                                        int64_t fixed_minimum)
  const {
  static debugging_option_c s_debug{"probe_range"};

  auto factor      = mtx_mp_rational_t{1, 100} * s_probe_range_percentage;
  auto probe_range = mtx::to_int(factor * file_size);
  auto to_use      = std::max(fixed_minimum, probe_range);

  mxdebug_if(s_debug,
             fmt::format("calculate_probe_range: calculated {0} based on file size {1} fixed minimum {2} percentage {3}/{4} percentage of size {5}\n",
                         to_use, file_size, fixed_minimum, boost::multiprecision::numerator(s_probe_range_percentage), boost::multiprecision::denominator(s_probe_range_percentage), probe_range));

  return to_use;
}

void
generic_reader_c::set_file_to_read(mm_io_cptr const &in) {
  m_in   = in;
  m_size = in->get_size();
}

void
generic_reader_c::set_probe_range_info(probe_range_info_t const &info) {
  m_probe_range_info = info;
}

void
generic_reader_c::set_track_info(track_info_c const &info) {
  m_ti = info;

  add_all_requested_track_ids(*this, m_ti.m_atracks.item_ids());
  add_all_requested_track_ids(*this, m_ti.m_vtracks.item_ids());
  add_all_requested_track_ids(*this, m_ti.m_stracks.item_ids());
  add_all_requested_track_ids(*this, m_ti.m_btracks.item_ids());
  add_all_requested_track_ids(*this, m_ti.m_track_tags.item_ids());
  add_all_requested_track_ids(*this, m_ti.m_all_fourccs);
  add_all_requested_track_ids(*this, m_ti.m_display_properties);
  add_all_requested_track_ids(*this, m_ti.m_timestamp_syncs);
  add_all_requested_track_ids(*this, m_ti.m_cue_creations);
  add_all_requested_track_ids(*this, m_ti.m_default_track_flags);
  add_all_requested_track_ids(*this, m_ti.m_fix_bitstream_frame_rate_flags);
  add_all_requested_track_ids(*this, m_ti.m_languages);
  add_all_requested_track_ids(*this, m_ti.m_sub_charsets);
  add_all_requested_track_ids(*this, m_ti.m_all_tags);
  add_all_requested_track_ids(*this, m_ti.m_all_aac_is_sbr);
  add_all_requested_track_ids(*this, m_ti.m_compression_list);
  add_all_requested_track_ids(*this, m_ti.m_track_names);
  add_all_requested_track_ids(*this, m_ti.m_all_ext_timestamps);
  add_all_requested_track_ids(*this, m_ti.m_pixel_crop_list);
  add_all_requested_track_ids(*this, m_ti.m_reduce_to_core);
}

void
generic_reader_c::add_track_tags_to_identification(libmatroska::KaxTags const &tags,
                                                   mtx::id::info_c &info) {
  for (auto const &tag_elt : tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(tag_elt);
    if (!tag)
      continue;

    for (auto const &simple_tag_elt : *tag) {
      auto simple_tag = dynamic_cast<libmatroska::KaxTagSimple *>(simple_tag_elt);
      if (!simple_tag)
        continue;
      auto name  = mtx::tags::get_simple_name(*simple_tag);
      auto value = mtx::tags::get_simple_value(*simple_tag);

      if (!name.empty()) {
        info.add(fmt::format("tag_{0}", balg::to_lower_copy(name)), value);
      }
    }
  }
}

void
generic_reader_c::show_demuxer_info() {
  if (verbose)
    mxinfo_fn(m_ti.m_fname, fmt::format(FY("Using the demultiplexer for the format '{0}'.\n"), get_format_name()));
}

void
generic_reader_c::show_packetizer_info(int64_t track_id,
                                       generic_packetizer_c const &packetizer) {
  mxinfo_tid(m_ti.m_fname, track_id, fmt::format(FY("Using the output module for the format '{0}'.\n"), packetizer.get_format_name()));
}

generic_packetizer_c &
generic_reader_c::ptzr(int64_t track_idx) {
  return *m_reader_packetizers[track_idx];
}
