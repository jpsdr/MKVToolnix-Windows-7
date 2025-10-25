/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the packet extensions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <deque>

#include "merge/packet.h"

class multiple_timestamps_packet_extension_c: public packet_extension_c {
protected:
  std::deque<int64_t> m_timestamps;
  std::deque<int64_t> m_positions;

public:
  multiple_timestamps_packet_extension_c() {
  }

  virtual ~multiple_timestamps_packet_extension_c() {
  }

  virtual packet_extension_type_e get_type() const {
    return MULTIPLE_TIMESTAMPS;
  }

  inline void add(int64_t timestamp, int64_t position) {
    m_timestamps.push_back(timestamp);
    m_positions.push_back(position);
  }

  inline bool empty() {
    return m_timestamps.empty();
  }

  inline bool get_next(int64_t &timestamp, int64_t &position) {
    if (m_timestamps.empty())
      return false;

    timestamp = m_timestamps.front();
    position = m_positions.front();

    m_timestamps.pop_front();
    m_positions.pop_front();

    return true;
  }
};

using multiple_timestamps_packet_extension_cptr = std::shared_ptr<multiple_timestamps_packet_extension_c>;

class subtitle_number_packet_extension_c: public packet_extension_c {
private:
  unsigned int m_number;

public:
  subtitle_number_packet_extension_c(unsigned int number)
    : m_number(number)
  {
  }

  virtual packet_extension_type_e get_type() const {
    return SUBTITLE_NUMBER;
  }

  unsigned int get_number() {
    return m_number;
  }
};

using subtitle_number_packet_extension_cptr = std::shared_ptr<subtitle_number_packet_extension_c>;

class before_adding_to_cluster_cb_packet_extension_c: public packet_extension_c {
public:
  using callback_t = std::function<void(packet_cptr const &, int64_t)>;

private:
  callback_t m_callback;

public:
  before_adding_to_cluster_cb_packet_extension_c(callback_t const &callback)
    : m_callback{callback}
  {
  }

  virtual packet_extension_type_e get_type() const {
    return BEFORE_ADDING_TO_CLUSTER_CB;
  }

  callback_t get_callback() const {
    return m_callback;
  }
};

using before_adding_to_cluster_cb_packet_extension_cptr = std::shared_ptr<before_adding_to_cluster_cb_packet_extension_c>;
