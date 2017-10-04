/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   OGG media stream reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <cmath>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "avilib.h"
#include "common/aac.h"
#include "common/chapters/chapters.h"
#include "common/codec.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/ivf.h"
#include "common/mpeg4_p2.h"
#include "common/ogmstreams.h"
#include "input/r_ogm.h"
#include "input/r_ogm_flac.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_avc_es.h"
#include "output/p_kate.h"
#include "output/p_mp3.h"
#include "output/p_mpeg4_p2.h"
#include "output/p_opus.h"
#include "output/p_pcm.h"
#include "output/p_textsubs.h"
#include "output/p_theora.h"
#include "output/p_video_for_windows.h"
#include "output/p_vorbis.h"
#include "output/p_vpx.h"

#define BUFFER_SIZE 4096

struct ogm_frame_t {
  memory_c *mem;
  int64_t duration;
  unsigned char flags;
};

class ogm_a_aac_demuxer_c: public ogm_demuxer_c {
public:
  mtx::aac::audio_config_t audio_config{};

public:
  ogm_a_aac_demuxer_c(ogm_reader_c *p_reader);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;
};

class ogm_a_ac3_demuxer_c: public ogm_demuxer_c {
public:

public:
  ogm_a_ac3_demuxer_c(ogm_reader_c *p_reader);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;
};

class ogm_a_mp3_demuxer_c: public ogm_demuxer_c {
public:

public:
  ogm_a_mp3_demuxer_c(ogm_reader_c *p_reader);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;
};

class ogm_a_pcm_demuxer_c: public ogm_demuxer_c {
public:

public:
  ogm_a_pcm_demuxer_c(ogm_reader_c *p_reader);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;
};

class ogm_a_opus_demuxer_c: public ogm_demuxer_c {
protected:
  timestamp_c m_calculated_end_timestamp;

  static debugging_option_c ms_debug;

public:
  ogm_a_opus_demuxer_c(ogm_reader_c *p_reader);

  virtual void process_page(int64_t granulepos);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;

  virtual bool is_header_packet(ogg_packet &op) override {
    if (op.bytes < 8)
      return false;

    return (0 == std::strncmp(reinterpret_cast<char const *>(&op.packet[0]), "OpusHead", 8))
        || (0 == std::strncmp(reinterpret_cast<char const *>(&op.packet[0]), "OpusTags", 8));
  };
};

debugging_option_c ogm_a_opus_demuxer_c::ms_debug{"opus"};

class ogm_a_vorbis_demuxer_c: public ogm_demuxer_c {
public:

public:
  ogm_a_vorbis_demuxer_c(ogm_reader_c *p_reader);

  virtual void process_page(int64_t granulepos);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_AUDIO;
  };

  virtual void initialize() override;
};

class ogm_s_text_demuxer_c: public ogm_demuxer_c {
public:

public:
  ogm_s_text_demuxer_c(ogm_reader_c *p_reader);

  virtual void process_page(int64_t granulepos);

  virtual generic_packetizer_c *create_packetizer();

  virtual const char *get_type() {
    return ID_RESULT_TRACK_SUBTITLES;
  };
};

class ogm_v_mscomp_demuxer_c: public ogm_demuxer_c {
public:
  int64_t frames_since_granulepos_change;

public:
  ogm_v_mscomp_demuxer_c(ogm_reader_c *p_reader);

  virtual const char *get_type() {
    return ID_RESULT_TRACK_VIDEO;
  };

  virtual std::string get_codec();

  virtual void initialize();
  virtual generic_packetizer_c *create_packetizer();
  virtual void process_page(int64_t granulepos);
  virtual std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
};

class ogm_v_avc_demuxer_c: public ogm_v_mscomp_demuxer_c {
public:

public:
  ogm_v_avc_demuxer_c(ogm_reader_c *p_reader);

  virtual generic_packetizer_c *create_packetizer();
  virtual std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
};

class ogm_v_theora_demuxer_c: public ogm_demuxer_c {
public:
  theora_identification_header_t theora;

public:
  ogm_v_theora_demuxer_c(ogm_reader_c *p_reader);

  virtual const char *get_type() {
    return ID_RESULT_TRACK_VIDEO;
  };

  virtual void initialize();
  virtual generic_packetizer_c *create_packetizer();
  virtual void process_page(int64_t granulepos);
  virtual bool is_header_packet(ogg_packet &op);
  virtual std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
};

class ogm_v_vp8_demuxer_c: public ogm_demuxer_c {
public:
  vp8_ogg_header_t vp8_header;
  unsigned int pixel_width, pixel_height;
  int64_t frame_rate_num, frame_rate_den;
  int64_t frames_since_granulepos_change;
  debugging_option_c debug;

public:
  ogm_v_vp8_demuxer_c(ogm_reader_c *p_reader, ogg_packet &op);

  virtual const char *get_type() {
    return ID_RESULT_TRACK_VIDEO;
  };

  virtual void initialize();
  virtual generic_packetizer_c *create_packetizer();
  virtual void process_page(int64_t granulepos);
  virtual std::pair<unsigned int, unsigned int> get_pixel_dimensions() const;
  virtual bool is_header_packet(ogg_packet &op) override;
};

class ogm_s_kate_demuxer_c: public ogm_demuxer_c {
public:
  kate_identification_header_t kate;

public:
  ogm_s_kate_demuxer_c(ogm_reader_c *p_reader);

  virtual const char *get_type() {
    return ID_RESULT_TRACK_SUBTITLES;
  };

  virtual void initialize();
  virtual generic_packetizer_c *create_packetizer();
  virtual void process_page(int64_t granulepos);
  virtual bool is_header_packet(ogg_packet &op);
};

// ---------------------------------------------------------------------------------

static std::shared_ptr<std::vector<std::string> >
extract_vorbis_comments(const memory_cptr &mem) {
  std::shared_ptr<std::vector<std::string> > comments(new std::vector<std::string>);
  mm_mem_io_c in(mem->get_buffer(), mem->get_size());
  uint32_t i, n, len;

  try {
    in.skip(7);                 // 0x03 "vorbis"
    n = in.read_uint32_le();    // vendor_length
    in.skip(n);                 // vendor_string
    n = in.read_uint32_le();    // user_comment_list_length

    comments->reserve(n);

    for (i = 0; i < n; i++) {
      len = in.read_uint32_le();
      memory_cptr buffer(new memory_c((unsigned char *)safemalloc(len + 1), len + 1));

      if (in.read(buffer->get_buffer(), len) != len)
        throw false;

      comments->push_back(std::string((char *)buffer->get_buffer(), len));
    }
  } catch(...) {
  }

  return comments;
}

