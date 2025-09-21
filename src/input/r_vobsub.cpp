/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VobSub stream reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/codec.h"
#include "common/debugging.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/spu.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "input/r_vobsub.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_vobsub.h"

namespace {
debugging_option_c s_debug{"vobsub_reader"};

auto
hexvalue(char c) {
  return isdigit(c)        ? (c) - '0'
       : tolower(c) == 'a' ? 10
       : tolower(c) == 'b' ? 11
       : tolower(c) == 'c' ? 12
       : tolower(c) == 'd' ? 13
       : tolower(c) == 'e' ? 14
       :                     15;
}

auto
ishexdigit(char s) {
  return isdigit(s) || strchr("abcdefABCDEF", s);
}

std::pair<std::string, std::string>
idx_and_sub_file_names(std::string const &file_name) {
  auto idx_path = mtx::fs::to_path(file_name);
  auto sub_path = idx_path;

  idx_path.replace_extension(".idx");
  sub_path.replace_extension(".sub");

  return { idx_path.string(), sub_path.string() };
}

} // anonymous namespace

const std::string vobsub_reader_c::id_string("# VobSub index file, v");


bool
vobsub_entry_c::operator < (const vobsub_entry_c &cmp) const {
  return timestamp < cmp.timestamp;
}

bool
vobsub_reader_c::probe_file() {
  auto extension = boost::algorithm::to_lower_copy(mtx::fs::to_path(m_in->get_file_name()).extension().string());
  if (!mtx::included_in(extension, ".idx", ".sub"))
    return false;

  auto idx_path = idx_and_sub_file_names(m_in->get_file_name()).first;

  try {
    auto in    = std::make_unique<mm_text_io_c>(std::make_shared<mm_file_io_c>(idx_path));
    auto chunk = in->getline(id_string.size());

    return (chunk.size() == id_string.size())
      && boost::istarts_with(chunk, id_string);

  } catch (mtx::mm_io::exception &) {
  }

  return false;
}

void
vobsub_reader_c::read_headers() {
  auto [idx_name, sub_name] = idx_and_sub_file_names(m_ti.m_fname);

  try {
    m_in = std::make_unique<mm_text_io_c>(std::make_shared<mm_file_io_c>(idx_name));
  } catch (...) {
    throw mtx::input::extended_x(fmt::format(FY("Could not open '{0}' for reading.\n"), idx_name));
  }

  try {
    m_sub_file = std::make_shared<mm_file_io_c>(sub_name);
  } catch (...) {
    throw mtx::input::extended_x(fmt::format(FY("Could not open '{0}' for reading.\n"), sub_name));
  }

  idx_data = "";
  auto len = id_string.length();

  std::string line;
  if (!m_in->getline2(line) || !balg::istarts_with(line, id_string) || (line.length() < (len + 1)))
    mxerror_fn(m_ti.m_fname, Y("No version number found.\n"));

  version = line[len] - '0';
  len++;
  while ((len < line.length()) && isdigit(line[len])) {
    version = version * 10 + line[len] - '0';
    len++;
  }
  if (version < 7)
    mxerror_fn(m_ti.m_fname, Y("Only v7 and newer VobSub files are supported. If you have an older version then use the VSConv utility from "
                               "http://sourceforge.net/projects/guliverkli/ to convert these files to v7 files.\n"));

  parse_headers();
  show_demuxer_info();
}

vobsub_reader_c::~vobsub_reader_c() {
  uint32_t i;

  for (i = 0; i < tracks.size(); i++) {
    mxdebug_if(s_debug, fmt::format("r_vobsub track {0} SPU size: {1}, overall size: {2}, overhead: {3} ({4:.3f}%)\n",
                                    i, tracks[i]->spu_size, tracks[i]->spu_size + tracks[i]->overhead, tracks[i]->overhead, 100.0 * tracks[i]->overhead / (tracks[i]->overhead + tracks[i]->spu_size)));
    delete tracks[i];
  }
}

