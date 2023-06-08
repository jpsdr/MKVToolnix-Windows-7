#pragma once

#include "common/common_pch.h"

namespace mtx::gui::Util {

class ProcessX : public mtx::exception {
protected:
  std::string m_message;
public:
  explicit ProcessX(std::string const &message) : m_message{message} { }
  virtual ~ProcessX() throw() { }

  virtual char const *what() const throw() {
    return m_message.c_str();
  }
};

}