/*
   Probes a file by simply comparing the first four bytes to 'OggS'.
*/
int
ogm_reader_c::probe_file(mm_io_c *in,
                         uint64_t size) {
  unsigned char data[4];

  if (4 > size)
    return 0;
  try {
    in->setFilePointer(0, seek_beginning);
    if (in->read(data, 4) != 4)
      return 0;
    in->setFilePointer(0, seek_beginning);
  } catch (...) {
    return 0;
  }
  if (strncmp((char *)data, "OggS", 4))
    return 0;
  return 1;
}

/*
   Opens the file for processing, initializes an ogg_sync_state used for
   reading from an OGG stream.
*/
ogm_reader_c::ogm_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c(ti, in)
{
}

void
ogm_reader_c::read_headers() {
  if (!ogm_reader_c::probe_file(m_in.get(), m_size))
    throw mtx::input::invalid_format_x();

  ogg_sync_init(&oy);

  show_demuxer_info();

  if (read_headers_internal() <= 0)
    throw mtx::input::header_parsing_x();
  handle_stream_comments();
}

ogm_reader_c::~ogm_reader_c() {
  ogg_sync_clear(&oy);
}

ogm_demuxer_cptr
ogm_reader_c::find_demuxer(int serialno) {
  size_t i;

  for (i = 0; i < sdemuxers.size(); i++)
    if (sdemuxers[i]->serialno == serialno) {
      if (sdemuxers[i]->in_use)
        return sdemuxers[i];
      return ogm_demuxer_cptr{};
    }

  return ogm_demuxer_cptr{};
}

/*
   Reads an OGG page from the stream. Returns 0 if there are no more pages
   left, EMOREDATA otherwise.
*/
int
ogm_reader_c::read_page(ogg_page *og) {
  int np, done, nread;
  unsigned char *buf;

  done = 0;
  while (!done) {
    np = ogg_sync_pageseek(&oy, og);

    // np == 0 means that there is not enough data for a complete page.
    if (0 >= np) {
      // np < 0 is the error case. Should not happen with local OGG files.
      if (0 > np)
        mxwarn_fn(m_ti.m_fname, Y("Could not find the next Ogg page. This indicates a damaged Ogg/Ogm file. Will try to continue.\n"));

      buf = (unsigned char *)ogg_sync_buffer(&oy, BUFFER_SIZE);
      if (!buf)
        mxerror_fn(m_ti.m_fname, Y("ogg_sync_buffer failed\n"));

      if (0 >= (nread = m_in->read(buf, BUFFER_SIZE)))
        return 0;

      ogg_sync_wrote(&oy, nread);
    } else
      // Alright, we have a page.
      done = 1;
  }

  // Here EMOREDATA actually indicates success - a page has been read.
  return FILE_STATUS_MOREDATA;
}

void
ogm_reader_c::create_packetizer(int64_t tid) {
  ogm_demuxer_cptr dmx;
  generic_packetizer_c *ptzr;

  if ((0 > tid) || (sdemuxers.size() <= static_cast<size_t>(tid)))
    return;
  dmx = sdemuxers[tid];

  if (!dmx->in_use)
    return;

  m_ti.m_private_data.reset();
  m_ti.m_id           = tid;
  m_ti.m_language     = dmx->language;
  m_ti.m_track_name   = dmx->title;

  ptzr                = dmx->create_packetizer();

  if (ptzr)
    dmx->ptzr         = add_packetizer(ptzr);

  m_ti.m_language.clear();
  m_ti.m_track_name.clear();
}

void
ogm_reader_c::create_packetizers() {
  size_t i;

  for (i = 0; i < sdemuxers.size(); i++)
    create_packetizer(i);
}

/*
   Checks every demuxer if it has a page available.
*/
int
ogm_reader_c::packet_available() {
  if (sdemuxers.empty())
    return 0;

  size_t i;
  for (i = 0; i < sdemuxers.size(); i++)
    if ((-1 != sdemuxers[i]->ptzr) && !PTZR(sdemuxers[i]->ptzr)->packet_available())
      return 0;

  return 1;
}

void
ogm_reader_c::handle_new_stream_and_packets(ogg_page *og) {
  ogm_demuxer_cptr dmx;

  handle_new_stream(og);
  dmx = find_demuxer(ogg_page_serialno(og));
  if (dmx)
    process_header_packets(dmx);
}

