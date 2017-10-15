/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   the generic_reader_c implementation

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/list_utils.h"
#include "common/strings/formatting.h"
#include "merge/generic_packetizer.h"
#include "merge/generic_reader.h"
#include "merge/input_x.h"
#include "merge/output_control.h"

static int64_rational_c s_probe_range_percentage{3, 10}; // 0.3%

static std::string
format_json_value(nlohmann::json const &value) {
  return value.is_number()  ? to_string(value.get<uint64_t>())
       : value.is_boolean() ? std::string{value.get<bool>() ? "1" : "0"}
       :                      value.get<std::string>();
}

static std::string
format_key_and_json_value(std::string const &key,
                          nlohmann::json const &value) {
  return (boost::format{"%1%:%2%"} % escape(key) % escape(format_json_value(value))).str();
}

static std::string
format_verbose_info(mtx::id::verbose_info_t const &info) {
  auto formatted = std::vector<std::string>{};
  auto sub_fmt   = boost::format("%1%.%2%.%3%");

  for (auto const &pair : info) {
    if (pair.second.is_array()) {
      auto idx = 0u;

      for (auto it = pair.second.begin(), end = pair.second.end(); it != end; ++it) {
        if (it->is_object()) {
          for (auto sub_it = it->begin(), sub_end = it->end(); sub_it != sub_end; ++sub_it)
            formatted.emplace_back(format_key_and_json_value((sub_fmt % pair.first % idx % sub_it.key()).str(), sub_it.value()));

          ++idx;

        } else
          formatted.emplace_back(format_key_and_json_value(pair.first, *it));
      }

    } else
      formatted.emplace_back(format_key_and_json_value(pair.first, pair.second));
  }

  brng::sort(formatted);

  return boost::join(formatted, " ");
}

// ----------------------------------------------------------------------

template<typename T>
void
add_all_requested_track_ids(generic_reader_c &reader,
                            T const &container) {
  for (auto const &pair : container)
    reader.add_requested_track_id(pair.first);
}

