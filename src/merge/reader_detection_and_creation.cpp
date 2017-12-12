/** \brief output handling

   mkvmerge -- utility for splicing together Matroska files
   from component media sub-types

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

// #include "common/logger.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/strings/formatting.h"
#include "common/xml/xml.h"
#include "input/r_aac.h"
#include "input/r_aac_adif.h"
#include "input/r_ac3.h"
#include "input/r_asf.h"
#include "input/r_avc.h"
#include "input/r_avi.h"
#include "input/r_cdxa.h"
#include "input/r_coreaudio.h"
#include "input/r_dirac.h"
#include "input/r_dts.h"
#include "input/r_dv.h"
#include "input/r_flac.h"
#include "input/r_flv.h"
#include "input/r_hdsub.h"
#include "input/r_hdmv_textst.h"
#include "input/r_hevc.h"
#include "input/r_ivf.h"
#include "input/r_matroska.h"
#include "input/r_microdvd.h"
#include "input/r_mp3.h"
#include "input/r_mpeg_es.h"
#include "input/r_mpeg_ps.h"
#include "input/r_mpeg_ts.h"
#include "input/r_ogm.h"
#include "input/r_pgssup.h"
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
#include "merge/filelist.h"
#include "merge/input_x.h"
#include "merge/reader_detection_and_creation.h"

static std::vector<bfs::path>
file_names_to_paths(const std::vector<std::string> &file_names) {
  std::vector<bfs::path> paths;
  for (auto &file_name : file_names)
    paths.push_back(bfs::system_complete(bfs::path(file_name)));

  return paths;
}

static mm_io_cptr
open_input_file(filelist_t &file) {
  try {
    if (file.all_names.size() == 1)
      return mm_io_cptr(new mm_read_buffer_io_c(new mm_file_io_c(file.name)));

    else {
      std::vector<bfs::path> paths = file_names_to_paths(file.all_names);
      return mm_io_cptr(new mm_read_buffer_io_c(new mm_multi_file_io_c(paths, file.name)));
    }

  } catch (mtx::mm_io::exception &ex) {
    mxerror(boost::format(Y("The file '%1%' could not be opened for reading: %2%.\n")) % file.name % ex);
    return mm_io_cptr{};

  } catch (...) {
    mxerror(boost::format(Y("The source file '%1%' could not be opened successfully, or retrieving its size by seeking to the end did not work.\n")) % file.name);
    return mm_io_cptr{};
  }
}

static bool
open_playlist_file(filelist_t &file,
                   mm_io_c *in) {
  auto mpls_in = mm_mpls_multi_file_io_c::open_multi(in);
  if (!mpls_in)
    return false;

  file.is_playlist      = true;
  file.playlist_mpls_in = std::static_pointer_cast<mm_mpls_multi_file_io_c>(mpls_in);

  return true;
}

template<typename Treader,
         typename Tio,
         typename ...Targs>
int
do_probe(Tio &io,
         Targs && ...args) {
  // log_it(boost::format("%1%: probe start") % typeid(Treader).name());
  auto result = Treader::probe_file(&(*io), std::forward<Targs>(args)...);
  // log_it(boost::format("%1%: probe done") % typeid(Treader).name());

  return result;
}

static mtx::file_type_e
detect_text_file_formats(filelist_t const &file) {
  auto text_io = mm_text_io_cptr{};
  try {
    text_io        = std::make_shared<mm_text_io_c>(new mm_file_io_c(file.name));
    auto text_size = text_io->get_size();

    if (do_probe<webvtt_reader_c>(text_io, text_size))
      return mtx::file_type_e::webvtt;
    else if (do_probe<srt_reader_c>(text_io, text_size))
      return mtx::file_type_e::srt;
    else if (do_probe<ssa_reader_c>(text_io, text_size))
      return mtx::file_type_e::ssa;
    else if (do_probe<vobsub_reader_c>(text_io, text_size))
      return mtx::file_type_e::vobsub;
    else if (do_probe<usf_reader_c>(text_io, text_size))
      return mtx::file_type_e::usf;

    // Unsupported text subtitle formats
    else if (do_probe<microdvd_reader_c>(text_io, text_size))
      return mtx::file_type_e::microdvd;

  } catch (mtx::mm_io::exception &ex) {
    mxerror(boost::format(Y("The file '%1%' could not be opened for reading: %2%.\n")) % file.name % ex);

  } catch (...) {
    mxerror(boost::format(Y("The source file '%1%' could not be opened successfully, or retrieving its size by seeking to the end did not work.\n")) % file.name);
  }

  return mtx::file_type_e::is_unknown;
}

/** \brief Probe the file type

   Opens the input file and calls the \c probe_file function for each known
   file reader class. Uses \c mm_text_io_c for subtitle probing.
*/
static std::pair<mtx::file_type_e, int64_t>
get_file_type_internal(filelist_t &file) {
  mm_io_cptr af_io = open_input_file(file);
  mm_io_c *io      = af_io.get();
  int64_t size     = std::min(io->get_size(), static_cast<int64_t>(1 << 25));

  auto is_playlist = !file.is_playlist && open_playlist_file(file, io);
  if (is_playlist)
    io = file.playlist_mpls_in.get();

  // File types that can be detected unambiguously but are not supported
  if (do_probe<aac_adif_reader_c>(io, size))
    return { mtx::file_type_e::aac, size };
  if (do_probe<asf_reader_c>(io, size))
    return { mtx::file_type_e::asf, size };
  if (do_probe<cdxa_reader_c>(io, size))
    return { mtx::file_type_e::cdxa, size };
  if (do_probe<flv_reader_c>(io, size))
    return { mtx::file_type_e::flv, size };
  if (do_probe<hdsub_reader_c>(io, size))
    return { mtx::file_type_e::hdsub, size };

  // File types that can be detected unambiguously
  if (do_probe<avi_reader_c>(io, size))
    return { mtx::file_type_e::avi, size };
  if (do_probe<kax_reader_c>(io, size))
    return { mtx::file_type_e::matroska, size };
  if (do_probe<wav_reader_c>(io, size))
    return { mtx::file_type_e::wav, size };
  if (do_probe<ogm_reader_c>(io, size))
    return { mtx::file_type_e::ogm, size };
  if (do_probe<hdmv_textst_reader_c>(io, size))
    return { mtx::file_type_e::hdmv_textst, size };
  if (do_probe<flac_reader_c>(io, size))
    return { mtx::file_type_e::flac, size };
  if (do_probe<pgssup_reader_c>(io, size))
    return { mtx::file_type_e::pgssup, size };
  if (do_probe<real_reader_c>(io, size))
    return { mtx::file_type_e::real, size };
  if (do_probe<qtmp4_reader_c>(io, size))
    return { mtx::file_type_e::qtmp4, size };
  if (do_probe<tta_reader_c>(io, size))
    return { mtx::file_type_e::tta, size };
  if (do_probe<vc1_es_reader_c>(io, size))
    return { mtx::file_type_e::vc1, size };
  if (do_probe<wavpack_reader_c>(io, size))
    return { mtx::file_type_e::wavpack4, size };
  if (do_probe<ivf_reader_c>(io, size))
    return { mtx::file_type_e::ivf, size };
  if (do_probe<coreaudio_reader_c>(io, size))
    return { mtx::file_type_e::coreaudio, size };
  if (do_probe<dirac_es_reader_c>(io, size))
    return { mtx::file_type_e::dirac, size };

  // All text file types (subtitles).
  auto type = detect_text_file_formats(file);

  if (mtx::file_type_e::is_unknown != type)
    return { type, size };

  // Try raw audio formats and require eight consecutive frames at the
  // start of the file.
  if (do_probe<mp3_reader_c>(io, size, 128 * 1024, 8, true))
    return { mtx::file_type_e::mp3, size };
  if (do_probe<ac3_reader_c>(io, size, 128 * 1024, 8, true))
    return { mtx::file_type_e::ac3, size };
  if (do_probe<aac_reader_c>(io, size, 128 * 1024, 8, true))
    return { mtx::file_type_e::aac, size };

  // File types that are mis-detected sometimes
  if (do_probe<dts_reader_c>(io, size, true))
    return { mtx::file_type_e::dts, size };
  if (do_probe<mtx::mpeg_ts::reader_c>(io, size))
    return { mtx::file_type_e::mpeg_ts, size };
  if (do_probe<mpeg_ps_reader_c>(io, size))
    return { mtx::file_type_e::mpeg_ps, size };

  // File types which are the same in raw format and in other container formats.
  // Detection requires 20 or more consecutive packets.
  static std::vector<int> s_probe_sizes1{ { 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 } };
  static int const s_probe_num_required_consecutive_packets1 = 64;

  for (auto probe_size : s_probe_sizes1) {
    if (do_probe<mp3_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets1))
      return { mtx::file_type_e::mp3, size };
    if (do_probe<ac3_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets1))
      return { mtx::file_type_e::ac3, size };
    if (do_probe<aac_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets1))
      return { mtx::file_type_e::aac, size };
  }

  // More file types with detection issues.
  if (do_probe<truehd_reader_c>(io, size))
    return { mtx::file_type_e::truehd, size };
  if (do_probe<dts_reader_c>(io, size))
    return { mtx::file_type_e::dts, size };
  if (do_probe<vobbtn_reader_c>(io, size))
    return { mtx::file_type_e::vobbtn, size };

  // Try some more of the raw audio formats before trying elementary
  // stream video formats (MPEG 1/2, AVC/h.264, HEVC/h.265; those
  // often enough simply work). However, require that the first frame
  // starts at the beginning of the file.
  if (do_probe<mp3_reader_c>(io, size, 32 * 1024, 1, true))
    return { mtx::file_type_e::mp3, size };
  if (do_probe<ac3_reader_c>(io, size, 32 * 1024, 1, true))
    return { mtx::file_type_e::ac3, size };
  if (do_probe<aac_reader_c>(io, size, 32 * 1024, 1, true))
    return { mtx::file_type_e::aac, size };

  if (do_probe<mpeg_es_reader_c>(io, size))
    return { mtx::file_type_e::mpeg_es, size };
  if (do_probe<avc_es_reader_c>(io, size))
    return { mtx::file_type_e::avc_es, size };
  if (do_probe<hevc_es_reader_c>(io, size))
    return { mtx::file_type_e::hevc_es, size };

  // File types which are the same in raw format and in other container formats.
  // Detection requires 20 or more consecutive packets.
  static std::vector<int> s_probe_sizes2{ { 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 } };
  static int const s_probe_num_required_consecutive_packets2 = 20;

  for (auto probe_size : s_probe_sizes2) {
    if (do_probe<mp3_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets2))
      return { mtx::file_type_e::mp3, size };
    else if (do_probe<ac3_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets2))
      return { mtx::file_type_e::ac3, size };
    else if (do_probe<aac_reader_c>(io, size, probe_size, s_probe_num_required_consecutive_packets2))
      return { mtx::file_type_e::aac, size };
  }

  // File types that are mis-detected sometimes and that aren't supported
  if (do_probe<dv_reader_c>(io, size))
    return { mtx::file_type_e::dv, size };

  return { mtx::file_type_e::is_unknown, size };
}