/*
   The page is the beginning of a new stream. Check the contents for known
   stream headers. If it is a known stream and the user has requested that
   it should be extracted then allocate a new packetizer based on the
   stream type and store the needed data in a new ogm_demuxer_c.
*/
void
ogm_reader_c::handle_new_stream(ogg_page *og) {
  ogg_stream_state os;
  ogg_packet op;

  if (ogg_stream_init(&os, ogg_page_serialno(og))) {
    mxwarn_fn(m_ti.m_fname, boost::format(Y("ogg_stream_init for stream number %1% failed. Will try to continue and ignore this stream.\n")) % sdemuxers.size());
    return;
  }

  // Read the first page and get its first packet.
  ogg_stream_pagein(&os, og);
  ogg_stream_packetout(&os, &op);

  ogm_demuxer_c *dmx = nullptr;

  /*
   * Check the contents for known stream headers. This one is the
   * standard Vorbis header.
   */
  if ((7 <= op.bytes) && !strncmp((char *)&op.packet[1], "vorbis", 6))
    dmx = new ogm_a_vorbis_demuxer_c(this);

  else if ((8 <= op.bytes) && !memcmp(&op.packet[0], "OpusHead", 8))
    dmx = new ogm_a_opus_demuxer_c(this);

  else if ((7 <= op.bytes) && !strncmp((char *)&op.packet[1], "theora", 6))
    dmx = new ogm_v_theora_demuxer_c(this);

  else if ((8 <= op.bytes) && !memcmp(&op.packet[1], "kate\0\0\0", 7))
    dmx = new ogm_s_kate_demuxer_c(this);

  // FLAC
  else if (   ((4 <= op.bytes) && !strncmp(reinterpret_cast<char *>(&op.packet[0]), "fLaC", 4))
           || ((5 <= op.bytes) && !strncmp(reinterpret_cast<char *>(&op.packet[1]), "FLAC", 4) && (0x7f == op.packet[0]))) {
#if !defined(HAVE_FLAC_FORMAT_H)
    if (demuxing_requested('a', sdemuxers.size(), dmx->language))
      mxerror_fn(m_ti.m_fname, Y("mkvmerge has not been compiled with FLAC support but handling of this stream has been requested.\n"));

    else {
      dmx         = new ogm_demuxer_c(this);
      dmx->codec  = codec_c::look_up(codec_c::type_e::A_FLAC);
      dmx->in_use = true;
    }

#else
    dmx = new ogm_a_flac_demuxer_c(this, 0x7f == op.packet[0] ? ofm_post_1_1_1 : ofm_pre_1_1_1);
#endif

  } else if ((static_cast<size_t>(op.bytes) >= sizeof(vp8_ogg_header_t)) && (0x4f == op.packet[0]) && (get_uint32_be(&op.packet[1]) == 0x56503830))
    dmx = new ogm_v_vp8_demuxer_c(this, op);

  else if (((*op.packet & PACKET_TYPE_BITS ) == PACKET_TYPE_HEADER) && (op.bytes >= ((int)sizeof(stream_header) + 1))) {
    // The new stream headers introduced by OggDS (see ogmstreams.h).

    stream_header *sth = (stream_header *)(op.packet + 1);
    char buf[5];
    buf[4] = 0;

    if (!strncmp(sth->streamtype, "video", 5)) {
      memcpy(buf, (char *)sth->subtype, 4);

      if (mtx::avc::is_avc_fourcc(buf) && !hack_engaged(ENGAGE_ALLOW_AVC_IN_VFW_MODE))
        dmx = new ogm_v_avc_demuxer_c(this);
      else
        dmx = new ogm_v_mscomp_demuxer_c(this);

    } else if (!strncmp(sth->streamtype, "audio", 5)) {
      memcpy(buf, (char *)sth->subtype, 4);

      uint32_t codec_id = strtol(buf, (char **)nullptr, 16);

      if (0x0001 == codec_id)
        dmx = new ogm_a_pcm_demuxer_c(this);

      else if ((0x0050 == codec_id) || (0x0055 == codec_id))
        dmx = new ogm_a_mp3_demuxer_c(this);

      else if (0x2000 == codec_id)
        dmx = new ogm_a_ac3_demuxer_c(this);

      else if (0x00ff == codec_id)
        dmx = new ogm_a_aac_demuxer_c(this);

      else
        mxwarn_fn(m_ti.m_fname, boost::format(Y("Unknown audio stream type 0x%|1$04x|. Stream ID %2% will be ignored.\n")) % codec_id % sdemuxers.size());

    } else if (!strncmp(sth->streamtype, "text", 4))
      dmx = new ogm_s_text_demuxer_c(this);
  }

  /*
   * The old OggDS headers (see MPlayer's source, libmpdemux/demux_ogg.c)
   * are not supported.
   */

  if (!dmx)
    dmx = new ogm_demuxer_c(this);

  std::string type = dmx->get_type();

  dmx->serialno    = ogg_page_serialno(og);
  dmx->track_id    = sdemuxers.size();
  dmx->in_use      = (type != "unknown") && demuxing_requested(type[0], dmx->track_id, dmx->language);

  dmx->packet_data.push_back(memory_c::clone(op.packet, op.bytes));

  memcpy(&dmx->os, &os, sizeof(ogg_stream_state));

  sdemuxers.push_back(ogm_demuxer_cptr(dmx));

  dmx->initialize();
}

/*
   Process the contents of a page. First find the demuxer associated with
   the page's serial number. If there is no such demuxer then either the
   OGG file is damaged (very rare) or the page simply belongs to a stream
   that the user didn't want extracted.
   If the demuxer is found then hand over all packets in this page to the
   associated packetizer.
*/
void
ogm_reader_c::process_page(ogg_page *og) {
  ogm_demuxer_cptr dmx;
  int64_t granulepos;

  dmx = find_demuxer(ogg_page_serialno(og));
  if (!dmx || !dmx->in_use)
    return;

  granulepos = ogg_page_granulepos(og);

  if ((-1 != granulepos) && (granulepos < dmx->last_granulepos)) {
    mxwarn_tid(m_ti.m_fname, dmx->track_id,
               Y("The timestamps for this stream have been reset in the middle of the file. This is not supported. The current packet will be discarded.\n"));
    return;
  }

  ogg_stream_pagein(&dmx->os, og);

  dmx->process_page(granulepos);

  dmx->last_granulepos = granulepos;
}

void
ogm_reader_c::process_header_page(ogg_page *og) {
  ogm_demuxer_cptr dmx;

  dmx = find_demuxer(ogg_page_serialno(og));
  if (!dmx ||dmx->headers_read)
    return;

  ogg_stream_pagein(&dmx->os, og);
  process_header_packets(dmx);
}

/*
   Search and store additional headers for the Ogg streams.
*/
void
ogm_reader_c::process_header_packets(ogm_demuxer_cptr dmx) {
  if (dmx->headers_read)
    return;

  dmx->process_header_page();
}

/*
   Read all header packets and - for Vorbis streams - the comment and
   codec data packets.
*/
int
ogm_reader_c::read_headers_internal() {
  ogg_page og;

  bool done = false;
  while (!done) {
    // Make sure we have a page that we can work with.
    if (read_page(&og) == FILE_STATUS_DONE)
      return 0;

    // Is this the first page of a new stream?
    if (ogg_page_bos(&og))
      handle_new_stream_and_packets(&og);
    else {                // No, so check if it's still a header page.
      bos_pages_read = 1;
      process_header_page(&og);

      done = true;

      size_t i;
      for (i = 0; i < sdemuxers.size(); i++) {
        ogm_demuxer_cptr &dmx = sdemuxers[i];
        if (!dmx->headers_read && dmx->in_use) {
          done = false;
          break;
        }
      }
    }
  }

  m_in->setFilePointer(0, seek_beginning);
  ogg_sync_clear(&oy);
  ogg_sync_init(&oy);

  return 1;
}

/*
   General reader. Read a page and hand it over for processing.
*/
file_status_e
ogm_reader_c::read(generic_packetizer_c *,
                   bool) {
  // Some tracks may contain huge gaps. We don't want to suck in the complete
  // file.
  if (get_queued_bytes() > 20 * 1024 * 1024)
    return FILE_STATUS_HOLDING;

  ogg_page og;

  do {
    // Make sure we have a page that we can work with.
    if (read_page(&og) == FILE_STATUS_DONE)
      return flush_packetizers();

    // Is this the first page of a new stream? No, so process it normally.
    if (!ogg_page_bos(&og))
      process_page(&og);
  } while (ogg_page_bos(&og));

  size_t i;
  // Are there streams that have not finished yet?
  for (i = 0; i < sdemuxers.size(); i++)
    if (!sdemuxers[i]->eos && sdemuxers[i]->in_use)
      return FILE_STATUS_MOREDATA;

  // No, we're done with this file.
  return flush_packetizers();
}

