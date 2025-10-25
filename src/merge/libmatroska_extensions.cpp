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

#include "common/common_pch.h"

#include <cassert>

#include "common/ebml.h"
#include "merge/libmatroska_extensions.h"

kax_reference_block_c::kax_reference_block_c():
  libmatroska::KaxReferenceBlock(), m_value(-1) {
}

libebml::filepos_t
kax_reference_block_c::UpdateSize(
#if LIBEBML_VERSION >= 0x020000
                                  ShouldWrite const &writeFilter,
#else
                                  bool bSaveDefault,
#endif
                                  bool bForceRender) {
#if LIBEBML_VERSION >= 0x020000
  if (!bTimestampSet) {
    assert(-1 != m_value);
    SetValue((m_value - static_cast<int64_t>(get_global_timestamp(*ParentBlock))) / static_cast<int64_t>(get_global_timestamp_scale(*ParentBlock)));
  }

  return libebml::EbmlSInteger::UpdateSize(writeFilter, bForceRender);
#else
  if (!bTimecodeSet) {
    assert(-1 != m_value);
    SetValue((m_value - static_cast<int64_t>(ParentBlock->GlobalTimecode())) / static_cast<int64_t>(ParentBlock->GlobalTimecodeScale()));
  }

  return libebml::EbmlSInteger::UpdateSize(bSaveDefault, bForceRender);
#endif
}

bool
kax_block_group_c::add_frame(const libmatroska::KaxTrackEntry &track,
                             uint64_t timestamp,
                             libmatroska::DataBuffer &buffer,
                             int64_t past_block,
                             int64_t forw_block,
                             libmatroska::LacingType lacing) {
  libmatroska::KaxBlock & block = get_child<libmatroska::KaxBlock>(*this);
  assert(ParentCluster);
  block.SetParent(*ParentCluster);

  ParentTrack                     = &track;
  bool result                     = block.AddFrame(track, timestamp, buffer, lacing);
  kax_reference_block_c *past_ref = nullptr;

  if (0 <= past_block) {
    past_ref = find_child<kax_reference_block_c>(*this);
    if (!past_ref) {
      past_ref = new kax_reference_block_c;
      PushElement(*past_ref);
    }
    past_ref->set_value(past_block);
    past_ref->SetParentBlock(*this);
  }

  if (0 <= forw_block) {
    kax_reference_block_c *forw_ref = find_child<kax_reference_block_c>(*this);
    if (past_ref == forw_ref) {
      forw_ref = new kax_reference_block_c;
      PushElement(*forw_ref);
    }
    forw_ref->set_value(forw_block);
    forw_ref->SetParentBlock(*this);
  }

  return result;
}

bool
kax_block_blob_c::add_frame_auto(const libmatroska::KaxTrackEntry &track,
                                 uint64_t timestamp,
                                 libmatroska::DataBuffer &buffer,
                                 libmatroska::LacingType lacing,
                                 int64_t past_block,
                                 int64_t forw_block,
                                 std::optional<bool> key_flag,
                                 std::optional<bool> discardable_flag) {
  bool result = false;

  if (   (libmatroska::BLOCK_BLOB_ALWAYS_SIMPLE == SimpleBlockMode)
      || (   (libmatroska::BLOCK_BLOB_SIMPLE_AUTO == SimpleBlockMode)
          && (-1 == past_block)
          && (-1 == forw_block))) {
    assert(true == bUseSimpleBlock);
    if (!Block.simpleblock) {
      Block.simpleblock = new libmatroska::KaxSimpleBlock();
      Block.simpleblock->SetParent(*ParentCluster);
    }

    result = Block.simpleblock->AddFrame(track, timestamp, buffer, lacing);

    if (key_flag || discardable_flag) {
      Block.simpleblock->SetKeyframe(key_flag && *key_flag);
      Block.simpleblock->SetDiscardable(discardable_flag && *discardable_flag);

    } else if ((-1 == past_block) && (-1 == forw_block)) {
      Block.simpleblock->SetKeyframe(true);
      Block.simpleblock->SetDiscardable(false);

    } else {
      Block.simpleblock->SetKeyframe(false);
      if (   ((-1 == forw_block) || (forw_block <= static_cast<int64_t>(timestamp)))
          && ((-1 == past_block) || (past_block <= static_cast<int64_t>(timestamp))))
        Block.simpleblock->SetDiscardable(false);
      else
        Block.simpleblock->SetDiscardable(true);
    }

  } else if (replace_simple_by_group()) {
    kax_block_group_c *group = static_cast<kax_block_group_c *>(Block.group);
    result = group->add_frame(track, timestamp, buffer, past_block, forw_block, lacing);
  }

  return result;
}

bool
kax_block_blob_c::replace_simple_by_group() {
  if (libmatroska::BLOCK_BLOB_ALWAYS_SIMPLE == SimpleBlockMode)
    return false;

  if (!bUseSimpleBlock) {
    if (!Block.group)
      Block.group = new kax_block_group_c();

  } else if (Block.simpleblock)
    assert(false);
  else
    Block.group = new kax_block_group_c();

  if (ParentCluster)
    Block.group->SetParent(*ParentCluster);

  bUseSimpleBlock = false;

  return true;
}

void
kax_block_blob_c::set_block_duration(uint64_t time_length) {
  if (replace_simple_by_group())
    Block.group->SetBlockDuration(time_length);
}

kax_cluster_c::~kax_cluster_c() {
  delete_non_blocks();
}

// The kax_block_group_c objects are stored in std::shared_ptrs outside of
// the cluster structure as well. libmatroska::KaxSimpleBlock objects are deleted
// when they're replaced with kax_block_group_c. All other object
// types must be deleted explicitely. This applies to
// e.g. libmatroska::KaxClusterTimecodes.
void
kax_cluster_c::delete_non_blocks() {
  unsigned idx;
  for (idx = 0; ListSize() > idx; ++idx) {
    libebml::EbmlElement *e = (*this)[idx];
    if (!dynamic_cast<kax_block_group_c *>(e) && !dynamic_cast<libmatroska::KaxSimpleBlock *>(e))
      delete e;
  }

  RemoveAll();
}

kax_cues_with_cleanup_c::kax_cues_with_cleanup_c()
  : libmatroska::KaxCues{}
{
}

kax_cues_with_cleanup_c::~kax_cues_with_cleanup_c() {
  // If rendering fails e.g. due to the file system being full,
  // libmatroska may assert() due to myTempReferences being
  // non-eempty. We don't care about that assertion.
  myTempReferences.clear();
}
