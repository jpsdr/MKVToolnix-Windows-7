/** \brief output handling

   mkvmerge -- utility for splicing together Matroska files
   from component media sub-types

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <typeinfo>

#include "common/mm_file_io.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/mm_text_io.h"
#include "common/path.h"
#include "common/strings/formatting.h"
#include "common/xml/xml.h"
#include "input/r_aac.h"
#include "input/r_ac3.h"
#include "input/r_avc.h"
#include "input/r_avi.h"
#include "input/r_coreaudio.h"
#include "input/r_dirac.h"
#include "input/r_dts.h"
#include "input/r_dv.h"
#include "input/r_flac.h"
#include "input/r_flv.h"
#include "input/r_hdmv_pgs.h"
#include "input/r_hdmv_textst.h"
#include "input/r_hevc.h"
#include "input/r_ivf.h"
#include "input/r_matroska.h"
#include "input/r_microdvd.h"
#include "input/r_mp3.h"
#include "input/r_mpeg_es.h"
#include "input/r_mpeg_ps.h"
#include "input/r_mpeg_ts.h"
#include "input/r_obu.h"
#include "input/r_ogm.h"
#include "input/r_qtmp4.h"
#include "input/r_real.h"
#include "input/r_srt.h"
#include "input/r_ssa.h"
#include "input/r_truehd.h"
#include "input/r_tta.h"
#include "input/r_usf.h"
#include "input/r_vc1.h"
#include "input/r_vobbtn.h"
#include "input/r_vobsub.h"
#include "input/r_wav.h"
#include "input/r_wavpack.h"
#include "input/r_webvtt.h"
#include "input/unsupported_types_signature_prober.h"
#include "merge/filelist.h"
#include "merge/input_x.h"
#include "merge/probe_range_info.h"
#include "merge/reader_detection_and_creation.h"

static std::vector<boost::filesystem::path>
file_names_to_paths(const std::vector<std::string> &file_names) {
  std::vector<boost::filesystem::path> paths;
  for (auto &file_name : file_names)
    paths.push_back(boost::filesystem::absolute(mtx::fs::to_path(file_name)));

  return paths;
}

static mm_io_cptr
open_input_file(filelist_t &file) {
  try {
    if (file.all_names.size() == 1)
      return std::make_shared<mm_read_buffer_io_c>(std::make_shared<mm_file_io_c>(file.name));

    else {
      auto paths = file_names_to_paths(file.all_names);
      return std::make_shared<mm_read_buffer_io_c>(std::make_shared<mm_multi_file_io_c>(paths, file.name));
    }

  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for reading: {1}.\n"), file.name, ex));
    return mm_io_cptr{};

  } catch (...) {
    mxerror(fmt::format(FY("The source file '{0}' could not be opened successfully, or retrieving its size by seeking to the end did not work.\n"), file.name));
    return mm_io_cptr{};
  }
}

static bool
open_playlist_file(filelist_t &file,
                   mm_io_c &in) {
  auto mpls_in = mm_mpls_multi_file_io_c::open_multi(in);
  if (!mpls_in)
    return false;

  file.is_playlist      = true;
  file.playlist_mpls_in = std::static_pointer_cast<mm_mpls_multi_file_io_c>(mpls_in);

  return true;
}

static debugging_option_c s_debug_probe{"probe_file_format"};

template<typename Treader>
std::unique_ptr<Treader>
create_and_prepare_reader(mm_io_cptr const &io,
                          probe_range_info_t const &probe_range_info = {}) {
  auto reader = std::make_unique<Treader>();

  io->setFilePointer(0);
  reader->set_file_to_read(io);
  reader->set_probe_range_info(probe_range_info);

  return reader;
}

template<typename Treader>
typename std::enable_if<
  std::is_base_of<generic_reader_c, Treader>::value,
  std::unique_ptr<generic_reader_c>
>::type
do_probe(mm_io_cptr const &io,
         probe_range_info_t const &probe_range_info = {}) {
  auto reader    = create_and_prepare_reader<Treader>(io, probe_range_info);
  auto probed_ok = false;

  try {
    probed_ok = reader->probe_file();
  } catch (mtx::exception &ex) {
    mxdebug_if(s_debug_probe, fmt::format("do_probe<{}>: mtx::exception caught: {}\n", typeid(Treader).name(), ex.what()));
  } catch (...) {
    mxdebug_if(s_debug_probe, fmt::format("do_probe<{}>: generic exception caught\n", typeid(Treader).name()));
  }

  mxdebug_if(s_debug_probe, fmt::format("do_probe<{}>: probe result: {}\n", typeid(Treader).name(), probed_ok));

  io->setFilePointer(0);

  if (probed_ok)
    return reader;

  return {};
}

template<typename Treader>
typename std::enable_if<
  !std::is_base_of<generic_reader_c, Treader>::value,
  std::unique_ptr<generic_reader_c>
>::type
do_probe(mm_io_cptr const &io,
         probe_range_info_t const & = {}) {
  io->setFilePointer(0);
  Treader::probe_file(*io);
  io->setFilePointer(0);

  return {};
}

using prober_t = std::function<std::unique_ptr<generic_reader_c>(mm_io_cptr const &, probe_range_info_t const &)>;

static prober_t
prober_for_type(mtx::file_type_e type) {
  static std::map<mtx::file_type_e, prober_t> type_probe_map;

  if (type_probe_map.empty()) {
    type_probe_map[mtx::file_type_e::avc_es]      = &do_probe<avc_es_reader_c>;
    type_probe_map[mtx::file_type_e::avi]         = &do_probe<avi_reader_c>;
    type_probe_map[mtx::file_type_e::coreaudio]   = &do_probe<coreaudio_reader_c>;
    type_probe_map[mtx::file_type_e::dirac]       = &do_probe<dirac_es_reader_c>;
    type_probe_map[mtx::file_type_e::dts]         = &do_probe<dts_reader_c>;
    type_probe_map[mtx::file_type_e::dv]          = &do_probe<dv_reader_c>;
    type_probe_map[mtx::file_type_e::flac]        = &do_probe<flac_reader_c>;
    type_probe_map[mtx::file_type_e::flv]         = &do_probe<flv_reader_c>;
    type_probe_map[mtx::file_type_e::hdmv_textst] = &do_probe<hdmv_textst_reader_c>;
    type_probe_map[mtx::file_type_e::hevc_es]     = &do_probe<hevc_es_reader_c>;
    type_probe_map[mtx::file_type_e::ivf]         = &do_probe<ivf_reader_c>;
    type_probe_map[mtx::file_type_e::matroska]    = &do_probe<kax_reader_c>;
    type_probe_map[mtx::file_type_e::mpeg_es]     = &do_probe<mpeg_es_reader_c>;
    type_probe_map[mtx::file_type_e::mpeg_ps]     = &do_probe<mpeg_ps_reader_c>;
    type_probe_map[mtx::file_type_e::mpeg_ts]     = &do_probe<mtx::mpeg_ts::reader_c>;
    type_probe_map[mtx::file_type_e::obu]         = &do_probe<obu_reader_c>;
    type_probe_map[mtx::file_type_e::ogm]         = &do_probe<ogm_reader_c>;
    type_probe_map[mtx::file_type_e::pgssup]      = &do_probe<hdmv_pgs_reader_c>;
    type_probe_map[mtx::file_type_e::qtmp4]       = &do_probe<qtmp4_reader_c>;
    type_probe_map[mtx::file_type_e::real]        = &do_probe<real_reader_c>;
    type_probe_map[mtx::file_type_e::truehd]      = &do_probe<truehd_reader_c>;
    type_probe_map[mtx::file_type_e::tta]         = &do_probe<tta_reader_c>;
    type_probe_map[mtx::file_type_e::vc1]         = &do_probe<vc1_es_reader_c>;
    type_probe_map[mtx::file_type_e::vobbtn]      = &do_probe<vobbtn_reader_c>;
    type_probe_map[mtx::file_type_e::wav]         = &do_probe<wav_reader_c>;
    type_probe_map[mtx::file_type_e::wavpack4]    = &do_probe<wavpack_reader_c>;
  }

  auto res = type_probe_map.find(type);
  if (res == type_probe_map.end()) {
    return {};
  }
  return (*res).second;
}

std::unique_ptr<generic_reader_c>
detect_text_file_formats(filelist_t const &file) {
  try {
    auto text_io = std::make_shared<mm_text_io_c>(std::make_shared<mm_read_buffer_io_c>(std::make_shared<mm_file_io_c>(file.name)));
    std::unique_ptr<generic_reader_c> reader;

    if ((reader = do_probe<webvtt_reader_c>(text_io)))
      return reader;
    if ((reader = do_probe<srt_reader_c>(text_io)))
      return reader;
    if ((reader = do_probe<ssa_reader_c>(text_io)))
      return reader;
    if ((reader = do_probe<vobsub_reader_c>(text_io)))
      return reader;
    if ((reader = do_probe<usf_reader_c>(text_io)))
      return reader;

    // Unsupported text subtitle formats
    do_probe<microdvd_reader_c>(text_io);

    // Support empty files for certain types.
    if ((text_io->get_size() - text_io->get_byte_order_length()) > 1)
      return {};

    auto extension = mtx::fs::to_path(file.name).extension().string().substr(1);

    for (auto type : mtx::file_type_t::by_extension(extension))
      if (type == mtx::file_type_e::srt)
        return create_and_prepare_reader<srt_reader_c>(text_io);

  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The file '{0}' could not be opened for reading: {1}.\n"), file.name, ex));

  } catch (...) {
    mxerror(fmt::format(FY("The source file '{0}' could not be opened successfully, or retrieving its size by seeking to the end did not work.\n"), file.name));
  }

  return {};
}

/** \brief Probe the file type

   Opens the input file and calls the \c probe_file function for each known
   file reader class. Uses \c mm_text_io_c for subtitle probing.
*/
std::unique_ptr<generic_reader_c>
probe_file_format(filelist_t &file) {
  auto io          = open_input_file(file);
  auto is_playlist = !file.is_playlist && open_playlist_file(file, *io);

  std::unique_ptr<generic_reader_c> reader;

  if (is_playlist)
    io = std::make_shared<mm_read_buffer_io_c>(file.playlist_mpls_in);

  // File types that can be detected unambiguously but are not
  // supported. The prober does not return if it detects the type.
  do_probe<unsupported_types_signature_prober_c>(io);

  // File types that can be detected unambiguously
  if ((reader = do_probe<avi_reader_c>(io)))
    return reader;
  if ((reader = do_probe<flv_reader_c>(io)))
    return reader;
  if ((reader = do_probe<kax_reader_c>(io)))
    return reader;
  if ((reader = do_probe<wav_reader_c>(io)))
    return reader;
  if ((reader = do_probe<ogm_reader_c>(io)))
    return reader;
  if ((reader = do_probe<hdmv_textst_reader_c>(io)))
    return reader;
  if ((reader = do_probe<flac_reader_c>(io)))
    return reader;
  if ((reader = do_probe<hdmv_pgs_reader_c>(io)))
    return reader;
  if ((reader = do_probe<real_reader_c>(io)))
    return reader;
  if ((reader = do_probe<qtmp4_reader_c>(io)))
    return reader;
  if ((reader = do_probe<tta_reader_c>(io)))
    return reader;
  if ((reader = do_probe<vc1_es_reader_c>(io)))
    return reader;
  if ((reader = do_probe<wavpack_reader_c>(io)))
    return reader;
  if ((reader = do_probe<ivf_reader_c>(io)))
    return reader;
  if ((reader = do_probe<coreaudio_reader_c>(io)))
    return reader;
  if ((reader = do_probe<dirac_es_reader_c>(io)))
    return reader;

  // Prefer types hinted by extension
  auto extension = mtx::fs::to_path(file.name).extension().string();
  if (!extension.empty()) {
    for (auto type : mtx::file_type_t::by_extension(extension.substr(1))) {
      auto p = prober_for_type(type);
      if (p && (reader = p(io, {})))
        return reader;
    }
  }

  // All text file types (subtitles).
  if ((reader = detect_text_file_formats(file)))
    return reader;

  // AVC & HEVC, even though often mis-detected, have a very high
  // probability of correct detection with headers right at the start.
  if ((reader = do_probe<avc_es_reader_c>(io, { 0, 0, true })))
    return reader;
  if ((reader = do_probe<hevc_es_reader_c>(io, { 0, 0, true })))
    return reader;

  // Try raw audio formats and require eight consecutive frames at the
  // start of the file.
  if ((reader = do_probe<mp3_reader_c>(io, { 128 * 1024, 8, true })))
    return reader;
  if ((reader = do_probe<ac3_reader_c>(io, { 128 * 1024, 8, true })))
    return reader;
  if ((reader = do_probe<aac_reader_c>(io, { 128 * 1024, 8, true })))
    return reader;

  // File types that are mis-detected sometimes
  if ((reader = do_probe<dts_reader_c>(io, { 0, 0, true })))
    return reader;
  if ((reader = do_probe<mtx::mpeg_ts::reader_c>(io)))
    return reader;
  if ((reader = do_probe<mpeg_ps_reader_c>(io)))
    return reader;
  if ((reader = do_probe<obu_reader_c>(io)))
    return reader;

  // File types which are the same in raw format and in other container formats.
  // Detection requires 20 or more consecutive packets.
  static std::vector<int> s_probe_sizes1{ { 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 } };
  static int const s_probe_num_required_consecutive_packets1 = 64;

  for (auto probe_size : s_probe_sizes1) {
    if ((reader = do_probe<mp3_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets1 })))
      return reader;
    if ((reader = do_probe<ac3_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets1 })))
      return reader;
    if ((reader = do_probe<aac_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets1 })))
      return reader;
  }

  // More file types with detection issues.
  if ((reader = do_probe<truehd_reader_c>(io, { 512 * 1024, 0 })))
    return reader;
  if ((reader = do_probe<dts_reader_c>(io)))
    return reader;
  if ((reader = do_probe<vobbtn_reader_c>(io)))
    return reader;

  // Try some more of the raw audio formats before trying elementary
  // stream video formats (MPEG 1/2, AVC/H.264, HEVC/H.265; those
  // often enough simply work). However, require that the first frame
  // starts at the beginning of the file.
  if ((reader = do_probe<mp3_reader_c>(io, { 32 * 1024, 1, true })))
    return reader;
  if ((reader = do_probe<ac3_reader_c>(io, { 32 * 1024, 1, true })))
    return reader;
  if ((reader = do_probe<aac_reader_c>(io, { 32 * 1024, 1, true })))
    return reader;

  if ((reader = do_probe<mpeg_es_reader_c>(io)))
    return reader;
  if ((reader = do_probe<avc_es_reader_c>(io, { 0, 0, false })))
    return reader;
  if ((reader = do_probe<hevc_es_reader_c>(io, { 0, 0, false })))
    return reader;

  // File types which are the same in raw format and in other container formats.
  // Detection requires 20 or more consecutive packets.
  static std::vector<int> s_probe_sizes2{ { 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 } };
  static int const s_probe_num_required_consecutive_packets2 = 20;

  for (auto probe_size : s_probe_sizes2) {
    if ((reader = do_probe<mp3_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets2 })))
      return reader;
    else if ((reader = do_probe<ac3_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets2 })))
      return reader;
    else if ((reader = do_probe<aac_reader_c>(io, { probe_size, s_probe_num_required_consecutive_packets2 })))
      return reader;
  }

  // File types that are mis-detected sometimes and that aren't supported
  do_probe<dv_reader_c>(io);

  return {};
}