void
ogm_reader_c::identify() {
  auto info = mtx::id::info_c{};
  size_t i;

  // Check if a video track has a TITLE comment. If yes we use this as the
  // new segment title / global file title.
  for (i = 0; i < sdemuxers.size(); i++)
    if ((sdemuxers[i]->title != "") && sdemuxers[i]->ms_compat) {
      info.add(mtx::id::title, sdemuxers[i]->title);
      break;
    }

  id_result_container(info.get());

  for (i = 0; i < sdemuxers.size(); i++) {
    info = mtx::id::info_c{};

    info.set(mtx::id::number,   sdemuxers[i]->serialno);
    info.add(mtx::id::language, sdemuxers[i]->language);

    if (!sdemuxers[i]->title.empty() && !sdemuxers[i]->ms_compat)
      info.add(mtx::id::track_name, sdemuxers[i]->title);

    if ((0 != sdemuxers[i]->display_width) && (0 != sdemuxers[i]->display_height))
      info.add(mtx::id::display_dimensions, boost::format("%1%x%2%") % sdemuxers[i]->display_width % sdemuxers[i]->display_height);

    if (dynamic_cast<ogm_s_text_demuxer_c *>(sdemuxers[i].get()) || dynamic_cast<ogm_s_kate_demuxer_c *>(sdemuxers[i].get())) {
      info.add(mtx::id::text_subtitles, true);
      info.add(mtx::id::encoding,       "UTF-8");
    }

    auto pixel_dimensions = sdemuxers[i]->get_pixel_dimensions();
    if (pixel_dimensions.first && pixel_dimensions.second)
      info.add(mtx::id::pixel_dimensions, boost::format("%1%x%2%") % pixel_dimensions.first % pixel_dimensions.second);

    info.add(mtx::id::audio_channels,           sdemuxers[i]->channels);
    info.add(mtx::id::audio_sampling_frequency, sdemuxers[i]->sample_rate);

    id_result_track(i, sdemuxers[i]->get_type(), sdemuxers[i]->get_codec(), info.get());
  }

  if (m_chapters.get())
    id_result_chapters(mtx::chapters::count_atoms(*m_chapters));
}

void
ogm_reader_c::handle_stream_comments() {
  std::shared_ptr<std::vector<std::string> > comments;
  std::string title;

  bool charset_warning_printed = false;
  charset_converter_cptr cch   = charset_converter_c::init(m_ti.m_chapter_charset);
  size_t i;

  for (i = 0; i < sdemuxers.size(); i++) {
    ogm_demuxer_cptr &dmx = sdemuxers[i];
    if (dmx->codec.is(codec_c::type_e::A_FLAC) || (2 > dmx->packet_data.size()))
      continue;

    comments = extract_vorbis_comments(dmx->packet_data[1]);
    if (comments->empty())
      continue;

    std::vector<std::string> chapter_strings;

    size_t j;
    for (j = 0; comments->size() > j; j++) {
      mxverb(2, boost::format("ogm_reader: commment for #%1% for %2%: %3%\n") % j % i % (*comments)[j]);
      std::vector<std::string> comment = split((*comments)[j], "=", 2);
      if (comment.size() != 2)
        continue;

      if (comment[0] == "LANGUAGE") {
        int index;

        index = map_to_iso639_2_code(comment[1].c_str());
        if (-1 != index)
          dmx->language = g_iso639_languages[index].iso639_2_code;
        else {

          std::string lang = comment[1];
          int pos1         = lang.find("[");
          while (0 <= pos1) {
            int pos2 = lang.find("]", pos1);
            if (-1 == pos2)
              pos2 = lang.length() - 1;
            lang.erase(pos1, pos2 - pos1 + 1);
            pos1 = lang.find("[");
          }

          pos1 = lang.find("(");
          while (0 <= pos1) {
            int pos2 = lang.find(")", pos1);
            if (-1 == pos2)
              pos2 = lang.length() - 1;
            lang.erase(pos1, pos2 - pos1 + 1);
            pos1 = lang.find("(");
          }

          index = map_to_iso639_2_code(lang.c_str());
          if (-1 != index)
            dmx->language = g_iso639_languages[index].iso639_2_code;
        }

      } else if (comment[0] == "TITLE")
        title = comment[1];

      else if (balg::starts_with(comment[0], "CHAPTER"))
        chapter_strings.push_back((*comments)[j]);
    }

    bool segment_title_set = false;
    if (title != "") {
      title = cch->utf8(title);
      if (!g_segment_title_set && g_segment_title.empty() && dmx->ms_compat) {
        g_segment_title     = title;
        g_segment_title_set = true;
        segment_title_set   = true;
      }
      dmx->title = title.c_str();
      title      = "";
    }

    auto chapters_set               = false;
    auto exception_parsing_chapters = false;

    if (!chapter_strings.empty() && !m_ti.m_no_chapters) {
      try {
        std::shared_ptr<mm_mem_io_c> out(new mm_mem_io_c(nullptr, 0, 1000));

        if (g_identifying) {
          // OGM comments might be written in a charset other than
          // UTF-8. During identification we simply want to know how
          // many chapter entries there are. As the GUI's users have no
          // way to specify the chapter charset when adding a file
          // such charset issues may cause mkvmerge to abort or not to
          // show any chapters at all (if "parse_chapters()"
          // fails). So just remove all chars > 127.
          for (auto chapter_string : chapter_strings) {
            // auto cleaned = std::string{};
            // balg::copy_if(chapter_string, [](char c) { return c >= 0; }, std::back_inserter(cleaned));
            brng::remove_erase_if(chapter_string, [](char c) { return c < 0; });
            out->puts(chapter_string + std::string{"\n"});
          }

        } else {
          out->write_bom("UTF-8");
          for (auto &chapter_string : chapter_strings)
            out->puts(cch->utf8(chapter_string) + std::string{"\n"});
        }

        out->set_file_name(m_ti.m_fname);

        std::shared_ptr<mm_text_io_c> text_out(new mm_text_io_c(out.get(), false));

        m_chapters   = mtx::chapters::parse(text_out.get(), 0, -1, 0, m_ti.m_chapter_language, "", true);
        chapters_set = true;

        mtx::chapters::align_uids(m_chapters.get());
      } catch (...) {
        exception_parsing_chapters = true;
      }
    }

    if (   exception_parsing_chapters
        || (   (segment_title_set || chapters_set)
            && !charset_warning_printed
            && (m_ti.m_chapter_charset.empty()))) {
      mxwarn_fn(m_ti.m_fname,
                Y("This Ogg/OGM file contains chapter or title information. Unfortunately the charset used to store this information in "
                  "the file cannot be identified unambiguously. The program assumes that your system's current charset is appropriate. This can "
                  "be overridden with the '--chapter-charset <charset>' switch.\n"));
      charset_warning_printed = true;

      if (exception_parsing_chapters)
        mxwarn_fn(m_ti.m_fname,
                  Y("This Ogg/OGM file contains chapters but they could not be parsed. "
                    "This can be due to the character set not being set properly for them or due to the entries not matching the expected SRT-style format.\n"));
    }
  }
}