void
vobsub_reader_c::create_packetizer(int64_t tid) {
  if ((tracks.size() <= static_cast<size_t>(tid)) || !demuxing_requested('s', tid, tracks[tid]->language) || (-1 != tracks[tid]->ptzr))
    return;

  vobsub_track_c *track = tracks[tid];
  m_ti.m_id             = tid;
  m_ti.m_language       = tracks[tid]->language;
  m_ti.m_private_data   = memory_c::clone(idx_data);
  track->ptzr           = add_packetizer(new vobsub_packetizer_c(this, m_ti));

  int64_t avg_duration;
  if (!track->entries.empty()) {
    avg_duration = 0;
    size_t k;
    for (k = 0; k < (track->entries.size() - 1); k++) {
      int64_t duration            = track->entries[k + 1].timestamp - track->entries[k].timestamp;
      track->entries[k].duration  = duration;
      avg_duration               += duration;
    }
  } else
    avg_duration = 1000000000;

  if (1 < track->entries.size())
    avg_duration /= (track->entries.size() - 1);
  track->entries[track->entries.size() - 1].duration = avg_duration;

  for (int idx = 0, end = track->entries.size(); idx < end; ++idx) {
    auto end_position   = (idx + 1) < end ? track->entries[idx + 1].position : m_in->get_size();
    m_bytes_to_process += end_position - track->entries[idx].position;
  }

  m_ti.m_language.clear();
  show_packetizer_info(tid, ptzr(track->ptzr));
}

void
vobsub_reader_c::create_packetizers() {
  uint32_t i;

  for (i = 0; i < tracks.size(); i++)
    create_packetizer(i);
}

