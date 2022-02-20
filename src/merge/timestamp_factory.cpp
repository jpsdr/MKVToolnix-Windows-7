/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the timestamp factory

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modifications by Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "merge/timestamp_factory.h"

timestamp_factory_cptr
timestamp_factory_c::create(std::string const &file_name,
                            std::string const &source_name,
                            int64_t tid) {
  if (file_name.empty())
    return timestamp_factory_cptr{};

  mm_io_cptr in;
  try {
    in = std::make_shared<mm_text_io_c>(std::make_shared<mm_file_io_c>(file_name));
  } catch(...) {
    mxerror(fmt::format(Y("The timestamp file '{0}' could not be opened for reading.\n"), file_name));
  }

  std::string line;
  int version = -1;
  bool ok     = in->getline2(line);
  if (ok) {
    auto format_line_re = QRegularExpression{"^# *time(?:code|stamp) *format v(\\d+).*"};
    auto matches        = format_line_re.match(Q(line));

    if (matches.hasMatch())
      ok = mtx::string::parse_number(to_utf8(matches.captured(1)), version);
    else
      ok = false;
  }

  if (!ok)
    mxerror(fmt::format(Y("The timestamp file '{0}' contains an unsupported/unrecognized format line. The very first line must look like '# timestamp format v1'.\n"),
                        file_name));

  timestamp_factory_c *factory = nullptr; // avoid gcc warning
  if (1 == version)
    factory = new timestamp_factory_v1_c(file_name, source_name, tid);

  else if ((2 == version) || (4 == version))
    factory = new timestamp_factory_v2_c(file_name, source_name, tid, version);

  else if (3 == version)
    factory = new timestamp_factory_v3_c(file_name, source_name, tid);

  else
    mxerror(fmt::format(Y("The timestamp file '{0}' contains an unsupported/unrecognized format (version {1}).\n"), file_name, version));

  factory->parse(*in);

  return timestamp_factory_cptr(factory);
}

timestamp_factory_cptr
timestamp_factory_c::create_fps_factory(int64_t default_duration,
                                        timestamp_sync_t const &tcsync) {
  return timestamp_factory_cptr{ new forced_default_duration_timestamp_factory_c{default_duration, tcsync, "", 1} };
}

