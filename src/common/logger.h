/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/strings/utf8.h"

namespace mtx::log {

class target_c;
using target_cptr = std::shared_ptr<target_c>;

class target_c {
private:
  int64_t m_log_start;

public:
  target_c();
  virtual ~target_c() = default;

  void log(std::string const &message) {
    log_line(message);
  }

protected:
  virtual std::string format_line(std::string const &message);
  virtual void log_line(std::string const &message) = 0;

  static target_cptr s_default_logger;

public:
  static target_c &get_default_logger();
  static void set_default_logger(target_cptr const &logger);
  static int64_t runtime();
};

class lifetime_logger_c {
private:
  std::string const m_comment;
  int64_t m_start{};

public:
  lifetime_logger_c(std::string const &comment);
  ~lifetime_logger_c();
};

class file_target_c: public target_c {
private:
  boost::filesystem::path m_file_name;

public:
  file_target_c(boost::filesystem::path file_name);
  virtual ~file_target_c() = default;

protected:
  virtual void log_line(std::string const &message) override;
};

class stderr_target_c: public target_c {
public:
  stderr_target_c();
  virtual ~stderr_target_c() = default;

protected:
  virtual void log_line(std::string const &message) override;
};

template<typename T>
target_c &
operator <<(target_c &logger,
            T const &message) {
  logger.log(to_utf8(message));
  return logger;
}

void init();

}

#define log_current_location() mtx::log::target_c::get_default_logger() << fmt::format("Current file, line, function: {0}:{1} in {2}", __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define log_scope_lifetime()   mtx::log::lifetime_logger_c mtx_scope_lifetime_logger{fmt::format("{0}:{1} in {2}", __FILE__, __LINE__, __PRETTY_FUNCTION__)}
#define log_it(arg)            mtx::log::target_c::get_default_logger() << (arg)