void
ogm_reader_c::add_available_track_ids() {
  add_available_track_id_range(sdemuxers.size());
}

// -----------------------------------------------------------

ogm_demuxer_c::ogm_demuxer_c(ogm_reader_c *p_reader)
  : reader(p_reader)
  , m_ti(p_reader->m_ti)
  , ptzr(-1)
  , ms_compat{}
  , serialno(0)
  , eos(0)
  , units_processed(0)
  , num_non_header_packets(0)
  , headers_read(false)
  , first_granulepos(0)
  , last_granulepos(0)
  , default_duration(0)
  , in_use(false)
  , display_width(0)
  , display_height(0)
  , channels{}
  , sample_rate{}
  , bits_per_sample{}
{
  memset(&os, 0, sizeof(ogg_stream_state));
}

ogm_demuxer_c::~ogm_demuxer_c() {
  ogg_stream_clear(&os);
}

void
ogm_demuxer_c::get_duration_and_len(ogg_packet &op,
                                    int64_t &duration,
                                    int &duration_len) {
  duration_len  = (*op.packet & PACKET_LEN_BITS01) >> 6;
  duration_len |= (*op.packet & PACKET_LEN_BITS2)  << 1;

  duration      = 0;

  if ((0 < duration_len) && (op.bytes >= (duration_len + 1))) {
    for (int i = 0; i < duration_len; i++) {
      duration <<= 8;
      duration  += *((unsigned char *)op.packet + duration_len - i);
    }
  } else
    duration = 0;
}

void
ogm_demuxer_c::process_page(int64_t /* granulepos */) {
  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if (((*op.packet & 3) == PACKET_TYPE_HEADER) || ((*op.packet & 3) == PACKET_TYPE_COMMENT))
      continue;

    int64_t duration;
    int duration_len;
    get_duration_and_len(op, duration, duration_len);

    memory_c *mem = new memory_c(&op.packet[duration_len + 1], op.bytes - 1 - duration_len, false);
    reader->m_reader_packetizers[ptzr]->process(new packet_t(mem));
    units_processed += op.bytes - 1;
  }
}

void
ogm_demuxer_c::process_header_page() {
  if (headers_read)
    return;

  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if (!is_header_packet(op)) {
      if (nh_packet_data.size() < num_non_header_packets) {
        nh_packet_data.push_back(memory_c::clone(op.packet, op.bytes));
        continue;
      }

      headers_read = true;
      ogg_stream_reset(&os);
      return;
    }

    packet_data.push_back(memory_c::clone(op.packet, op.bytes));
  }
}

std::pair<unsigned int, unsigned int>
ogm_demuxer_c::get_pixel_dimensions()
  const {
  return { 0, 0 };
}

// -----------------------------------------------------------

ogm_a_aac_demuxer_c::ogm_a_aac_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::A_AAC);
}

void
ogm_a_aac_demuxer_c::initialize() {
  boost::optional<mtx::aac::audio_config_t> parsed_audio_config;

  if (packet_data[0]->get_size() >= (sizeof(stream_header) + 5))
    parsed_audio_config = mtx::aac::parse_audio_specific_config(packet_data[0]->get_buffer() + sizeof(stream_header) + 5,
                                                                packet_data[0]->get_size()   - sizeof(stream_header) - 5);

  if (parsed_audio_config) {
    audio_config = *parsed_audio_config;
    if (audio_config.sbr)
      audio_config.profile = AAC_PROFILE_SBR;

  } else {
    auto sth                 = reinterpret_cast<stream_header *>(&packet_data[0]->get_buffer()[1]);
    audio_config.channels    = get_uint16_le(&sth->sh.audio.channels);
    audio_config.sample_rate = get_uint64_le(&sth->samples_per_unit);
    audio_config.profile     = AAC_PROFILE_LC;
  }

  mxverb(2,
         boost::format("ogm_reader: %1%/%2%: profile %3%, channels %4%, sample_rate %5%, sbr %6%, output_sample_rate %7%\n")
         % m_ti.m_id % m_ti.m_fname % audio_config.profile % audio_config.channels % audio_config.sample_rate % audio_config.sbr % audio_config.output_sample_rate);
}

generic_packetizer_c *
ogm_a_aac_demuxer_c::create_packetizer() {
  auto ptzr_obj = new aac_packetizer_c(reader, m_ti, audio_config, aac_packetizer_c::headerless);
  if (audio_config.sbr)
    ptzr_obj->set_audio_output_sampling_freq(audio_config.output_sample_rate);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

// -----------------------------------------------------------

ogm_a_ac3_demuxer_c::ogm_a_ac3_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::A_AC3);
}

void
ogm_a_ac3_demuxer_c::initialize() {
  auto sth    = reinterpret_cast<stream_header *>(packet_data[0]->get_buffer() + 1);
  channels    = get_uint16_le(&sth->sh.audio.channels);
  sample_rate = get_uint64_le(&sth->samples_per_unit);
}

generic_packetizer_c *
ogm_a_ac3_demuxer_c::create_packetizer() {
  auto ptzr_obj = new ac3_packetizer_c(reader, m_ti, sample_rate, channels, 0);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

// -----------------------------------------------------------

ogm_a_mp3_demuxer_c::ogm_a_mp3_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::A_MP3);
}

void
ogm_a_mp3_demuxer_c::initialize() {
  auto sth    = reinterpret_cast<stream_header *>(packet_data[0]->get_buffer() + 1);
  channels    = get_uint16_le(&sth->sh.audio.channels);
  sample_rate = get_uint64_le(&sth->samples_per_unit);
}

generic_packetizer_c *
ogm_a_mp3_demuxer_c::create_packetizer() {
  auto ptzr_obj = new mp3_packetizer_c(reader, m_ti, sample_rate, channels, true);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

// -----------------------------------------------------------

ogm_a_pcm_demuxer_c::ogm_a_pcm_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::A_PCM);
}