void
timestamp_factory_v1_c::parse(mm_io_c &in) {
  std::string line;
  timestamp_range_c t;
  std::vector<timestamp_range_c>::iterator iit;
  std::vector<timestamp_range_c>::const_iterator pit;

  int line_no = 1;
  do {
    if (!in.getline2(line))
      mxerror(fmt::format(Y("The timestamp file '{0}' does not contain a valid 'Assume' line with the default number of frames per second.\n"), m_file_name));
    line_no++;
    mtx::string::strip(line);
    if (!line.empty() && ('#' != line[0]))
      break;
  } while (true);

  if (!balg::istarts_with(line, "assume "))
    mxerror(fmt::format(Y("The timestamp file '{0}' does not contain a valid 'Assume' line with the default number of frames per second.\n"), m_file_name));

  line.erase(0, 6);
  mtx::string::strip(line);

  if (!mtx::string::parse_number(line.c_str(), m_default_fps))
    mxerror(fmt::format(Y("The timestamp file '{0}' does not contain a valid 'Assume' line with the default number of frames per second.\n"), m_file_name));

  while (in.getline2(line)) {
    line_no++;
    mtx::string::strip(line, true);
    if (line.empty() || ('#' == line[0]))
      continue;

    auto parts = mtx::string::split(line, ",", 3);
    if (   (parts.size() != 3)
        || !mtx::string::parse_number(parts[0], t.start_frame)
        || !mtx::string::parse_number(parts[1], t.end_frame)
        || !mtx::string::parse_number(parts[2], t.fps)) {
      mxwarn(fmt::format(Y("Line {0} of the timestamp file '{1}' could not be parsed.\n"), line_no, m_file_name));
      continue;
    }

    if ((t.fps <= 0) || (t.end_frame < t.start_frame)) {
      mxwarn(fmt::format(Y("Line {0} of the timestamp file '{1}' contains inconsistent data (e.g. the start frame number is bigger than the end frame "
                           "number, or some values are smaller than zero).\n"), line_no, m_file_name));
      continue;
    }

    m_ranges.push_back(t);
  }

  mxdebug_if(m_debug, fmt::format("ext_timestamps: Version 1, default fps {0}, {1} entries.\n", m_default_fps, m_ranges.size()));

  if (m_ranges.size() == 0)
    t.start_frame = 0;
  else {
    std::sort(m_ranges.begin(), m_ranges.end());
    bool done;
    do {
      done = true;
      iit  = m_ranges.begin();
      size_t i;
      for (i = 0; i < (m_ranges.size() - 1); i++) {
        iit++;
        if (m_ranges[i].end_frame < (m_ranges[i + 1].start_frame - 1)) {
          t.start_frame = m_ranges[i].end_frame + 1;
          t.end_frame = m_ranges[i + 1].start_frame - 1;
          t.fps = m_default_fps;
          m_ranges.insert(iit, t);
          done = false;
          break;
        }
      }
    } while (!done);
    if (m_ranges[0].start_frame != 0) {
      t.start_frame = 0;
      t.end_frame = m_ranges[0].start_frame - 1;
      t.fps = m_default_fps;
      m_ranges.insert(m_ranges.begin(), t);
    }
    t.start_frame = m_ranges[m_ranges.size() - 1].end_frame + 1;
  }

  t.end_frame = 0xfffffffffffffffll;
  t.fps       = m_default_fps;
  m_ranges.push_back(t);

  m_ranges[0].base_timestamp = 0.0;
  pit = m_ranges.begin();
  for (iit = m_ranges.begin() + 1; iit < m_ranges.end(); iit++, pit++)
    iit->base_timestamp = pit->base_timestamp + ((double)pit->end_frame - (double)pit->start_frame + 1) * 1000000000.0 / pit->fps;

  for (iit = m_ranges.begin(); iit < m_ranges.end(); iit++)
    mxdebug_if(m_debug, fmt::format("ranges: entry {0} -> {1} at {2} with {3}\n", iit->start_frame, iit->end_frame, iit->fps, iit->base_timestamp));
}

bool
timestamp_factory_v1_c::get_next(packet_t &packet) {
  packet.assigned_timestamp = get_at(m_frameno);
  if (!m_preserve_duration || (0 >= packet.duration))
    packet.duration = get_at(m_frameno + 1) - packet.assigned_timestamp;

  m_frameno++;
  if ((m_frameno > m_ranges[m_current_range].end_frame) && (m_current_range < (m_ranges.size() - 1)))
    m_current_range++;

  mxdebug_if(m_debug, fmt::format("ext_timestamps v1: tc {0} dur {1} for {2}\n", packet.assigned_timestamp, packet.duration, m_frameno - 1));

  return false;
}

int64_t
timestamp_factory_v1_c::get_at(uint64_t frame) {
  timestamp_range_c *t = &m_ranges[m_current_range];
  if ((frame > t->end_frame) && (m_current_range < (m_ranges.size() - 1)))
    t = &m_ranges[m_current_range + 1];

  return (int64_t)(t->base_timestamp + 1000000000.0 * (frame - t->start_frame) / t->fps);
}

