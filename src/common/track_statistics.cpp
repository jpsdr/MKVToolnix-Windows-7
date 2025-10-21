/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/construct.h"
#include "common/date_time.h"
#include "common/strings/formatting.h"
#include "common/tags/tags.h"
#include "common/track_statistics.h"

void
track_statistics_c::create_tags(libmatroska::KaxTags &tags,
                                std::string const &writing_app,
                                std::optional<QDateTime> const &writing_date)
  const {
  auto bps      = get_bits_per_second();
  auto duration = get_duration();
  auto names    = std::vector<std::string>{ "BPS"s, "DURATION"s, "NUMBER_OF_FRAMES"s, "NUMBER_OF_BYTES"s };

  mtx::tags::remove_simple_tags_for<libmatroska::KaxTagTrackUID>(tags, m_track_uid, "BPS");
  mtx::tags::remove_simple_tags_for<libmatroska::KaxTagTrackUID>(tags, m_track_uid, "DURATION");
  mtx::tags::remove_simple_tags_for<libmatroska::KaxTagTrackUID>(tags, m_track_uid, "NUMBER_OF_FRAMES");
  mtx::tags::remove_simple_tags_for<libmatroska::KaxTagTrackUID>(tags, m_track_uid, "NUMBER_OF_BYTES");

  auto tag = find_or_create_tag(tags);

  mtx::tags::set_simple(*tag, "BPS",              fmt::to_string(bps ? *bps : 0));
  mtx::tags::set_simple(*tag, "DURATION",         mtx::string::format_timestamp(duration ? *duration : 0));
  mtx::tags::set_simple(*tag, "NUMBER_OF_FRAMES", fmt::to_string(m_num_frames));
  mtx::tags::set_simple(*tag, "NUMBER_OF_BYTES",  fmt::to_string(m_num_bytes));

  if (!m_source_id.empty()) {
    mtx::tags::set_simple(*tag, "SOURCE_ID", m_source_id);
    names.emplace_back("SOURCE_ID"s);
  }

  mtx::tags::set_simple(*tag, "_STATISTICS_WRITING_APP", writing_app);

  if (writing_date) {
    auto writing_date_str = mtx::date_time::format(writing_date->toUTC(), "%Y-%m-%d %H:%M:%S");
    mtx::tags::set_simple(*tag, "_STATISTICS_WRITING_DATE_UTC", writing_date_str);
  }

  mtx::tags::set_simple(*tag, "_STATISTICS_TAGS", mtx::string::join(names, " "));
}

libmatroska::KaxTag *
track_statistics_c::find_or_create_tag(libmatroska::KaxTags &tags)
  const {
  for (auto &element : tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(element);
    if (!tag)
      continue;

    auto targets = find_child<libmatroska::KaxTagTargets>(*tag);
    if (!targets)
      continue;

    auto actual_target_type_value = static_cast<mtx::tags::target_type_e>(find_child_value<libmatroska::KaxTagTargetTypeValue>(*targets, 0ull));
    auto actual_id                = find_child_value<libmatroska::KaxTagTrackUID>(*targets, 0ull);
    auto actual_target_type       = find_child<libmatroska::KaxTagTargetType>(*targets);

    if (   (actual_target_type_value == mtx::tags::Movie)
        && (actual_id                == m_track_uid)
        && !actual_target_type)
      return tag;
  }

  using namespace mtx::construct;

  auto tag = cons<libmatroska::KaxTag>(cons<libmatroska::KaxTagTargets>(new libmatroska::KaxTagTargetTypeValue, mtx::tags::Movie,
                                                                        new libmatroska::KaxTagTrackUID,        m_track_uid));
  tags.PushElement(*tag);

  return tag;
}
