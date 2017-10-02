/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/strings/utf8.h"

namespace mtx { namespace log {

class target_c;
using target_cptr = std::shared_ptr<target_c>;

class target_c {
private:
  int64_t m_log_start;

public:
  target_c();
  virtual ~target_c();

  void log(std::string const &message) {
    log_line(message);
  }
  void log(boost::format const &message) {
    log_line(message.str());
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

class file_target_c: public target_c {
private:
  bfs::path m_file_name;

public:
  file_target_c(bfs::path const &file_name);
  virtual ~file_target_c();

protected:
  virtual void log_line(std::string const &message) override;
};

class stderr_target_c: public target_c {
public:
  stderr_target_c();
  virtual ~stderr_target_c();

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

}}

#define log_current_location() mtx::log::target_c::get_default_logger() << (boost::format("Current file, line, function: %1%:%2% in %3%") % __FILE__ % __LINE__ % __PRETTY_FUNCTION__)
#define log_it(arg)            mtx::log::target_c::get_default_logger() << (arg)