void
timestamp_factory_v2_c::parse(mm_io_c &in) {
  std::string line;
  std::map<int64_t, int64_t> dur_map;

  int64_t dur_sum          = 0;
  int line_no              = 0;
  double previous_timestamp = 0;

  while (in.getline2(line)) {
    line_no++;
    mtx::string::strip(line);
    if ((line.length() == 0) || (line[0] == '#'))
      continue;

    double timestamp;
    if (!mtx::string::parse_number(line.c_str(), timestamp))
      mxerror(fmt::format(Y("The line {0} of the timestamp file '{1}' does not contain a valid floating point number.\n"), line_no, m_file_name));

    if ((2 == m_version) && (timestamp < previous_timestamp))
      mxerror(fmt::format(Y("The timestamp v2 file '{0}' contains timestamps that are not ordered. "
                            "Due to a bug in mkvmerge versions up to and including v1.5.0 this was necessary "
                            "if the track to which the timestamp file was applied contained B frames. "
                            "Starting with v1.5.1 mkvmerge now handles this correctly, and the timestamps in the timestamp file must be ordered normally. "
                            "For example, the frame sequence 'IPBBP...' at 25 FPS requires a timestamp file with "
                            "the first timestamps being '0', '40', '80', '120' etc and. not '0', '120', '40', '80' etc.\n\n"
                            "If you really have to specify non-sorted timestamps then use the timestamp format v4. "
                            "It is identical to format v2 but allows non-sorted timestamps.\n"),
                          in.get_file_name()));

    previous_timestamp = timestamp;
    m_timestamps.push_back((int64_t)(timestamp * 1000000));
    if (m_timestamps.size() > 1) {
      int64_t duration = m_timestamps[m_timestamps.size() - 1] - m_timestamps[m_timestamps.size() - 2];
      if (dur_map.find(duration) == dur_map.end())
        dur_map[duration] = 1;
      else
        dur_map[duration] = dur_map[duration] + 1;
      dur_sum += duration;
      m_durations.push_back(duration);
    }
  }

  if (m_timestamps.empty())
    mxerror(fmt::format(Y("The timestamp file '{0}' does not contain any valid entry.\n"), m_file_name));

  if (m_debug) {
    mxdebug("Absolute probablities with maximum in separate line:\n");
    mxdebug("Duration  | Absolute probability\n");
    mxdebug("----------+---------------------\n");
  }

  dur_sum = -1;
  for (auto entry : dur_map) {
    if ((0 > dur_sum) || (dur_map[dur_sum] < entry.second))
      dur_sum = entry.first;
    mxdebug_if(m_debug, fmt::format("{0: 9} | {1: 9}\n", entry.first, entry.second));
  }

  mxdebug_if(m_debug, "Max-------+---------------------\n");
  mxdebug_if(m_debug, fmt::format("{0: 9} | {1: 9}\n", dur_sum, dur_map[dur_sum]));

  if (0 < dur_sum)
    m_default_duration = dur_sum;

  m_durations.push_back(dur_sum);
}

bool
timestamp_factory_v2_c::get_next(packet_t &packet) {
  if ((static_cast<size_t>(m_frameno) >= m_timestamps.size()) && !m_warning_printed) {
    mxwarn_tid(m_source_name, m_tid,
               fmt::format(Y("The number of external timestamps {0} is smaller than the number of frames in this track. "
                             "The remaining frames of this track might not be timestamped the way you intended them to be. mkvmerge might even crash.\n"),
                           m_timestamps.size()));
    m_warning_printed = true;

    if (m_timestamps.empty()) {
      packet.assigned_timestamp = 0;
      if (!m_preserve_duration || (0 >= packet.duration))
        packet.duration = 0;

    } else {
      packet.assigned_timestamp = m_timestamps.back();
      if (!m_preserve_duration || (0 >= packet.duration))
        packet.duration = m_timestamps.back();
    }

    return false;
  }

  packet.assigned_timestamp = m_timestamps[m_frameno];
  if (!m_preserve_duration || (0 >= packet.duration))
    packet.duration = m_durations[m_frameno];
  m_frameno++;

  return false;
}