void
ogm_a_pcm_demuxer_c::initialize() {
  auto sth        = reinterpret_cast<stream_header *>(packet_data[0]->get_buffer() + 1);
  channels        = get_uint16_le(&sth->sh.audio.channels);
  sample_rate     = get_uint64_le(&sth->samples_per_unit);
  bits_per_sample = get_uint16_le(&sth->bits_per_sample);
}

generic_packetizer_c *
ogm_a_pcm_demuxer_c::create_packetizer() {
  auto ptzr_obj = new pcm_packetizer_c(reader, m_ti, sample_rate, channels, bits_per_sample);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

// -----------------------------------------------------------

ogm_a_vorbis_demuxer_c::ogm_a_vorbis_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::A_VORBIS);
}

void
ogm_a_vorbis_demuxer_c::initialize() {
  vorbis_info vi;
  vorbis_comment vc;
  ogg_packet packet;

  memset(&packet, 0, sizeof(ogg_packet));

  packet.packet = packet_data[0]->get_buffer();
  packet.bytes  = packet_data[0]->get_size();
  packet.b_o_s  = 1;

  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  vorbis_synthesis_headerin(&vi, &vc, &packet);

  sample_rate = vi.rate;
  channels    = vi.channels;

  vorbis_info_clear(&vi);
  vorbis_comment_clear(&vc);
}

generic_packetizer_c *
ogm_a_vorbis_demuxer_c::create_packetizer() {
  generic_packetizer_c *ptzr_obj = new vorbis_packetizer_c(reader, m_ti,
                                                           packet_data[0]->get_buffer(), packet_data[0]->get_size(),
                                                           packet_data[1]->get_buffer(), packet_data[1]->get_size(),
                                                           packet_data[2]->get_buffer(), packet_data[2]->get_size());

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_a_vorbis_demuxer_c::process_page(int64_t /* granulepos */) {
  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if (((*op.packet & 3) == PACKET_TYPE_HEADER) || ((*op.packet & 3) == PACKET_TYPE_COMMENT))
      continue;

    reader->m_reader_packetizers[ptzr]->process(new packet_t(new memory_c(op.packet, op.bytes, false)));
  }
}

// -----------------------------------------------------------

ogm_a_opus_demuxer_c::ogm_a_opus_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
  , m_calculated_end_timestamp{timestamp_c::ns(0)}
{
  codec = codec_c::look_up(codec_c::type_e::A_OPUS);
}

generic_packetizer_c *
ogm_a_opus_demuxer_c::create_packetizer() {
  m_ti.m_private_data = packet_data[0];
  auto ptzr_obj       = new opus_packetizer_c(reader, m_ti);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_a_opus_demuxer_c::process_page(int64_t granulepos) {
  ogg_packet op;

  auto ogg_timestamp = timestamp_c::ns(granulepos * 1000000000 / 48000);

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if ((4 <= op.bytes) && !memcmp(op.packet, "Opus", 4))
      continue;

    auto packet                = std::make_shared<packet_t>(memory_c::clone(op.packet, op.bytes));
    auto toc                   = mtx::opus::toc_t::decode(packet->data);
    m_calculated_end_timestamp += toc.packet_duration;

    if (m_calculated_end_timestamp > ogg_timestamp) {
      packet->discard_padding = m_calculated_end_timestamp - ogg_timestamp;
      mxdebug_if(ms_debug,
                 boost::format("Opus discard padding calculated %1% Ogg timestamp %2% diff %3% samples %4% (Ogg page's granulepos %5%)\n")
                 % m_calculated_end_timestamp % ogg_timestamp % packet->discard_padding % (packet->discard_padding.to_ns() * 48000 / 1000000000) % granulepos);
    }

    reader->m_reader_packetizers[ptzr]->process(packet);
  }
}

void
ogm_a_opus_demuxer_c::initialize() {
  auto id_header = mtx::opus::id_header_t::decode(packet_data[0]);
  sample_rate    = id_header.input_sample_rate;
  channels       = id_header.channels;
}

// -----------------------------------------------------------

ogm_s_text_demuxer_c::ogm_s_text_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::S_SRT);
}

generic_packetizer_c *
ogm_s_text_demuxer_c::create_packetizer() {
  generic_packetizer_c *ptzr_obj = new textsubs_packetizer_c(reader, m_ti, MKV_S_TEXTUTF8, true, false);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_s_text_demuxer_c::process_page(int64_t granulepos) {
  ogg_packet op;
  int duration_len;
  int64_t duration;

  units_processed++;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if (((*op.packet & 3) == PACKET_TYPE_HEADER) || ((*op.packet & 3) == PACKET_TYPE_COMMENT))
      continue;

    get_duration_and_len(op, duration, duration_len);

    if (((op.bytes - 1 - duration_len) > 2) || ((op.packet[duration_len + 1] != ' ') && (op.packet[duration_len + 1] != 0) && !iscr(op.packet[duration_len + 1]))) {
      memory_c *mem = new memory_c(&op.packet[duration_len + 1], op.bytes - 1 - duration_len, false);
      reader->m_reader_packetizers[ptzr]->process(new packet_t(mem, granulepos * 1000000, (int64_t)duration * 1000000));
    }
  }
}

// -----------------------------------------------------------

ogm_v_avc_demuxer_c::ogm_v_avc_demuxer_c(ogm_reader_c *p_reader)
  : ogm_v_mscomp_demuxer_c(p_reader)
{
  codec                  = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
  num_non_header_packets = 3;
}

generic_packetizer_c *
ogm_v_avc_demuxer_c::create_packetizer() {
  stream_header *sth          = (stream_header *)&packet_data[0]->get_buffer()[1];
  generic_packetizer_c *vptzr = new avc_es_video_packetizer_c(reader, m_ti);

  vptzr->set_video_pixel_dimensions(get_uint32_le(&sth->sh.video.width), get_uint32_le(&sth->sh.video.height));

  show_packetizer_info(m_ti.m_id, vptzr);

  return vptzr;
}

std::pair<unsigned int, unsigned int>
ogm_v_avc_demuxer_c::get_pixel_dimensions()
  const {
  auto sth = reinterpret_cast<stream_header *>(&packet_data[0]->get_buffer()[1]);
  return { get_uint32_le(&sth->sh.video.width), get_uint32_le(&sth->sh.video.height) };
}

// -----------------------------------------------------------

ogm_v_mscomp_demuxer_c::ogm_v_mscomp_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
  , frames_since_granulepos_change(0)
{
  ms_compat = true;
}

std::string
ogm_v_mscomp_demuxer_c::get_codec() {
  stream_header *sth = (stream_header *)(packet_data[0]->get_buffer() + 1);
  char fourcc[5];

  memcpy(fourcc, sth->subtype, 4);
  fourcc[4] = 0;

  return fourcc;
}