void
vobsub_reader_c::parse_headers() {
  mtx::bcp47::language_c language;
  std::string line;
  vobsub_track_c *track  = nullptr;
  int64_t line_no        = 0;
  int64_t last_timestamp = 0;
  bool sort_required     = false;

  m_in->setFilePointer(0);

  while (1) {
    if (!m_in->getline2(line))
      break;
    line_no++;

    if ((line.length() == 0) || (line[0] == '#'))
      continue;

    auto matches = QRegularExpression{"^id: *([^,\\n]+)", QRegularExpression::CaseInsensitiveOption}.match(Q(line));
    if (matches.hasMatch()) {
      language = mtx::bcp47::language_c::parse(to_utf8(matches.captured(1).toLower()));
      if (!language.is_valid())
        language = mtx::bcp47::language_c::parse("und");

      if (track) {
        if (track->entries.empty())
          delete track;
        else {
          tracks.push_back(track);
          if (sort_required) {
            mxdebug_if(s_debug, fmt::format("vobsub_reader: Sorting track {0}\n", tracks.size()));
            std::stable_sort(track->entries.begin(), track->entries.end());
          }
        }
      }
      track          = new vobsub_track_c(language);
      delay          = 0;
      last_timestamp = 0;
      sort_required  = false;
      continue;
    }

    if (balg::istarts_with(line, "alt:") || balg::istarts_with(line, "langidx:"))
      continue;

    if (balg::istarts_with(line, "delay:")) {
      line.erase(0, 6);
      mtx::string::strip(line);

      int factor = 1;
      if (!line.empty() && (line[0] == '-')) {
        factor = -1;
        line.erase(0, 1);
      }

      int64_t timestamp;
      if (!mtx::string::parse_timestamp(line, timestamp, true))
        mxerror_fn(m_ti.m_fname, fmt::format(FY("line {0}: The 'delay' timestamp could not be parsed.\n"), line_no));
      delay += timestamp * factor;
    }

    if ((7 == version) && balg::istarts_with(line, "timestamp:")) {
      if (!track)
        mxerror_fn(m_ti.m_fname, Y("The .idx file does not contain an 'id: ...' line to indicate the language.\n"));

      mtx::string::strip(line);
      mtx::string::shrink_whitespace(line);
      auto parts = mtx::string::split(line.c_str(), " ");

      if ((4 != parts.size()) || (13 > parts[1].length()) || !balg::iequals(parts[2], "filepos:")) {
        mxwarn_fn(m_ti.m_fname, fmt::format(FY("Line {0}: The line seems to be a subtitle entry but the format couldn't be recognized. This entry will be skipped.\n"), line_no));
        continue;
      }

      int idx         = 0;
      auto sline      = parts[3].c_str();
      int64_t filepos = hexvalue(sline[idx]);
      idx++;
      while ((0 != sline[idx]) && ishexdigit(sline[idx])) {
        filepos = (filepos << 4) + hexvalue(sline[idx]);
        idx++;
      }

      parts[1].erase(parts[1].length() - 1);
      int factor = 1;
      if ('-' == parts[1][0]) {
        factor = -1;
        parts[1].erase(0, 1);
      }

      int64_t timestamp;
      if (!mtx::string::parse_timestamp(parts[1], timestamp)) {
        mxwarn_fn(m_ti.m_fname,
                  fmt::format(FY("Line {0}: The line seems to be a subtitle entry but the format couldn't be recognized. This entry will be skipped.\n"), line_no));
        continue;
      }

      vobsub_entry_c entry;
      entry.position  = filepos;
      entry.timestamp = timestamp * factor + delay;

      if (   (0 >  delay)
          && (0 != last_timestamp)
          && (entry.timestamp < last_timestamp)) {
        delay           += last_timestamp - entry.timestamp;
        entry.timestamp  = last_timestamp;
      }

      if (0 > entry.timestamp) {
        mxwarn_fn(m_ti.m_fname,
                  fmt::format(FY("Line {0}: The line seems to be a subtitle entry but the timestamp was negative even after adding the track "
                                 "delay. Negative timestamps are not supported in Matroska. This entry will be skipped.\n"), line_no));
        continue;
      }

      track->entries.push_back(entry);

      if ((entry.timestamp < last_timestamp) &&
          demuxing_requested('s', tracks.size())) {
        mxwarn_fn(m_ti.m_fname,
                  fmt::format(FY("Line {0}: The current timestamp ({1}) is smaller than the previous one ({2}). "
                                 "The entries will be sorted according to their timestamps. "
                                 "This might result in the wrong order for some subtitle entries. "
                                 "If this is the case then you have to fix the .idx file manually.\n"),
                              line_no, mtx::string::format_timestamp(entry.timestamp, 3), mtx::string::format_timestamp(last_timestamp, 3)));
        sort_required = true;
      }
      last_timestamp = entry.timestamp;

      continue;
    }

    idx_data += line;
    idx_data += "\n";
  }

  if (track) {
    if (track->entries.size() == 0)
      delete track;
    else {
      tracks.push_back(track);
      if (sort_required) {
        mxdebug_if(s_debug, fmt::format("vobsub_reader: Sorting track {0}\n", tracks.size()));
        std::stable_sort(track->entries.begin(), track->entries.end());
      }
    }
  }

  if (!g_identifying && (1 < verbose)) {
    size_t i, k, tsize = tracks.size();
    for (i = 0; i < tsize; i++) {
      mxinfo(fmt::format("vobsub_reader: Track number {0}\n", i));
      for (k = 0; k < tracks[i]->entries.size(); k++)
        mxinfo(fmt::format("vobsub_reader:  {0:04} position: {1:12} (0x{2:04x}{3:08x}), timestamp: {4:12} ({5})\n",
                           k, tracks[i]->entries[k].position, tracks[i]->entries[k].position >> 32, tracks[i]->entries[k].position & 0xffffffff,
                           tracks[i]->entries[k].timestamp / 1000000, mtx::string::format_timestamp(tracks[i]->entries[k].timestamp, 3)));
    }
  }
}