void
timestamp_factory_v3_c::parse(mm_io_c &in) {
  std::string line;
  timestamp_duration_c t;
  std::vector<timestamp_duration_c>::iterator iit;

  std::string err_msg_assume = fmt::format(Y("The timestamp file '{0}' does not contain a valid 'Assume' line with the default number of frames per second.\n"), m_file_name);

  int line_no = 1;
  do {
    if (!in.getline2(line))
      mxerror(err_msg_assume);
    line_no++;
    mtx::string::strip(line);
    if ((line.length() != 0) && (line[0] != '#'))
      break;
  } while (true);

  if (!balg::istarts_with(line, "assume "))
    mxerror(err_msg_assume);
  line.erase(0, 6);
  mtx::string::strip(line);

  if (!mtx::string::parse_number(line.c_str(), m_default_fps))
    mxerror(err_msg_assume);

  while (in.getline2(line)) {
    line_no++;
    mtx::string::strip(line, true);
    if ((line.length() == 0) || (line[0] == '#'))
      continue;

    double dur;
    if (balg::istarts_with(line, "gap,")) {
      line.erase(0, 4);
      mtx::string::strip(line);

      t.is_gap = true;
      t.fps    = m_default_fps;

      if (!mtx::string::parse_number(line.c_str(), dur))
        mxerror(fmt::format(Y("The timestamp file '{0}' does not contain a valid 'Gap' line with the duration of the gap.\n"), m_file_name));
      t.duration = (int64_t)(1000000000.0 * dur);

    } else {
      t.is_gap = false;
      auto parts = mtx::string::split(line, ",");

      if ((1 == parts.size()) && mtx::string::parse_number(parts[0], dur))
        t.fps = m_default_fps;

      else if ((2 != parts.size()) || !mtx::string::parse_number(parts[1], t.fps)) {
        mxwarn(fmt::format(Y("Line {0} of the timestamp file '{1}' could not be parsed.\n"), line_no, m_file_name));
        continue;
      }
      t.duration = (int64_t)(1000000000.0 * dur);
    }

    if ((t.fps < 0) || (t.duration <= 0)) {
      mxwarn(fmt::format(Y("Line {0} of the timestamp file '{1}' contains inconsistent data (e.g. the duration or the FPS are smaller than zero).\n"),
                         line_no, m_file_name));
      continue;
    }

    m_durations.push_back(t);
  }

  mxdebug_if(m_debug, fmt::format("ext_timestamps: Version 3, default fps {0}, {1} entries.\n", m_default_fps, m_durations.size()));

  if (m_durations.size() == 0)
    mxwarn(fmt::format(Y("The timestamp file '{0}' does not contain any valid entry.\n"), m_file_name));

  t.duration = 0xfffffffffffffffll;
  t.is_gap   = false;
  t.fps      = m_default_fps;
  m_durations.push_back(t);

  for (iit = m_durations.begin(); iit < m_durations.end(); iit++)
    mxdebug_if(m_debug, fmt::format("durations:{0} entry for {1} with {2} FPS\n", iit->is_gap ? " gap" : "", iit->duration, iit->fps));
}

bool
timestamp_factory_v3_c::get_next(packet_t &packet) {
  bool result = false;

  if (m_durations[m_current_duration].is_gap) {
    // find the next non-gap
    size_t duration_index = m_current_duration;
    while (m_durations[duration_index].is_gap || (0 == m_durations[duration_index].duration)) {
      m_current_offset += m_durations[duration_index].duration;
      duration_index++;
    }
    m_current_duration = duration_index;
    result             = true;
    // yes, there is a gap before this frame
  }

  packet.assigned_timestamp = m_current_offset + m_current_timestamp;
  // If default_fps is 0 then the duration is unchanged, usefull for audio.
  if (m_durations[m_current_duration].fps && (!m_preserve_duration || (0 >= packet.duration)))
    packet.duration = (int64_t)(1000000000.0 / m_durations[m_current_duration].fps);

  m_current_timestamp += packet.duration;

  if (m_current_timestamp >= m_durations[m_current_duration].duration) {
    m_current_offset   += m_durations[m_current_duration].duration;
    m_current_timestamp  = 0;
    m_current_duration++;
  }

  mxdebug_if(m_debug, fmt::format("ext_timestamps v3: tc {0} dur {1}\n", packet.assigned_timestamp, packet.duration));

  return result;
}

bool
forced_default_duration_timestamp_factory_c::get_next(packet_t &packet) {
  packet.assigned_timestamp = m_frameno * m_default_duration + m_tcsync.displacement;
  packet.duration           = m_default_duration;
  ++m_frameno;

  return false;
}
