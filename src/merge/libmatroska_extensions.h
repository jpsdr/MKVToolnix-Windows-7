/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlVersion.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSemantic.h>

class kax_cluster_c: public libmatroska::KaxCluster {
public:
  kax_cluster_c(): libmatroska::KaxCluster() {
    PreviousTimecode = 0;
  }
  virtual ~kax_cluster_c();

  void delete_non_blocks();

  void set_min_timestamp(int64_t min_timestamp) {
    MinTimecode = min_timestamp;
  }
  void set_max_timestamp(int64_t max_timestamp) {
    MaxTimecode = max_timestamp;
  }
};

class kax_reference_block_c: public libmatroska::KaxReferenceBlock {
protected:
  int64_t m_value;

public:
  kax_reference_block_c();

  void set_value(int64_t value) {
    m_value     = value;
#if LIBEBML_VERSION < 0x000800
    bValueIsSet = true;
#else
    SetValueIsSet();
#endif
  }

  virtual filepos_t UpdateSize(bool bSaveDefault, bool bForceRender);
};

class kax_block_group_c: public libmatroska::KaxBlockGroup {
public:
  kax_block_group_c(): libmatroska::KaxBlockGroup() {
  }

  bool add_frame(const libmatroska::KaxTrackEntry &track, uint64_t timestamp, libmatroska::DataBuffer &buffer, int64_t past_block, int64_t forw_block, libmatroska::LacingType lacing);
};

class kax_block_blob_c: public libmatroska::KaxBlockBlob {
public:
  kax_block_blob_c(libmatroska::BlockBlobType type): libmatroska::KaxBlockBlob(type) {
  }

  bool add_frame_auto(const libmatroska::KaxTrackEntry &track, uint64_t timestamp, libmatroska::DataBuffer &buffer, libmatroska::LacingType lacing,
                      int64_t past_block, int64_t forw_block, std::optional<bool> key_flag, std::optional<bool> discardable_flag);
  void set_block_duration(uint64_t time_length);
  bool replace_simple_by_group();
};
using kax_block_blob_cptr = std::shared_ptr<kax_block_blob_c>;

class kax_cues_position_dummy_c: public libmatroska::KaxCues {
public:
  kax_cues_position_dummy_c()
    : libmatroska::KaxCues{}
  {
  }

  filepos_t Render(libebml::IOCallback &output) {
    return libebml::EbmlElement::Render(output, true, false, true);
  }
};

class kax_cues_with_cleanup_c: public libmatroska::KaxCues {
public:
  kax_cues_with_cleanup_c();
  virtual ~kax_cues_with_cleanup_c();
};

class kax_block_add_id_c: public libmatroska::KaxBlockAddID {
public:
  kax_block_add_id_c(bool always_write)
    : libmatroska::KaxBlockAddID{}
  {
    if (always_write)
      ForceNoDefault();
  }
};
