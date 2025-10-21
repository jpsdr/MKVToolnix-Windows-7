/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the packet structure

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace libmatroska {
  class KaxBlock;
  class KaxBlockBlob;
  class KaxCluster;
}

class generic_packetizer_c;
class track_statistics_c;

class packet_extension_c {
public:
  enum packet_extension_type_e {
    MULTIPLE_TIMESTAMPS,
    SUBTITLE_NUMBER,
    BEFORE_ADDING_TO_CLUSTER_CB,
  };

public:
  packet_extension_c() {
  }

  virtual ~packet_extension_c() {
  }

  virtual packet_extension_type_e get_type() const = 0;
};
using packet_extension_cptr = std::shared_ptr<packet_extension_c>;

struct block_add_t {
  memory_cptr data;
  std::optional<uint64_t> id;

  block_add_t() = default;

  block_add_t(memory_cptr const &p_data)
    : data{p_data}
    , id{std::nullopt}
  {
  }

  block_add_t(memory_cptr const &p_data, uint64_t p_id)
    : data{p_data}
    , id{p_id}
  {
  }

  block_add_t(block_add_t const &) = default;
  block_add_t(block_add_t &&) = default;
};

struct packet_t {
  memory_cptr data;
  std::vector<block_add_t> data_adds;
  memory_cptr codec_state;

  libmatroska::KaxBlockBlob *group;
  libmatroska::KaxBlock *block;
  libmatroska::KaxCluster *cluster;
  int ref_priority;
  int64_t timestamp, bref, fref, duration, assigned_timestamp;
  int64_t timestamp_before_factory;
  int64_t unmodified_assigned_timestamp, unmodified_duration;
  std::optional<uint64_t> uncompressed_size;
  timestamp_c discard_padding, output_order_timestamp;
  bool duration_mandatory, superseeded, gap_following, factory_applied;
  std::optional<bool> key_flag, discardable_flag;
  generic_packetizer_c *source;

  std::vector<packet_extension_cptr> extensions;

  packet_t()
    : group{}
    , block{}
    , cluster{}
    , ref_priority{}
    , timestamp(-1)
    , bref(-1)
    , fref(-1)
    , duration(-1)
    , assigned_timestamp{}
    , timestamp_before_factory{}
    , unmodified_assigned_timestamp{}
    , unmodified_duration{}
    , discard_padding{}
    , duration_mandatory{}
    , superseeded{}
    , gap_following{}
    , factory_applied{}
    , source{}
  {
  }

  packet_t(memory_cptr const &p_memory,
           int64_t p_timestamp = -1,
           int64_t p_duration = -1,
           int64_t p_bref     = -1,
           int64_t p_fref     = -1)
    : data(p_memory)
    , group{}
    , block{}
    , cluster{}
    , ref_priority{}
    , timestamp(p_timestamp)
    , bref(p_bref)
    , fref(p_fref)
    , duration(p_duration)
    , assigned_timestamp{}
    , timestamp_before_factory{}
    , unmodified_assigned_timestamp{}
    , unmodified_duration{}
    , discard_padding{}
    , duration_mandatory{}
    , superseeded{}
    , gap_following{}
    , factory_applied{}
    , source{}
  {
  }

  packet_t(memory_c *n_memory,
           int64_t p_timestamp = -1,
           int64_t p_duration = -1,
           int64_t p_bref     = -1,
           int64_t p_fref     = -1)
    : data(memory_cptr(n_memory))
    , group{}
    , block{}
    , cluster{}
    , ref_priority{}
    , timestamp(p_timestamp)
    , bref(p_bref)
    , fref(p_fref)
    , duration(p_duration)
    , assigned_timestamp{}
    , timestamp_before_factory{}
    , unmodified_assigned_timestamp{}
    , unmodified_duration{}
    , discard_padding{}
    , duration_mandatory{}
    , superseeded{}
    , gap_following{}
    , factory_applied{}
    , source{}
  {
  }

  ~packet_t() {
  }

  bool
  has_timestamp()
    const {
    return 0 <= timestamp;
  }

  bool
  has_bref()
    const {
    return 0 <= bref;
  }

  bool
  has_fref()
    const {
    return 0 <= fref;
  }

  bool
  has_duration()
    const {
    return 0 <= duration;
  }

  bool
  has_discard_padding()
    const {
    return discard_padding.valid();
  }

  int64_t
  get_duration()
    const {
    return has_duration() ? duration : 0;
  }

  int64_t
  get_unmodified_duration()
    const {
    return has_duration() ? unmodified_duration : 0;
  }

  void
  force_key_frame() {
    bref     = -1;
    fref     = -1;
    key_flag = true;
  }

  bool
  is_key_frame()
    const {
    return (key_flag && *key_flag) || (!has_bref() && !has_fref());
  }

  bool
  is_p_frame()
    const {
    return (has_bref() || has_fref()) && (has_bref() != has_fref());
  }

  bool
  is_b_frame()
    const {
    return has_bref() && has_fref();
  }

  packet_extension_c *
  find_extension(packet_extension_c::packet_extension_type_e type)
    const {
    for (auto &extension : extensions)
      if (extension->get_type() == type)
        return extension.get();

    return nullptr;
  }

  void add_extensions(std::vector<packet_extension_cptr> const &new_extensions);

  void normalize_timestamps();

  void account(track_statistics_c &statistics, int64_t timestamp_offset);
  uint64_t calculate_uncompressed_size();
};
using packet_cptr = std::shared_ptr<packet_t>;