void
read_file_headers() {
  static auto s_debug_timestamp_restrictions = debugging_option_c{"timestamp_restrictions"};

  g_file_sizes = 0;

  for (auto &file : g_files) {
    try {
      file->reader->m_appending = file->appending;
      file->reader->set_track_info(*file->ti);
      file->reader->set_timestamp_restrictions(file->restricted_timestamp_min, file->restricted_timestamp_max);
      file->reader->read_headers();

      // Re-calculate file size because the reader might switch to a
      // multi I/O reader in read_headers().
      file->size    = file->reader->get_file_size();
      g_file_sizes += file->size;

      mxdebug_if(s_debug_timestamp_restrictions,
                 fmt::format("Timestamp restrictions for {2}: min {0} max {1}\n", file->restricted_timestamp_min, file->restricted_timestamp_max, file->ti->m_fname));

    } catch (mtx::mm_io::open_x &error) {
      mxerror(fmt::format(FY("The demultiplexer for the file '{0}' failed to initialize:\n{1}\n"), file->ti->m_fname, Y("The file could not be opened for reading, or there was not enough data to parse its headers.")));

    } catch (mtx::input::open_x &error) {
      mxerror(fmt::format(FY("The demultiplexer for the file '{0}' failed to initialize:\n{1}\n"), file->ti->m_fname, Y("The file could not be opened for reading, or there was not enough data to parse its headers.")));

    } catch (mtx::input::invalid_format_x &error) {
      mxerror(fmt::format(FY("The demultiplexer for the file '{0}' failed to initialize:\n{1}\n"), file->ti->m_fname, Y("The file content does not match its format type and was not recognized.")));

    } catch (mtx::input::header_parsing_x &error) {
      mxerror(fmt::format(FY("The demultiplexer for the file '{0}' failed to initialize:\n{1}\n"), file->ti->m_fname, Y("The file headers could not be parsed, e.g. because they're incomplete, invalid or damaged.")));

    } catch (mtx::input::exception &error) {
      mxerror(fmt::format(FY("The demultiplexer for the file '{0}' failed to initialize:\n{1}\n"), file->ti->m_fname, error.error()));
    }
  }
}
