/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/tags/tags.h"
#include "common/track_statistics.h"
#include "propedit/change.h"
#include "propedit/track_target.h"

namespace libmatroska {
class KaxBlockGroup;
class KaxSimpleBlock;
class KaxCluster;
}

class content_decoder_c;

class tag_target_c: public track_target_c {
public:
  enum tag_operation_mode_e {
    tom_undefined,
    tom_all,
    tom_global,
    tom_track,
    tom_add_track_statistics,
    tom_delete_track_statistics,
  };

  tag_operation_mode_e m_operation_mode;
  std::shared_ptr<libmatroska::KaxTags> m_new_tags;
  bool m_tags_modified{};

  std::unordered_map<uint64_t, uint64_t> m_default_durations_by_number;
  std::unordered_map<uint64_t, track_statistics_c> m_track_statistics_by_number;
  std::unordered_map<uint64_t, std::shared_ptr<content_decoder_c>> m_content_decoders_by_number;
  uint64_t m_timestamp_scale{};

public:
  tag_target_c();
  tag_target_c(tag_operation_mode_e operation_mode);
  virtual ~tag_target_c() override;

  virtual void validate() override;

  virtual bool operator ==(target_c const &cmp) const override;
  virtual void parse_tags_spec(const std::string &spec);
  virtual void dump_info() const override;

  virtual bool has_changes() const override;
  virtual bool has_content_been_modified() const override;

  virtual void execute() override;
  virtual bool write_elements_set_to_default_value() const override;
  virtual bool add_mandatory_elements_if_missing() const override;

protected:
  virtual void add_or_replace_global_tags(libmatroska::KaxTags *tags);
  virtual void add_or_replace_track_tags(libmatroska::KaxTags *tags);
  virtual void add_or_replace_track_statistics_tags();
  virtual void delete_track_statistics_tags();

  virtual bool non_track_target() const;
  virtual bool sub_master_is_track() const;
  virtual bool requires_sub_master() const;

  virtual bool read_segment_info_and_tracks();
  virtual void account_frame(uint64_t track_num, uint64_t timestamp, uint64_t duration, memory_cptr frame);
  virtual void account_block_group(libmatroska::KaxBlockGroup &block_group, libmatroska::KaxCluster &cluster);
  virtual void account_simple_block(libmatroska::KaxSimpleBlock &simple_block, libmatroska::KaxCluster &cluster);
  virtual void account_one_cluster(libmatroska::KaxCluster &cluster);
  virtual void account_all_clusters();
  virtual void create_track_statistics_tags();
};