void
ogm_v_mscomp_demuxer_c::initialize() {
  stream_header *sth = (stream_header *)(packet_data[0]->get_buffer() + 1);
  codec              = codec_c::look_up(get_codec());

  if (0 > g_video_fps)
    g_video_fps = 10000000.0 / (float)get_uint64_le(&sth->time_unit);

  default_duration = 100 * get_uint64_le(&sth->time_unit);
}

generic_packetizer_c *
ogm_v_mscomp_demuxer_c::create_packetizer() {
  alBITMAPINFOHEADER bih;
  stream_header *sth = (stream_header *)&packet_data[0]->get_buffer()[1];

  // AVI compatibility mode. Fill in the values we've got and guess
  // the others.
  memset(&bih, 0, sizeof(alBITMAPINFOHEADER));
  put_uint32_le(&bih.bi_size,       sizeof(alBITMAPINFOHEADER));
  put_uint32_le(&bih.bi_width,      get_uint32_le(&sth->sh.video.width));
  put_uint32_le(&bih.bi_height,     get_uint32_le(&sth->sh.video.height));
  put_uint16_le(&bih.bi_planes,     1);
  put_uint16_le(&bih.bi_bit_count,  24);
  put_uint32_le(&bih.bi_size_image, get_uint32_le(&bih.bi_width) * get_uint32_le(&bih.bi_height) * 3);
  memcpy(&bih.bi_compression, sth->subtype, 4);

  m_ti.m_private_data = memory_c::clone(&bih, sizeof(alBITMAPINFOHEADER));

  double fps          = (double)10000000.0 / get_uint64_le(&sth->time_unit);
  int width           = get_uint32_le(&sth->sh.video.width);
  int height          = get_uint32_le(&sth->sh.video.height);

  generic_packetizer_c *ptzr_obj;
  if (mpeg4::p2::is_fourcc(sth->subtype))
    ptzr_obj = new mpeg4_p2_video_packetizer_c(reader, m_ti, fps, width, height, false);
  else
    ptzr_obj = new video_for_windows_packetizer_c(reader, m_ti, fps, width, height);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

std::pair<unsigned int, unsigned int>
ogm_v_mscomp_demuxer_c::get_pixel_dimensions()
  const {
  stream_header *sth = reinterpret_cast<stream_header *>(&packet_data[0]->get_buffer()[1]);

  return { get_uint32_le(&sth->sh.video.width), get_uint32_le(&sth->sh.video.height) };
}

void
ogm_v_mscomp_demuxer_c::process_page(int64_t granulepos) {
  std::vector<ogm_frame_t> frames;
  ogg_packet op;
  int64_t duration;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if (((*op.packet & 3) == PACKET_TYPE_HEADER) || ((*op.packet & 3) == PACKET_TYPE_COMMENT))
      continue;

    int duration_len;
    get_duration_and_len(op, duration, duration_len);

    if (!duration_len || !duration)
      duration = 1;

    ogm_frame_t frame = {
      new memory_c(&op.packet[duration_len + 1], op.bytes - 1 - duration_len, false),
      duration * default_duration,
      op.packet[0],
    };

    frames.push_back(frame);
  }

  // Is there a gap in the granulepos values?
  if (static_cast<size_t>(granulepos - last_granulepos) > frames.size())
    last_granulepos = granulepos - frames.size();

  size_t i;
  for (i = 0; i < frames.size(); ++i) {
    ogm_frame_t &frame = frames[i];

    int64_t timestamp = (last_granulepos + frames_since_granulepos_change) * default_duration;
    ++frames_since_granulepos_change;

    reader->m_reader_packetizers[ptzr]->process(new packet_t(frame.mem, timestamp, frame.duration, frame.flags & PACKET_IS_SYNCPOINT ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC));

    units_processed += duration;
  }

  if (granulepos != last_granulepos)
    frames_since_granulepos_change = 0;
}

// -----------------------------------------------------------

ogm_v_theora_demuxer_c::ogm_v_theora_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::V_THEORA);

  memset(&theora, 0, sizeof(theora_identification_header_t));
}

void
ogm_v_theora_demuxer_c::initialize() {
  try {
    memory_cptr &mem = packet_data[0];
    theora_parse_identification_header(mem->get_buffer(), mem->get_size(), theora);

    display_width  = theora.display_width;
    display_height = theora.display_height;
  } catch (mtx::theora::header_parsing_x &e) {
    mxerror_tid(reader->m_ti.m_fname, track_id, boost::format(Y("The Theora identification header could not be parsed (%1%).\n")) % e.error());
  }
}

generic_packetizer_c *
ogm_v_theora_demuxer_c::create_packetizer() {
  m_ti.m_private_data            = lace_memory_xiph(packet_data);

  double                fps      = (double)theora.frn / (double)theora.frd;
  generic_packetizer_c *ptzr_obj = new theora_video_packetizer_c(reader, m_ti, fps, theora.fmbw, theora.fmbh);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_v_theora_demuxer_c::process_page(int64_t granulepos) {
  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if ((0 != op.bytes) && (0 != (op.packet[0] & 0x80)))
      // Header packets have the very first bit set. Skip those.
      continue;

    // Zero-length frames are 'repeat previous frame' markers and
    // cannot be I frames.
    bool is_keyframe = (0 != op.bytes) && (0x00 == (op.packet[0] & 0x40));
    int64_t timestamp = (int64_t)(1000000000.0 * units_processed * theora.frd / theora.frn);
    int64_t duration = (int64_t)(1000000000.0 *                   theora.frd / theora.frn);
    int64_t bref     = is_keyframe ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC;

    ++units_processed;

    reader->m_reader_packetizers[ptzr]->process(new packet_t(new memory_c(op.packet, op.bytes, false), timestamp, duration, bref, VFT_NOBFRAME));

    mxverb(3,
           boost::format("Theora track %1% kfgshift %2% granulepos 0x%|3$08x| %|4$08x|%5%\n")
           % track_id % theora.kfgshift % (granulepos >> 32) % (granulepos & 0xffffffff) % (is_keyframe ? " key" : ""));
  }
}

bool
ogm_v_theora_demuxer_c::is_header_packet(ogg_packet &op) {
  return (0 != op.bytes) && (0x80 <= op.packet[0]) && (0x82 >= op.packet[0]);
}

std::pair<unsigned int, unsigned int>
ogm_v_theora_demuxer_c::get_pixel_dimensions()
  const {
  return { theora.fmbw, theora.fmbh };
}

// -----------------------------------------------------------