int
vobsub_reader_c::deliver_packet(uint8_t *buf,
                                int size,
                                int64_t timestamp,
                                int64_t default_duration,
                                generic_packetizer_c *packetizer) {
  if (!buf || (0 == size)) {
    safefree(buf);
    return -1;
  }

  auto duration = mtx::spu::get_duration(buf, size);
  if (!duration.valid()) {
    duration = timestamp_c::ns(default_duration);
    std::string dbg;

    if (s_debug)
      dbg = fmt::format("vobsub_reader: Could not extract the duration for a SPU packet (timestamp: {0}).", mtx::string::format_timestamp(timestamp, 3));

    int dcsq  =                   get_uint16_be(&buf[2]);
    int dcsq2 = dcsq + 3 < size ? get_uint16_be(&buf[dcsq + 2]) : -1;

    // Some players ignore sub-pictures if there is no stop display command.
    // Add a stop display command only if 1 command chain exists and the hack is enabled.

    if ((dcsq == dcsq2) && mtx::hacks::is_engaged(mtx::hacks::VOBSUB_SUBPIC_STOP_CMDS)) {
      dcsq         += 2;        // points to first command chain
      dcsq2        += 4;        // find end of first command chain
      bool unknown  = false;
      uint8_t type;
      for (type = buf[dcsq2++]; 0xff != type; type = buf[dcsq2++]) {
        switch(type) {
          case 0x00:            // Menu ID, 1 byte
            break;
          case 0x01:            // Start display
            break;
          case 0x02:            // Stop display
            unknown = true;     // Can only have one Stop Display command
            break;
          case 0x03:            // Palette
            dcsq2 += 2;
            break;
          case 0x04:            // Alpha
            dcsq2 += 2;
            break;
          case 0x05:            // Coords
            dcsq2 += 6;
            break;
          case 0x06:            // Graphic lines
            dcsq2 += 4;
            break;
          default:
            unknown = true;
        }
      }

      if (!unknown) {
        size          += 6;
        uint32_t len   = (buf[0] << 8 | buf[1]) + 6; // increase sub-picture length by 6
        buf[0]         = (uint8_t)(len >> 8);
        buf[1]         = (uint8_t)(len);
        uint32_t stm   = duration.to_ns() / 1000000; // calculate STM for Stop Display
        stm            = ((stm - 1) * 90 >> 10) + 1;
        buf[dcsq]      = (uint8_t)(dcsq2 >> 8);      // set pointer to 2nd chain
        buf[dcsq + 1]  = (uint8_t)(dcsq2);
        buf[dcsq2]     = (uint8_t)(stm >> 8);        // set DCSQ_STM
        buf[dcsq2 + 1] = (uint8_t)(stm);
        buf[dcsq2 + 2] = (uint8_t)(dcsq2 >> 8);      // last chain so point to itself
        buf[dcsq2 + 3] = (uint8_t)(dcsq2);
        buf[dcsq2 + 4] = 0x02;                       // stop display command
        buf[dcsq2 + 5] = 0xff;                       // end command

        if (s_debug)
          dbg += fmt::format(" Added Stop Display cmd (SP_DCSQ_STM=0x{0:04x})", stm);
      }
    }

    mxdebug_if(s_debug, dbg + "\n");
  }

  if (duration.valid())
    packetizer->process(std::make_shared<packet_t>(memory_c::take_ownership(buf, size), timestamp, duration.to_ns()));
  else
    safefree(buf);

  return -1;
}

