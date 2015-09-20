/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_LOGGER_H
#define MTX_COMMON_LOGGER_H

#include "common/common_pch.h"

#include "common/strings/utf8.h"

class logger_c;
using logger_cptr = std::shared_ptr<logger_c>;

class logger_c {
private:
  bfs::path m_file_name;
  int64_t m_log_start;

public:
  logger_c(bfs::path const &file_name);
  virtual ~logger_c();

  void log(std::string const &message) {
    log_line(message);
  }
  void log(boost::format const &message) {
    log_line(message.str());
  }

protected:
  virtual std::string format_line(std::string const &message);
  virtual void log_line(std::string const &message);

  static logger_cptr s_default_logger;

public:
  static logger_c &get_default_logger();
  static void set_default_logger(logger_cptr logger);
  static int64_t runtime();
};

template<typename T>
logger_c &
operator <<(logger_c &logger,
            T const &message) {
  logger.log(to_utf8(message));
  return logger;
}

#define log_current_location() logger_c::get_default_logger() << (boost::format("Current file & line: %1%:%2%") % __FILE__ % __LINE__)
#define log_it(arg)            logger_c::get_default_logger() << (arg)

#endif // MTX_COMMON_LOGGER_H
