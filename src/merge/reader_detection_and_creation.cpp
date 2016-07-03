/** \brief output handling

   mkvmerge -- utility for splicing together Matroska files
   from component media sub-types

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

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
      return mm_io_cptr(new mm_read_buffer_io_c(new mm_file_io_c(file.name), 1 << 17));

    else {
      std::vector<bfs::path> paths = file_names_to_paths(file.all_names);
      return mm_io_cptr(new mm_read_buffer_io_c(new mm_multi_file_io_c(paths, file.name), 1 << 17));
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

static file_type_e
detect_text_file_formats(filelist_t const &file) {
  auto text_io = mm_text_io_cptr{};
  try {
    text_io        = std::make_shared<mm_text_io_c>(new mm_file_io_c(file.name));
    auto text_size = text_io->get_size();

    if (webvtt_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_WEBVTT;
    else if (srt_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_SRT;
    else if (ssa_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_SSA;
    else if (vobsub_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_VOBSUB;
    else if (usf_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_USF;

    // Unsupported text subtitle formats
    else if (microdvd_reader_c::probe_file(text_io.get(), text_size))
      return FILE_TYPE_MICRODVD;

  } catch (mtx::mm_io::exception &ex) {
    mxerror(boost::format(Y("The file '%1%' could not be opened for reading: %2%.\n")) % file.name % ex);

  } catch (...) {
    mxerror(boost::format(Y("The source file '%1%' could not be opened successfully, or retrieving its size by seeking to the end did not work.\n")) % file.name);
  }

  return FILE_TYPE_IS_UNKNOWN;
}

/** \brief Probe the file type

   Opens the input file and calls the \c probe_file function for each known
   file reader class. Uses \c mm_text_io_c for subtitle probing.
*/
static std::pair<file_type_e, int64_t>
get_file_type_internal(filelist_t &file) {
  mm_io_cptr af_io = open_input_file(file);
  mm_io_c *io      = af_io.get();
  int64_t size     = std::min(io->get_size(), static_cast<int64_t>(1 << 25));

  auto is_playlist = !file.is_playlist && open_playlist_file(file, io);
  if (is_playlist)
    io = file.playlist_mpls_in.get();

  file_type_e type = FILE_TYPE_IS_UNKNOWN;

  // File types that can be detected unambiguously but are not supported
  if (aac_adif_reader_c::probe_file(io, size))
    type = FILE_TYPE_AAC;
  else if (asf_reader_c::probe_file(io, size))
    type = FILE_TYPE_ASF;
  else if (cdxa_reader_c::probe_file(io, size))
    type = FILE_TYPE_CDXA;
  else if (flv_reader_c::probe_file(io, size))
    type = FILE_TYPE_FLV;
  else if (hdsub_reader_c::probe_file(io, size))
    type = FILE_TYPE_HDSUB;

  // File types that can be detected unambiguously
  else if (avi_reader_c::probe_file(io, size))
    type = FILE_TYPE_AVI;
  else if (kax_reader_c::probe_file(io, size))
    type = FILE_TYPE_MATROSKA;
  else if (wav_reader_c::probe_file(io, size))
    type = FILE_TYPE_WAV;
  else if (ogm_reader_c::probe_file(io, size))
    type = FILE_TYPE_OGM;
  else if (flac_reader_c::probe_file(io, size))
    type = FILE_TYPE_FLAC;
  else if (pgssup_reader_c::probe_file(io, size))
    type = FILE_TYPE_PGSSUP;
  else if (real_reader_c::probe_file(io, size))
    type = FILE_TYPE_REAL;
  else if (qtmp4_reader_c::probe_file(io, size))
    type = FILE_TYPE_QTMP4;
  else if (tta_reader_c::probe_file(io, size))
    type = FILE_TYPE_TTA;
  else if (vc1_es_reader_c::probe_file(io, size))
    type = FILE_TYPE_VC1;
  else if (wavpack_reader_c::probe_file(io, size))
    type = FILE_TYPE_WAVPACK4;
  else if (ivf_reader_c::probe_file(io, size))
    type = FILE_TYPE_IVF;
  else if (coreaudio_reader_c::probe_file(io, size))
    type = FILE_TYPE_COREAUDIO;
  else if (dirac_es_reader_c::probe_file(io, size))
    type = FILE_TYPE_DIRAC;

  // All text file types (subtitles).
  else
    type = detect_text_file_formats(file);

  if (FILE_TYPE_IS_UNKNOWN != type)
    ;                           // intentional fall-through
  // File types that are mis-detected sometimes
  else if (dts_reader_c::probe_file(io, size, true))
    type = FILE_TYPE_DTS;
  else if (mpeg_ts_reader_c::probe_file(io, size))
    type = FILE_TYPE_MPEG_TS;
  else if (mpeg_ps_reader_c::probe_file(io, size))
    type = FILE_TYPE_MPEG_PS;
  else {
    // File types which are the same in raw format and in other container formats.
    // Detection requires 20 or more consecutive packets.
    static const int s_probe_sizes[]                          = { 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 };
    static const int s_probe_num_required_consecutive_packets = 64;

    int i;
    for (i = 0; (0 != s_probe_sizes[i]) && (FILE_TYPE_IS_UNKNOWN == type); ++i)
      if (mp3_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_MP3;
      else if (ac3_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_AC3;
      else if (aac_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_AAC;
  }
  // More file types with detection issues.
  if (type != FILE_TYPE_IS_UNKNOWN)
    ;
  else if (truehd_reader_c::probe_file(io, size))
    type = FILE_TYPE_TRUEHD;
  else if (dts_reader_c::probe_file(io, size))
    type = FILE_TYPE_DTS;
  else if (vobbtn_reader_c::probe_file(io, size))
    type = FILE_TYPE_VOBBTN;

  // Try some more of the raw audio formats before trying elementary
  // stream video formats (MPEG 1/2, AVC/h.264, HEVC/h.265; those
  // often enough simply work). However, require that the first frame
  // starts at the beginning of the file.
  else if (mp3_reader_c::probe_file(io, size, 32 * 1024, 1, true))
    type = FILE_TYPE_MP3;
  else if (ac3_reader_c::probe_file(io, size, 32 * 1024, 1, true))
    type = FILE_TYPE_AC3;
  else if (aac_reader_c::probe_file(io, size, 32 * 1024, 1, true))
    type = FILE_TYPE_AAC;

  else if (mpeg_es_reader_c::probe_file(io, size))
    type = FILE_TYPE_MPEG_ES;
  else if (avc_es_reader_c::probe_file(io, size))
    type = FILE_TYPE_AVC_ES;
  else if (hevc_es_reader_c::probe_file(io, size))
    type = FILE_TYPE_HEVC_ES;
  else {
    // File types which are the same in raw format and in other container formats.
    // Detection requires 20 or more consecutive packets.
    static const int s_probe_sizes[]                          = { 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024, 0 };
    static const int s_probe_num_required_consecutive_packets = 20;

    int i;
    for (i = 0; (0 != s_probe_sizes[i]) && (FILE_TYPE_IS_UNKNOWN == type); ++i)
      if (mp3_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_MP3;
      else if (ac3_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_AC3;
      else if (aac_reader_c::probe_file(io, size, s_probe_sizes[i], s_probe_num_required_consecutive_packets))
        type = FILE_TYPE_AAC;
  }

  // File types that are mis-detected sometimes and that aren't supported
  if (type != FILE_TYPE_IS_UNKNOWN)
    ;
  else if (dv_reader_c::probe_file(io, size))
    type = FILE_TYPE_DV;

  return std::make_pair(type, size);
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
  static auto s_debug_timecode_restrictions = debugging_option_c{"timecode_restrictions"};

  for (auto &file : g_files) {
    try {
      mm_io_cptr input_file = file->playlist_mpls_in ? std::static_pointer_cast<mm_io_c>(file->playlist_mpls_in) : open_input_file(*file);

      switch (file->type) {
        case FILE_TYPE_AAC:
          file->reader.reset(new aac_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_AC3:
          file->reader.reset(new ac3_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_AVC_ES:
          file->reader.reset(new avc_es_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_HEVC_ES:
          file->reader.reset(new hevc_es_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_AVI:
          file->reader.reset(new avi_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_COREAUDIO:
          file->reader.reset(new coreaudio_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_DIRAC:
          file->reader.reset(new dirac_es_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_DTS:
          file->reader.reset(new dts_reader_c(*file->ti, input_file));
          break;
#if defined(HAVE_FLAC_FORMAT_H)
        case FILE_TYPE_FLAC:
          file->reader.reset(new flac_reader_c(*file->ti, input_file));
          break;
#endif
        case FILE_TYPE_FLV:
          file->reader.reset(new flv_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_IVF:
          file->reader.reset(new ivf_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_MATROSKA:
          file->reader.reset(new kax_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_MP3:
          file->reader.reset(new mp3_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_MPEG_ES:
          file->reader.reset(new mpeg_es_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_MPEG_PS:
          file->reader.reset(new mpeg_ps_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_MPEG_TS:
          file->reader.reset(new mpeg_ts_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_OGM:
          file->reader.reset(new ogm_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_PGSSUP:
          file->reader.reset(new pgssup_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_QTMP4:
          file->reader.reset(new qtmp4_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_REAL:
          file->reader.reset(new real_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_SSA:
          file->reader.reset(new ssa_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_SRT:
          file->reader.reset(new srt_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_TRUEHD:
          file->reader.reset(new truehd_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_TTA:
          file->reader.reset(new tta_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_USF:
          file->reader.reset(new usf_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_VC1:
          file->reader.reset(new vc1_es_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_VOBBTN:
          file->reader.reset(new vobbtn_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_VOBSUB:
          file->reader.reset(new vobsub_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_WAV:
          file->reader.reset(new wav_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_WAVPACK4:
          file->reader.reset(new wavpack_reader_c(*file->ti, input_file));
          break;
        case FILE_TYPE_WEBVTT:
          file->reader.reset(new webvtt_reader_c(*file->ti, input_file));
          break;
        default:
          mxerror(boost::format(Y("EVIL internal bug! (unknown file type). %1%\n")) % BUGMSG);
          break;
      }

      file->reader->read_headers();
      file->reader->set_timecode_restrictions(file->restricted_timecode_min, file->restricted_timecode_max);

      // Re-calculate file size because the reader might switch to a
      // multi I/O reader in read_headers().
      file->size = file->reader->get_file_size();

      mxdebug_if(s_debug_timecode_restrictions,
                 boost::format("Timecode restrictions for %3%: min %1% max %2%\n") % file->restricted_timecode_min % file->restricted_timecode_max % file->ti->m_fname);

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