// Adopted from mplayer's vobsub.c
int
vobsub_reader_c::extract_one_spu_packet(int64_t track_id) {
  uint32_t len, idx, mpeg_version;
  int c, packet_aid;
  /* Goto start of a packet, it starts with 0x000001?? */
  const uint8_t wanted[] = { 0, 0, 1 };
  uint8_t buf[5];

  vobsub_track_c *track         = tracks[track_id];
  auto &packetizer              = ptzr(track->ptzr);
  int64_t timestamp             = track->entries[track->idx].timestamp;
  int64_t duration              = track->entries[track->idx].duration;
  uint64_t extraction_start_pos = track->entries[track->idx].position;
  uint64_t extraction_end_pos   = track->idx >= track->entries.size() - 1 ? m_sub_file->get_size() : track->entries[track->idx + 1].position;

  int64_t pts                   = 0;
  uint8_t *dst_buf              = nullptr;
  uint32_t dst_size             = 0;
  uint32_t packet_size          = 0;
  unsigned int spu_len          = 0;
  bool spu_len_valid            = false;

  m_bytes_processed            += extraction_end_pos - extraction_start_pos;

  auto deliver                  = [this, &dst_buf, &dst_size, &timestamp, &duration, &packetizer]() {
    return deliver_packet(dst_buf, dst_size, timestamp, duration, &packetizer);
  };

  m_sub_file->setFilePointer(extraction_start_pos);
  track->packet_num++;

  while (1) {
    if (spu_len_valid && ((dst_size >= spu_len) || (m_sub_file->getFilePointer() >= extraction_end_pos))) {
      if (dst_size != spu_len)
        mxdebug_if(s_debug,
                   fmt::format("r_vobsub.cpp: stddeliver spu_len different from dst_size; pts {4} spu_len {0} dst_size {1} curpos {2} endpos {3}\n",
                               spu_len, dst_size, m_sub_file->getFilePointer(), extraction_end_pos, mtx::string::format_timestamp(pts)));
      if (2 < dst_size)
        put_uint16_be(dst_buf, dst_size);

      return deliver();
    }
    if (m_sub_file->read(buf, 4) != 4)
      return deliver();
    while (memcmp(buf, wanted, sizeof(wanted)) != 0) {
      c = m_sub_file->getch();
      if (0 > c)
        return deliver();
      memmove(buf, buf + 1, 3);
      buf[3] = c;
    }
    switch (buf[3]) {
      case 0xb9:                // System End Code
        return deliver();
        break;

      case 0xba:                // Packet start code
        c = m_sub_file->getch();
        if (0 > c)
          return deliver();
        if ((c & 0xc0) == 0x40)
          mpeg_version = 4;
        else if ((c & 0xf0) == 0x20)
          mpeg_version = 2;
        else {
          if (!track->mpeg_version_warning_printed) {
            mxwarn_tid(m_ti.m_fname, track_id,
                       fmt::format(FY("Unsupported MPEG mpeg_version: 0x{0:02x} in packet {1} for timestamp {2}, assuming MPEG2. "
                                      "No further warnings will be printed for this track.\n"),
                                   c, track->packet_num, mtx::string::format_timestamp(timestamp, 3)));
            track->mpeg_version_warning_printed = true;
          }
          mpeg_version = 2;
        }

        if (4 == mpeg_version) {
          if (!m_sub_file->setFilePointer2(9, libebml::seek_current))
            return deliver();
        } else if (2 == mpeg_version) {
          if (!m_sub_file->setFilePointer2(7, libebml::seek_current))
            return deliver();
        } else
          abort();
        break;

      case 0xbd:                // packet
        if (m_sub_file->read(buf, 2) != 2)
          return deliver();

        len = buf[0] << 8 | buf[1];
        idx = m_sub_file->getFilePointer() - extraction_start_pos;
        c   = m_sub_file->getch();

        if (0 > c)
          return deliver();
        if ((c & 0xC0) == 0x40) { // skip STD scale & size
          if (m_sub_file->getch() < 0)
            return deliver();
          c = m_sub_file->getch();
          if (0 > c)
            return deliver();
        }
        if ((c & 0xf0) == 0x20) // System-1 stream timestamp
          abort();
        else if ((c & 0xf0) == 0x30)
          abort();
        else if ((c & 0xc0) == 0x80) { // System-2 (.VOB) stream
          uint32_t pts_flags, hdrlen, dataidx;
          c = m_sub_file->getch();
          if (0 > c)
            return deliver();

          pts_flags = c;
          c         = m_sub_file->getch();
          if (0 > c)
            return deliver();

          hdrlen  = c;
          dataidx = m_sub_file->getFilePointer() - extraction_start_pos + hdrlen;
          if (dataidx > idx + len) {
            mxwarn_fn(m_ti.m_fname, fmt::format(FY("Invalid header length: {0} (total length: {1}, idx: {2}, dataidx: {3})\n"), hdrlen, len, idx, dataidx));
            return deliver();
          }

          if ((pts_flags & 0xc0) == 0x80) {
            if (m_sub_file->read(buf, 5) != 5)
              return deliver();
            if (!(((buf[0] & 0xf0) == 0x20) && (buf[0] & 1) && (buf[2] & 1) && (buf[4] & 1))) {
              mxwarn_fn(m_ti.m_fname, fmt::format(FY("PTS error: 0x{0:02x} {1:02x}{2:02x} {3:02x}{4:02x}\n"), buf[0], buf[1], buf[2], buf[3], buf[4]));
              pts = 0;
            } else
              pts = ((int64_t)((buf[0] & 0x0e) << 29 | buf[1] << 22 | (buf[2] & 0xfe) << 14 | buf[3] << 7 | (buf[4] >> 1))) * 100000 / 9;
          }

          m_sub_file->setFilePointer2(dataidx + extraction_start_pos);
          packet_aid = m_sub_file->getch();
          if (0 > packet_aid) {
            mxwarn_fn(m_ti.m_fname, fmt::format(FY("Bogus aid {0}\n"), packet_aid));
            return deliver();
          }

          packet_size = len - ((unsigned int)m_sub_file->getFilePointer() - extraction_start_pos - idx);
          if (-1 == track->aid)
            track->aid = packet_aid;
          else if (track->aid != packet_aid) {
            // The packet does not belong to the current subtitle stream.
            mxdebug_if(s_debug,
                       fmt::format("vobsub_reader: skipping sub packet with aid {0} (wanted aid: {1}) with size {2} at {3}\n",
                                   packet_aid, track->aid, packet_size, m_sub_file->getFilePointer() - extraction_start_pos));
            m_sub_file->skip(packet_size);
            idx = len;
            break;
          }

          if (mtx::hacks::is_engaged(mtx::hacks::VOBSUB_SUBPIC_STOP_CMDS)) {
            // Space for the stop display command and padding
            dst_buf = (uint8_t *)saferealloc(dst_buf, dst_size + packet_size + 6);
            memset(dst_buf + dst_size + packet_size, 0xff, 6);

          } else
            dst_buf = (uint8_t *)saferealloc(dst_buf, dst_size + packet_size);

          mxdebug_if(s_debug, fmt::format("vobsub_reader: sub packet data: aid: {0}, pts: {1}, packet_size: {2}\n", track->aid, mtx::string::format_timestamp(pts, 3), packet_size));
          if (m_sub_file->read(&dst_buf[dst_size], packet_size) != packet_size) {
            mxwarn(Y("vobsub_reader: sub file read failure"));
            return deliver();
          }
          if (!spu_len_valid) {
            spu_len       = get_uint16_be(dst_buf);
            spu_len_valid = true;
          }

          dst_size        += packet_size;
          track->spu_size += packet_size;
          track->overhead += m_sub_file->getFilePointer() - extraction_start_pos - packet_size;

          idx              = len;
        }
        break;

      case 0xbe:                // Padding
        if (m_sub_file->read(buf, 2) != 2)
          return deliver();
        len = buf[0] << 8 | buf[1];
        if ((0 < len) && !m_sub_file->setFilePointer2(len, libebml::seek_current))
          return deliver();
        break;

      default:
        if ((0xc0 <= buf[3]) && (buf[3] < 0xf0)) {
          // MPEG audio or video
          if (m_sub_file->read(buf, 2) != 2)
            return deliver();
          len = (buf[0] << 8) | buf[1];
          if ((0 < len) && !m_sub_file->setFilePointer2(len, libebml::seek_current))
            return deliver();

        } else {
          mxwarn_fn(m_ti.m_fname, fmt::format(FY("Unknown header 0x{0:02x}{1:02x}{2:02x}{3:02x}\n"), buf[0], buf[1], buf[2], buf[3]));
          return deliver();
        }
    }
  }

  return deliver();
}