generic_reader_c::generic_reader_c(const track_info_c &ti,
                                   const mm_io_cptr &in)
  : m_ti{ti}
  , m_in{in}
  , m_size{static_cast<uint64_t>(in->get_size())}
  , m_ptzr_first_packet{}
  , m_max_timestamp_seen{}
  , m_appending{}
  , m_num_video_tracks{}
  , m_num_audio_tracks{}
  , m_num_subtitle_tracks{}
  , m_reference_timestamp_tolerance{}
{
  add_all_requested_track_ids(*this, m_ti.m_atracks.m_items);
  add_all_requested_track_ids(*this, m_ti.m_vtracks.m_items);
  add_all_requested_track_ids(*this, m_ti.m_stracks.m_items);
  add_all_requested_track_ids(*this, m_ti.m_btracks.m_items);
  add_all_requested_track_ids(*this, m_ti.m_track_tags.m_items);
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

generic_reader_c::~generic_reader_c() {
  size_t i;

  for (i = 0; i < m_reader_packetizers.size(); i++)
    delete m_reader_packetizers[i];
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
    while (read(packetizer, true) != FILE_STATUS_DONE)
      ;
}

bool
generic_reader_c::demuxing_requested(char type,
                                     int64_t id,
                                     boost::optional<std::string> const &language)
  const {
  auto const *tracks = 'v' == type ? &m_ti.m_vtracks
                     : 'a' == type ? &m_ti.m_atracks
                     : 's' == type ? &m_ti.m_stracks
                     : 'b' == type ? &m_ti.m_btracks
                     : 'T' == type ? &m_ti.m_track_tags
                     :               nullptr;

  if (!tracks)
    mxerror(boost::format("generic_reader_c::demuxing_requested: %2%") % (boost::format(Y("Invalid track type %1%.")) % type));

  return tracks->selected(id, language);
}

attach_mode_e
generic_reader_c::attachment_requested(int64_t id) {
  if (m_ti.m_attach_mode_list.none())
    return ATTACH_MODE_SKIP;

  if (m_ti.m_attach_mode_list.empty())
    return ATTACH_MODE_TO_ALL_FILES;

  if (m_ti.m_attach_mode_list.selected(id))
    return m_ti.m_attach_mode_list.get(id);

  if (m_ti.m_attach_mode_list.selected(-1))
    return m_ti.m_attach_mode_list.get(-1);

  return ATTACH_MODE_SKIP;
}

int
generic_reader_c::add_packetizer(generic_packetizer_c *ptzr) {
  if (outputting_webm() && !ptzr->is_compatible_with(OC_WEBM))
    mxerror(boost::format(Y("The codec type '%1%' cannot be used in a WebM compliant file.\n")) % ptzr->get_format_name());

  m_reader_packetizers.push_back(ptzr);
  m_used_track_ids.push_back(ptzr->m_ti.m_id);
  if (!m_appending)
    add_packetizer_globally(ptzr);

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
  auto itr = brng::find_if(m_reader_packetizers, [id](generic_packetizer_c *p) { return p->m_ti.m_id == id; });

  return itr != m_reader_packetizers.end() ? *itr : nullptr;
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

  size_t r;
  for (r = 0; m_requested_track_ids.size() > r; ++r) {
    bool found = false;
    size_t a;
    for (a = 0; m_available_track_ids.size() > a; ++a)
      if (m_requested_track_ids[r] == m_available_track_ids[a]) {
        found = true;
        break;
      }

    if (!found)
      mxwarn_fn(m_ti.m_fname,
                boost::format(Y("A track with the ID %1% was requested but not found in the file. The corresponding option will be ignored.\n"))
                % m_requested_track_ids[r]);
  }
}

void
generic_reader_c::add_requested_track_id(int64_t id) {
  if (-1 == id)
    return;

  bool found = false;
  size_t i;
  for (i = 0; i < m_requested_track_ids.size(); i++)
    if (m_requested_track_ids[i] == id) {
      found = true;
      break;
    }

  if (!found)
    m_requested_track_ids.push_back(id);
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
  return flush_packetizer(PTZR(num));
}

file_status_e
generic_reader_c::flush_packetizer(generic_packetizer_c *ptzr) {
  ptzr->flush();

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
  id_result_t result(track_id, type, info, empty_string, 0);
  result.verbose_info = verbose_info;
  m_id_results_tracks.push_back(result);
}

void
generic_reader_c::id_result_attachment(int64_t attachment_id,
                                       const std::string &type,
                                       int size,
                                       const std::string &file_name,
                                       const std::string &description,
                                       boost::optional<uint64_t> id) {
  id_result_t result(attachment_id, type, file_name, description, size);
  if (id)
    result.verbose_info.emplace_back("uid", *id);
  m_id_results_attachments.push_back(result);
}

void
generic_reader_c::id_result_chapters(int num_entries) {
  id_result_t result(0, ID_RESULT_CHAPTERS, empty_string, empty_string, num_entries);
  m_id_results_chapters.push_back(result);
}

void
generic_reader_c::id_result_tags(int64_t track_id,
                                 int num_entries) {
  id_result_t result(track_id, ID_RESULT_TAGS, empty_string, empty_string, num_entries);
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
  auto identify_verbose    = mtx::included_in(g_identification_output_format, identification_output_format_e::verbose_text, identification_output_format_e::gui);
  auto identify_for_gui    = identification_output_format_e::gui == g_identification_output_format;

  std::string format_file, format_track, format_attachment, format_att_description, format_att_file_name;

  if (identify_for_gui) {
    format_file            =   "File '%1%': container: %2%";
    format_track           =   "Track ID %1%: %2% (%3%)";
    format_attachment      =   "Attachment ID %1%: type \"%2%\", size %3% bytes";
    format_att_description =   ", description \"%1%\"";
    format_att_file_name   =   ", file name \"%1%\"";

  } else {
    format_file            = Y("File '%1%': container: %2%");
    format_track           = Y("Track ID %1%: %2% (%3%)");
    format_attachment      = Y("Attachment ID %1%: type '%2%', size %3% bytes");
    format_att_description = Y(", description '%1%'");
    format_att_file_name   = Y(", file name '%1%'");
  }

  mxinfo(boost::format(format_file) % m_ti.m_fname % m_id_results_container.info);

  if (identify_verbose && !m_id_results_container.verbose_info.empty())
    mxinfo(boost::format(" [%1%]") % format_verbose_info(m_id_results_container.verbose_info));

  mxinfo("\n");

  for (auto &result : m_id_results_tracks) {
    mxinfo(boost::format(format_track) % result.id % result.type % result.info);

    if (identify_verbose && !result.verbose_info.empty())
      mxinfo(boost::format(" [%1%]") % format_verbose_info(result.verbose_info));

    mxinfo("\n");
  }

  for (auto &result : m_id_results_attachments) {
    mxinfo(boost::format(format_attachment) % result.id % id_escape_string(result.type) % result.size);

    if (!result.description.empty())
      mxinfo(boost::format(format_att_description) % id_escape_string(result.description));

    if (!result.info.empty())
      mxinfo(boost::format(format_att_file_name) % id_escape_string(result.info));

    if (identify_verbose && !result.verbose_info.empty())
      mxinfo(boost::format(" [%1%]") % format_verbose_info(result.verbose_info));

    mxinfo("\n");
  }

  for (auto &result : m_id_results_chapters) {
    if (identify_for_gui)
      mxinfo(boost::format("Chapters: %1% entries") % result.size);
    else
      mxinfo(boost::format(NY("Chapters: %1% entry", "Chapters: %1% entries", result.size)) % result.size);
    mxinfo("\n");
  }

  for (auto &result : m_id_results_tags) {
    if (ID_RESULT_GLOBAL_TAGS_ID == result.id) {
      if (identify_for_gui)
        mxinfo(boost::format("Global tags: %1% entries") % result.size);
      else
        mxinfo(boost::format(NY("Global tags: %1% entry", "Global tags: %1% entries", result.size)) % result.size);

    } else if (identify_for_gui)
      mxinfo(boost::format("Tags for track ID %1%: %2% entries") % result.id % result.size);
    else
      mxinfo(boost::format(NY("Tags for track ID %1%: %2% entry", "Tags for track ID %1%: %2% entries", result.size)) % result.id % result.size);

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

std::string
generic_reader_c::id_escape_string(const std::string &s) {
  return identification_output_format_e::gui == g_identification_output_format ? escape(s) : s;
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

int
generic_reader_c::get_progress() {
  return 100 * m_in->getFilePointer() / m_size;
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
generic_reader_c::set_probe_range_percentage(int64_rational_c const &probe_range_percentage) {
  s_probe_range_percentage = probe_range_percentage;
}

int64_t
generic_reader_c::calculate_probe_range(int64_t file_size,
                                        int64_t fixed_minimum)
  const {
  static debugging_option_c s_debug{"probe_range"};

  auto factor      = int64_rational_c{1, 100} * s_probe_range_percentage;
  auto probe_range = boost::rational_cast<int64_t>(factor * file_size);
  auto to_use      = std::max(fixed_minimum, probe_range);

  mxdebug_if(s_debug,
             boost::format("calculate_probe_range: calculated %1% based on file size %2% fixed minimum %3% percentage %4%/%5% percentage of size %6%\n")
             % to_use % file_size % fixed_minimum % s_probe_range_percentage.numerator() % s_probe_range_percentage.denominator() % probe_range);

  return to_use;
}
