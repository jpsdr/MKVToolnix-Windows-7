/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlVersion.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSemantic.h>

#include "common/ebml.h"

class kax_cluster_c: public libmatroska::KaxCluster {
public:
  kax_cluster_c(): libmatroska::KaxCluster() {
#if LIBMATROSKA_VERSION >= 0x020000
    PreviousTimestamp = 0;
#else
    PreviousTimecode = 0;
#endif

  }
  virtual ~kax_cluster_c();

  void delete_non_blocks();

#if LIBMATROSKA_VERSION >= 0x020000
  void set_min_timestamp(int64_t min_timestamp) {
    MinTimestamp = min_timestamp;
  }
  void set_max_timestamp(int64_t max_timestamp) {
    MaxTimestamp = max_timestamp;
  }
#else
  void set_min_timestamp(int64_t min_timestamp) {
    MinTimecode = min_timestamp;
  }
  void set_max_timestamp(int64_t max_timestamp) {
    MaxTimecode = max_timestamp;
  }
#endif
};

class kax_reference_block_c: public libmatroska::KaxReferenceBlock {
protected:
  int64_t m_value;

public:
  kax_reference_block_c();

  void set_value(int64_t value) {
    m_value = value;
    SetValueIsSet();
  }

#if LIBEBML_VERSION >= 0x020000
  virtual libebml::filepos_t UpdateSize(ShouldWrite const &writeFilter = WriteSkipDefault, bool bForceRender = false) override;
#else
  virtual libebml::filepos_t UpdateSize(bool bSaveDefault, bool bForceRender) override;
#endif
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

  libebml::filepos_t Render(libebml::IOCallback &output) {
#if LIBEBML_VERSION >= 0x020000
    return libebml::EbmlElement::Render(output, WriteAll, false, true);
#else
    return libebml::EbmlElement::Render(output, true, false, true);
#endif
  }
};

class kax_cues_with_cleanup_c: public libmatroska::KaxCues {
public:
  kax_cues_with_cleanup_c();
  virtual ~kax_cues_with_cleanup_c();
};

class kax_block_add_id_c: public libmatroska::KaxBlockAddID {
public:
  kax_block_add_id_c([[maybe_unused]] bool always_write)
    : libmatroska::KaxBlockAddID{}
  {
#if LIBEBML_VERSION < 0x020000
    if (always_write)
      ForceNoDefault();
#endif
  }
};
