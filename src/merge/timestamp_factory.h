/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the timestamp factory

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/packet.h"
#include "merge/track_info.h"

enum timestamp_factory_application_e {
  TFA_AUTOMATIC,
  TFA_IMMEDIATE,
  TFA_SHORT_QUEUEING,
  TFA_FULL_QUEUEING
};

class timestamp_range_c {
public:
  uint64_t start_frame, end_frame;
  double fps, base_timestamp;

  bool operator <(const timestamp_range_c &cmp) const {
    return start_frame < cmp.start_frame;
  }
};

class timestamp_duration_c {
public:
  double fps;
  int64_t duration;
  bool is_gap;
};

class timestamp_factory_c;
using timestamp_factory_cptr = std::shared_ptr<timestamp_factory_c>;

class timestamp_factory_c {
protected:
  std::string m_file_name, m_source_name;
  int64_t m_tid;
  int m_version;
  bool m_preserve_duration;
  debugging_option_c m_debug;

public:
  timestamp_factory_c(const std::string &file_name,
                     const std::string &source_name,
                     int64_t tid,
                     int version)
    : m_file_name(file_name)
    , m_source_name(source_name)
    , m_tid(tid)
    , m_version(version)
    , m_preserve_duration(false)
    , m_debug{"timestamp_factory"}
  {
  }
  virtual ~timestamp_factory_c() = default;

  virtual void parse(mm_io_c &) = 0;
  virtual void parse_json(nlohmann::json &) = 0;
  virtual bool get_next(packet_t &packet) {
    // No gap is following!
    packet.assigned_timestamp = packet.timestamp;
    return false;
  }
  virtual double get_default_duration(double proposal) {
    return proposal;
  }

  virtual bool contains_gap() {
    return false;
  }

  virtual void set_preserve_duration(bool preserve_duration) {
    m_preserve_duration = preserve_duration;
  }

  static timestamp_factory_cptr create(const std::string &file_name, const std::string &source_name, int64_t tid);
  static timestamp_factory_cptr create_for_version(std::string const &file_name, std::string const &source_name, int64_t tid, int version);
  static timestamp_factory_cptr create_fps_factory(int64_t default_duration, timestamp_sync_t const &tcsync);
  static timestamp_factory_cptr create_from_json(std::string const &file_name, std::string const &source_name, int64_t tid, mm_io_c &in);
};

class timestamp_factory_v1_c: public timestamp_factory_c {
protected:
  std::vector<timestamp_range_c> m_ranges;
  uint32_t m_current_range;
  uint64_t m_frameno;
  double m_default_fps;

public:
  timestamp_factory_v1_c(const std::string &file_name,
                        const std::string &source_name,
                        int64_t tid)
    : timestamp_factory_c(file_name, source_name, tid, 1)
    , m_current_range(0)
    , m_frameno(0)
    , m_default_fps(0.0)
  {
  }
  virtual ~timestamp_factory_v1_c() = default;

  virtual void parse(mm_io_c &in) override;
  virtual void parse_json(nlohmann::json &) override;
  virtual bool get_next(packet_t &packet) override;
  virtual double get_default_duration(double proposal) override {
    return 0.0 != m_default_fps ? 1000000000.0 / m_default_fps : proposal;
  }

protected:
  virtual int64_t get_at(uint64_t frame);
  virtual void postprocess_parsed_ranges();
};

class timestamp_factory_v2_c: public timestamp_factory_c {
protected:
  std::vector<int64_t> m_timestamps, m_durations;
  int64_t m_frameno;
  double m_default_duration;
  bool m_warning_printed;

public:
  timestamp_factory_v2_c(const std::string &file_name,
                        const std::string &source_name,
                        int64_t tid, int version)
    : timestamp_factory_c(file_name, source_name, tid, version)
    , m_frameno(0)
    , m_default_duration(0)
    , m_warning_printed(false)
  {
  }
  virtual ~timestamp_factory_v2_c() = default;

  virtual void parse(mm_io_c &in) override;
  virtual void parse_json(nlohmann::json &) override;
  virtual bool get_next(packet_t &packet) override;
  virtual double get_default_duration(double proposal) override {
    return m_default_duration != 0 ? m_default_duration : proposal;
  }

protected:
  virtual void postprocess_parsed_timestamps();
};

class timestamp_factory_v3_c: public timestamp_factory_c {
protected:
  std::vector<timestamp_duration_c> m_durations;
  size_t m_current_duration;
  int64_t m_current_timestamp;
  int64_t m_current_offset;
  double m_default_fps;

public:
  timestamp_factory_v3_c(const std::string &file_name,
                        const std::string &source_name,
                        int64_t tid)
    : timestamp_factory_c(file_name, source_name, tid, 3)
    , m_current_duration(0)
    , m_current_timestamp(0)
    , m_current_offset(0)
    , m_default_fps(0.0)
  {
  }
  virtual void parse(mm_io_c &in) override;
  virtual void parse_json(nlohmann::json &) override;
  virtual bool get_next(packet_t &packet) override;
  virtual bool contains_gap() override {
    return true;
  }

private:
  std::optional<timestamp_duration_c> parse_normal_line(std::string const &line);
};

class forced_default_duration_timestamp_factory_c: public timestamp_factory_c {
protected:
  int64_t m_default_duration{}, m_frameno{};
  timestamp_sync_t m_tcsync{};

public:
  forced_default_duration_timestamp_factory_c(int64_t default_duration,
                                             timestamp_sync_t const &tcsync,
                                             const std::string &source_name,
                                             int64_t tid)
    : timestamp_factory_c{"", source_name, tid, 1}
    , m_default_duration{mtx::to_int(tcsync.factor * default_duration)}
    , m_tcsync{tcsync}
  {
  }

  virtual ~forced_default_duration_timestamp_factory_c() = default;

  virtual void parse(mm_io_c &) override {
  }
  virtual void parse_json(nlohmann::json &) override {
  }

  virtual bool get_next(packet_t &packet) override;
  virtual double get_default_duration(double) override {
    return m_default_duration;
  }
};