ogm_v_vp8_demuxer_c::ogm_v_vp8_demuxer_c(ogm_reader_c *p_reader,
                                         ogg_packet &op)
  : ogm_demuxer_c(p_reader)
  , pixel_width(0)
  , pixel_height(0)
  , frame_rate_num(0)
  , frame_rate_den(0)
  , frames_since_granulepos_change(0)
  , debug{"ogg_vp8|ogm_vp8"}
{
  codec = codec_c::look_up(codec_c::type_e::V_VP8);

  memcpy(&vp8_header, op.packet, sizeof(vp8_ogg_header_t));
}

void
ogm_v_vp8_demuxer_c::initialize() {
  pixel_width          = get_uint16_be(&vp8_header.pixel_width);
  pixel_height         = get_uint16_be(&vp8_header.pixel_height);
  unsigned int par_num = get_uint16_be(&vp8_header.par_num);
  unsigned int par_den = get_uint16_be(&vp8_header.par_den);

  if ((0 != par_num) && (0 != par_den)) {
    if ((static_cast<double>(pixel_width) / static_cast<double>(pixel_height)) < (static_cast<double>(par_num) / static_cast<double>(par_den))) {
      display_width  = std::llround(static_cast<double>(pixel_width) * par_num / par_den);
      display_height = pixel_height;
    } else {
      display_width  = pixel_width;
      display_height = std::llround(static_cast<double>(pixel_height) * par_den / par_num);
    }

  } else {
    display_width  = pixel_width;
    display_height = pixel_height;
  }

  frame_rate_num   = static_cast<uint64_t>(get_uint32_be(&vp8_header.frame_rate_num));
  frame_rate_den   = static_cast<uint64_t>(get_uint32_be(&vp8_header.frame_rate_den));
  default_duration = frame_rate_den * 1000000000ull / frame_rate_num;
}

generic_packetizer_c *
ogm_v_vp8_demuxer_c::create_packetizer() {
  auto ptzr_obj = new vpx_video_packetizer_c(reader, m_ti, codec_c::type_e::V_VP8);

  ptzr_obj->set_video_pixel_width(pixel_width);
  ptzr_obj->set_video_pixel_height(pixel_height);
  ptzr_obj->set_video_display_width(display_width);
  ptzr_obj->set_video_display_height(display_height);
  ptzr_obj->set_track_default_duration(default_duration);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_v_vp8_demuxer_c::process_page(int64_t granulepos) {
  if ((0 < granulepos) && (granulepos != last_granulepos))
    last_granulepos = granulepos;

  ogg_packet op;
  std::vector<memory_cptr> packets;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if ((units_processed > 0) || !is_header_packet(op))
      packets.push_back(memory_c::clone(op.packet, op.bytes));
  }

  if (packets.empty())
    return;

  auto pts         = last_granulepos >> 32;
  auto inv_count   = (last_granulepos >> 30) & 0x03;
  auto distance    = (last_granulepos >>  3) & 0x07ffffff;
  auto num_packets = packets.size();

  for (auto idx = 0u; idx < num_packets; ++idx) {
    // Zero-length packets are 'repeat previous frame' markers and
    // cannot be I packets.
    auto &data       = packets[idx];
    auto is_keyframe = (0 != data->get_size()) && ((data->get_buffer()[0] & 0x01) == 0);
    auto frame_num   = pts - std::min<int64_t>(pts, num_packets - idx - 1 + 1);
    auto timestamp    = static_cast<int64_t>(1000000000.0 * frame_num * frame_rate_den / frame_rate_num);
    auto bref        = is_keyframe ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC;

    ++units_processed;

    reader->m_reader_packetizers[ptzr]->process(new packet_t(data, timestamp, default_duration, bref, VFT_NOBFRAME));

    mxdebug_if(debug,
               boost::format("VP8 track %1% size %10% #proc %11% frame# %12% fr_num %2% fr_den %3% granulepos 0x%|4$08x| %|5$08x| pts %6% inv_count %7% distance %8%%9%\n")
               % track_id % frame_rate_num % frame_rate_den % pts % (last_granulepos & 0xffffffff)
               % pts % inv_count % distance % (is_keyframe ? " key" : "") % data->get_size() % units_processed % frame_num);
  }
}

std::pair<unsigned int, unsigned int>
ogm_v_vp8_demuxer_c::get_pixel_dimensions()
  const {
  return { pixel_width, pixel_height };
}

bool
ogm_v_vp8_demuxer_c::is_header_packet(ogg_packet &op) {
  if (   (static_cast<std::size_t>(op.bytes) >= sizeof(vp8_ogg_header_t))
      && (op.packet[0]                 == 0x4f)
      && (get_uint32_be(&op.packet[1]) == 0x56503830))
    return true;

  if (   (op.bytes     >= 7)
      && (op.packet[0] == 0x03)
      && (!std::memcmp(op.packet, "vorbis", 6)))
    return true;

  return false;
}

// -----------------------------------------------------------

ogm_s_kate_demuxer_c::ogm_s_kate_demuxer_c(ogm_reader_c *p_reader)
  : ogm_demuxer_c(p_reader)
{
  codec = codec_c::look_up(codec_c::type_e::S_KATE);

  memset(&kate, 0, sizeof(kate_identification_header_t));
}

void
ogm_s_kate_demuxer_c::initialize() {
  try {
    memory_cptr &mem = packet_data[0];
    kate_parse_identification_header(mem->get_buffer(), mem->get_size(), kate);
  } catch (mtx::kate::header_parsing_x &e) {
    mxerror_tid(reader->m_ti.m_fname, track_id, boost::format(Y("The Kate identification header could not be parsed (%1%).\n")) % e.error());
  }
}

generic_packetizer_c *
ogm_s_kate_demuxer_c::create_packetizer() {
  m_ti.m_private_data = lace_memory_xiph(packet_data);
  auto ptzr_obj       = new kate_packetizer_c(reader, m_ti);

  show_packetizer_info(m_ti.m_id, ptzr_obj);

  return ptzr_obj;
}

void
ogm_s_kate_demuxer_c::process_page(int64_t /* granulepos */) {
  ogg_packet op;

  while (ogg_stream_packetout(&os, &op) == 1) {
    eos |= op.e_o_s;

    if ((0 == op.bytes) || (0 != (op.packet[0] & 0x80)))
      continue;

    reader->m_reader_packetizers[ptzr]->process(new packet_t(new memory_c(op.packet, op.bytes, false)));

    ++units_processed;

    if (op.e_o_s) {
      eos = 1;
      return;
    }
  }
}

bool
ogm_s_kate_demuxer_c::is_header_packet(ogg_packet &op) {
  return (0x80 & op.packet[0]);
}