file_status_e
vobsub_reader_c::read(generic_packetizer_c *packetizer,
                      bool) {
  vobsub_track_c *track = nullptr;
  uint32_t id;

  for (id = 0; id < tracks.size(); ++id)
    if ((-1 != tracks[id]->ptzr) && (&ptzr(tracks[id]->ptzr) == packetizer)) {
      track = tracks[id];
      break;
    }

  if (!track)
    return FILE_STATUS_DONE;

  if (track->idx >= track->entries.size())
    return flush_packetizers();

  extract_one_spu_packet(id);
  track->idx++;

  return track->idx >= track->entries.size() ? flush_packetizers() : FILE_STATUS_MOREDATA;
}

int64_t
vobsub_reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
vobsub_reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

void
vobsub_reader_c::identify() {
  size_t i;

  id_result_container();

  for (i = 0; i < tracks.size(); i++) {
    auto info = mtx::id::info_c{};

    info.add(mtx::id::language, tracks[i]->language.get_iso639_alpha_3_code());

    id_result_track(i, ID_RESULT_TRACK_SUBTITLES, codec_c::get_name(codec_c::type_e::S_VOBSUB, "VobSub"), info.get());
  }
}

file_status_e
vobsub_reader_c::flush_packetizers() {
  for (auto track : tracks)
    if (track->ptzr != -1)
      ptzr(track->ptzr).flush();

  return FILE_STATUS_DONE;
}

void
vobsub_reader_c::add_available_track_ids() {
  add_available_track_id_range(tracks.size());
}