void
get_file_type(filelist_t &file) {
  auto result = get_file_type_internal(file);

  g_file_sizes += result.second;

  file.size     = result.second;
  file.type     = result.first;
}

/** \brief Creates the file readers

   For each file the appropriate file reader class is instantiated.
   The newly created class must read all track information in its
   constructor and throw an exception in case of an error. Otherwise
   it is assumed that the file can be handled.
*/
void
create_readers() {
  static auto s_debug_timestamp_restrictions = debugging_option_c{"timestamp_restrictions"};

  for (auto &file : g_files) {
    try {
      mm_io_cptr input_file = file->playlist_mpls_in ? std::static_pointer_cast<mm_io_c>(file->playlist_mpls_in) : open_input_file(*file);

      switch (file->type) {
        case mtx::file_type_e::aac:
          file->reader.reset(new aac_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::ac3:
          file->reader.reset(new ac3_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::avc_es:
          file->reader.reset(new avc_es_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::hevc_es:
          file->reader.reset(new hevc_es_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::avi:
          file->reader.reset(new avi_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::coreaudio:
          file->reader.reset(new coreaudio_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::dirac:
          file->reader.reset(new dirac_es_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::dts:
          file->reader.reset(new dts_reader_c(*file->ti, input_file));
          break;
#if defined(HAVE_FLAC_FORMAT_H)
        case mtx::file_type_e::flac:
          file->reader.reset(new flac_reader_c(*file->ti, input_file));
          break;
#endif
        case mtx::file_type_e::flv:
          file->reader.reset(new flv_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::hdmv_textst:
          file->reader.reset(new hdmv_textst_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::ivf:
          file->reader.reset(new ivf_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::matroska:
          file->reader.reset(new kax_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::mp3:
          file->reader.reset(new mp3_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::mpeg_es:
          file->reader.reset(new mpeg_es_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::mpeg_ps:
          file->reader.reset(new mpeg_ps_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::mpeg_ts:
          file->reader.reset(new mtx::mpeg_ts::reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::ogm:
          file->reader.reset(new ogm_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::pgssup:
          file->reader.reset(new pgssup_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::qtmp4:
          file->reader.reset(new qtmp4_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::real:
          file->reader.reset(new real_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::ssa:
          file->reader.reset(new ssa_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::srt:
          file->reader.reset(new srt_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::truehd:
          file->reader.reset(new truehd_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::tta:
          file->reader.reset(new tta_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::usf:
          file->reader.reset(new usf_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::vc1:
          file->reader.reset(new vc1_es_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::vobbtn:
          file->reader.reset(new vobbtn_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::vobsub:
          file->reader.reset(new vobsub_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::wav:
          file->reader.reset(new wav_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::wavpack4:
          file->reader.reset(new wavpack_reader_c(*file->ti, input_file));
          break;
        case mtx::file_type_e::webvtt:
          file->reader.reset(new webvtt_reader_c(*file->ti, input_file));
          break;
        default:
          mxerror(boost::format(Y("EVIL internal bug! (unknown file type). %1%\n")) % BUGMSG);
          break;
      }

      file->reader->read_headers();
      file->reader->set_timestamp_restrictions(file->restricted_timestamp_min, file->restricted_timestamp_max);

      // Re-calculate file size because the reader might switch to a
      // multi I/O reader in read_headers().
      file->size = file->reader->get_file_size();

      mxdebug_if(s_debug_timestamp_restrictions,
                 boost::format("Timestamp restrictions for %3%: min %1% max %2%\n") % file->restricted_timestamp_min % file->restricted_timestamp_max % file->ti->m_fname);

    } catch (mtx::mm_io::open_x &error) {
      mxerror(boost::format(Y("The demultiplexer for the file '%1%' failed to initialize:\n%2%\n")) % file->ti->m_fname % Y("The file could not be opened for reading, or there was not enough data to parse its headers."));

    } catch (mtx::input::open_x &error) {
      mxerror(boost::format(Y("The demultiplexer for the file '%1%' failed to initialize:\n%2%\n")) % file->ti->m_fname % Y("The file could not be opened for reading, or there was not enough data to parse its headers."));

    } catch (mtx::input::invalid_format_x &error) {
      mxerror(boost::format(Y("The demultiplexer for the file '%1%' failed to initialize:\n%2%\n")) % file->ti->m_fname % Y("The file content does not match its format type and was not recognized."));

    } catch (mtx::input::header_parsing_x &error) {
      mxerror(boost::format(Y("The demultiplexer for the file '%1%' failed to initialize:\n%2%\n")) % file->ti->m_fname % Y("The file headers could not be parsed, e.g. because they're incomplete, invalid or damaged."));

    } catch (mtx::input::exception &error) {
      mxerror(boost::format(Y("The demultiplexer for the file '%1%' failed to initialize:\n%2%\n")) % file->ti->m_fname % error.error());
    }
  }
}
