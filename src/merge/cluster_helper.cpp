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

#include "common/common_pch.h"

#include "common/at_scope_exit.h"
#include "common/command_line.h"
#include "common/doc_type_version_handler.h"
#include "common/ebml.h"
#include "common/hacks.h"
#include "common/strings/formatting.h"
#include "common/translation.h"
#include "merge/cluster_helper.h"
#include "merge/cues.h"
#include "merge/generic_packetizer.h"
#include "merge/generic_reader.h"
#include "merge/libmatroska_extensions.h"
#include "merge/output_control.h"
#include "merge/packet_extensions.h"
#include "merge/private/cluster_helper.h"

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSeekHead.h>

debugging_option_c render_groups_c::ms_gap_detection{"cluster_helper_gap_detection"};

cluster_helper_c::impl_t::~impl_t() {
}

cluster_helper_c::cluster_helper_c()
  : m{new cluster_helper_c::impl_t{}}
{
}

cluster_helper_c::~cluster_helper_c() {
}

mm_io_c *
cluster_helper_c::get_output() {
  return m->out;
}

libmatroska::KaxCluster *
cluster_helper_c::get_cluster() {
  return m->cluster.get();
}

int64_t
cluster_helper_c::get_first_timestamp_in_file()
  const {
  return m->first_timestamp_in_file;
}

int64_t
cluster_helper_c::get_first_timestamp_in_part()
  const {
  return m->first_timestamp_in_part;
}

int64_t
cluster_helper_c::get_max_timestamp_in_file()
  const {
  return m->max_timestamp_in_file;
}

int
cluster_helper_c::get_packet_count()
  const {
  return m->packets.size();
}

bool
cluster_helper_c::splitting()
  const {
  if (g_splitting_by_all_chapters || !g_splitting_by_chapter_numbers.empty())
    return true;

  return !m->split_points.empty();
}

bool
cluster_helper_c::discarding()
  const {
  return splitting() && m->discarding;
}

bool
cluster_helper_c::is_splitting_and_processed_fully()
  const {
  return m->splitting_and_processed_fully;
}

void
cluster_helper_c::render_before_adding_if_necessary(packet_cptr const &packet) {
  int64_t timestamp        = get_timestamp();
  int64_t timestamp_delay  = (   (packet->assigned_timestamp > m->max_timestamp_in_cluster)
                              || (-1 == m->max_timestamp_in_cluster))                        ? packet->assigned_timestamp : m->max_timestamp_in_cluster;
  timestamp_delay         -= (   (-1 == m->min_timestamp_in_cluster)
                              || (packet->assigned_timestamp < m->min_timestamp_in_cluster)) ? packet->assigned_timestamp : m->min_timestamp_in_cluster;
  timestamp_delay          = (int64_t)(timestamp_delay / g_timestamp_scale);

  mxdebug_if(m->debug_packets,
             fmt::format("cluster_helper_c::add_packet(): new packet {{ source {0}/{1} "
                         "timestamp: {2} duration: {3} bref: {4} fref: {5} assigned_timestamp: {6} timestamp_delay: {7} }}\n",
                         packet->source->m_ti.m_id, packet->source->m_ti.m_fname, packet->timestamp,          packet->duration,
                         packet->bref,              packet->fref,                 packet->assigned_timestamp, mtx::string::format_timestamp(timestamp_delay)));

  bool is_video_keyframe = (packet->source == g_video_packetizer) && packet->is_key_frame();
  bool do_render         = (std::numeric_limits<int16_t>::max() < timestamp_delay)
                        || (std::numeric_limits<int16_t>::min() > timestamp_delay)
                        || (   (std::max<int64_t>(0, m->min_timestamp_in_cluster) > m->previous_cluster_ts)
                            && (packet->assigned_timestamp                        > m->min_timestamp_in_cluster)
                            && (!g_video_packetizer || !is_video_keyframe || m->first_video_keyframe_seen)
                            && (   (packet->gap_following && !m->packets.empty())
                                || ((packet->assigned_timestamp - timestamp) > g_max_ns_per_cluster)
                                || is_video_keyframe));

  if (is_video_keyframe)
    m->first_video_keyframe_seen = true;

  mxdebug_if(m->debug_rendering,
             fmt::format("render check cur_ts {8} min_ts_ic {0} prev_cl_ts {1} test {2} is_vid_and_key {3} tc_delay {4} gap_following_and_not_empty {5} cur_ts>min_ts_ic {7} first_video_key_seen {9} do_render {6}\n",
                         m->min_timestamp_in_cluster, m->previous_cluster_ts, std::max<int64_t>(0, m->min_timestamp_in_cluster) > m->previous_cluster_ts, is_video_keyframe,
                         timestamp_delay, packet->gap_following && !m->packets.empty(), do_render, packet->assigned_timestamp > m->min_timestamp_in_cluster, packet->assigned_timestamp, m->first_video_keyframe_seen));

  if (!do_render)
    return;

  render();
  prepare_new_cluster();
}

void
cluster_helper_c::render_after_adding_if_necessary(packet_cptr const &packet) {
  // Render the cluster if it is full (according to my many criteria).
  auto timestamp = get_timestamp();
  if (   ((packet->assigned_timestamp - timestamp) > g_max_ns_per_cluster)
      || (m->packets.size()                        > static_cast<size_t>(g_max_blocks_per_cluster))
      || (get_cluster_content_size()               > 1500000)) {
    render();
    prepare_new_cluster();
  }
}

void
cluster_helper_c::split_if_necessary(packet_cptr const &packet) {
  if (   !splitting()
      || (m->current_split_point_idx >= m->split_points.size())
      || (g_file_num > g_split_max_num_files)
      || !packet->is_key_frame()
      || (   (packet->source->get_track_type() != track_video)
          && g_video_packetizer))
    return;

  auto &current_split_point = m->split_points[m->current_split_point_idx];
  bool split_now = false;

  // Maybe we want to start a new file now.
  if (split_point_c::size == current_split_point.m_type) {
    int64_t additional_size = 0;

    if (!m->packets.empty())
      // Cluster + Cluster timestamp: roughly 21 bytes. Add all frame sizes & their overheaders, too.
      additional_size = 21 + std::accumulate(m->packets.begin(), m->packets.end(), 0, [](size_t size, const packet_cptr &p) { return size + p->data->get_size() + (p->is_key_frame() ? 10 : p->is_p_frame() ? 13 : 16); });

    additional_size += 18 * m->num_cue_elements;

    mxdebug_if(m->debug_splitting,
               fmt::format("cluster_helper split decision: header_overhead: {0}, additional_size: {1}, bytes_in_file: {2}, sum: {3}\n",
                           m->header_overhead, additional_size, m->bytes_in_file, m->header_overhead + additional_size + m->bytes_in_file));
    if ((m->header_overhead + additional_size + m->bytes_in_file) >= current_split_point.m_point)
      split_now = true;

  } else if (   (split_point_c::duration == current_split_point.m_type)
             && (0 <= m->first_timestamp_in_file)
                && (timestamp_c::ns(packet->assigned_timestamp - m->first_timestamp_in_file - current_split_point.m_point) > timestamp_c::ms(-1)))
    split_now = true;

  else if (   (   (split_point_c::timestamp == current_split_point.m_type)
               || (split_point_c::parts     == current_split_point.m_type))
           && (timestamp_c::ns(packet->assigned_timestamp - current_split_point.m_point) > timestamp_c::ms(-1)))
    split_now = true;

  else if (   (   (split_point_c::frame_field       == current_split_point.m_type)
               || (split_point_c::parts_frame_field == current_split_point.m_type))
           && (m->frame_field_number >= current_split_point.m_point))
    split_now = true;

  if (!split_now)
    return;

  split(packet);
}

void
cluster_helper_c::split(packet_cptr const &packet) {
  render();

  m->num_cue_elements = 0;

  auto &current_split_point  = m->split_points[m->current_split_point_idx];
  bool create_new_file       = current_split_point.m_create_new_file;
  bool previously_discarding = m->discarding;
  auto generate_chapter      = false;
  auto formatted_splitting_before = mtx::string::format_timestamp(packet->assigned_timestamp);

  mxdebug_if(m->debug_splitting, fmt::format("Splitting: splitpoint {0} reached before timestamp {1}, create new? {2}.\n", current_split_point.str(), formatted_splitting_before, create_new_file));

  finish_file(false, create_new_file, previously_discarding);

  if (mtx::cli::g_gui_mode)
    mxinfo(fmt::format("#GUI#splitting_before_timestamp {0}\n", formatted_splitting_before));

  mxinfo(fmt::format(FY("Timestamp used in split decision: {0}\n"), formatted_splitting_before));

  if (current_split_point.m_use_once) {
    if (   !current_split_point.m_discard
        && (chapter_generation_mode_e::when_appending == m->chapter_generation_mode)
        && (   (split_point_c::parts             == current_split_point.m_type)
            || (split_point_c::parts_frame_field == current_split_point.m_type)))
      generate_chapter = true;

    if (   current_split_point.m_discard
        && (   (split_point_c::parts             == current_split_point.m_type)
            || (split_point_c::parts_frame_field == current_split_point.m_type))
        && ((m->current_split_point_idx + 1) >= m->split_points.size())) {
      mxdebug_if(m->debug_splitting, fmt::format("Splitting: Last part in 'parts:' splitting mode finished\n"));
      m->splitting_and_processed_fully = true;
    }

    m->discarding = current_split_point.m_discard;
    ++m->current_split_point_idx;
  }

  if (create_new_file) {
    create_next_output_file();
    if (g_no_linking) {
      m->previous_cluster_ts = -1;
      m->timestamp_offset    = g_video_packetizer ? m->max_video_timestamp_rendered : packet->assigned_timestamp;
    }

    m->bytes_in_file           =  0;
    m->first_timestamp_in_file = -1;
    m->max_timestamp_in_file   = -1;
    m->min_timestamp_in_file.reset();
  }

  m->first_timestamp_in_part = -1;

  handle_discarded_duration(create_new_file, previously_discarding);

  if (generate_chapter)
    generate_one_chapter(timestamp_c::ns(packet->assigned_timestamp - std::max<int64_t>(0, m->timestamp_offset) - m->discarded_duration));

  prepare_new_cluster();
}

void
cluster_helper_c::add_packet(packet_cptr const &packet) {
  if (!m->cluster)
    prepare_new_cluster();

  packet->normalize_timestamps();
  render_before_adding_if_necessary(packet);
  split_if_necessary(packet);

  m->packets.push_back(packet);
  m->cluster_content_size += packet->data->get_size();

  if (packet->assigned_timestamp > m->max_timestamp_in_cluster)
    m->max_timestamp_in_cluster = packet->assigned_timestamp;

  if ((-1 == m->min_timestamp_in_cluster) || (packet->assigned_timestamp < m->min_timestamp_in_cluster))
    m->min_timestamp_in_cluster = packet->assigned_timestamp;

  render_after_adding_if_necessary(packet);

  if (g_video_packetizer == packet->source)
    ++m->frame_field_number;

  generate_chapters_if_necessary(packet);
}

int64_t
cluster_helper_c::get_timestamp() {
  return m->packets.empty() ? 0 : m->packets.front()->assigned_timestamp;
}

void
cluster_helper_c::prepare_new_cluster() {
  m->cluster.reset(new kax_cluster_c);
  m->cluster_content_size = 0;
  m->packets.clear();

  m->cluster->SetParent(*g_kax_segment);
  set_previous_timestamp(*m->cluster, std::max<int64_t>(0, m->previous_cluster_ts), static_cast<int64_t>(g_timestamp_scale));
}

int
cluster_helper_c::get_cluster_content_size() {
  return m->cluster_content_size;
}

void
cluster_helper_c::set_output(mm_io_c *out) {
  m->out = out;
}

void
cluster_helper_c::set_duration(render_groups_c *rg) {
  if (rg->m_durations.empty())
    return;

  kax_block_blob_c *group = rg->m_groups.back().get();
  int64_t def_duration    = rg->m_source->get_track_default_duration();
  int64_t block_duration  = 0;

  size_t i;
  for (i = 0; rg->m_durations.size() > i; ++i)
    block_duration += rg->m_durations[i];
  mxdebug_if(m->debug_duration,
             fmt::format("cluster_helper::set_duration: block_duration {0} rounded duration {1} def_duration {2} use_durations {3} rg->m_duration_mandatory {4}\n",
                         block_duration, round_timestamp_scale(block_duration), def_duration, g_use_durations ? 1 : 0, rg->m_duration_mandatory ? 1 : 0));

  if (rg->m_duration_mandatory) {
    if (   (0 == block_duration)
        || (   (0 < block_duration)
            && (round_timestamp_scale(block_duration) != round_timestamp_scale(static_cast<int64_t>(rg->m_durations.size()) * def_duration)))) {
      auto rounding_error = rg->m_source->get_track_type() == track_subtitle ? rg->m_first_timestamp_rounding_error.value_or(0) : 0;
      group->set_block_duration(round_timestamp_scale(block_duration + rounding_error));
    }

  } else if (   (   g_use_durations
                 || (0 < def_duration))
             && (0 < block_duration)
             && (round_timestamp_scale(block_duration) != round_timestamp_scale(rg->m_durations.size() * def_duration)))
    group->set_block_duration(round_timestamp_scale(block_duration));
}

bool
cluster_helper_c::must_duration_be_set(render_groups_c *rg,
                                       packet_cptr const &new_packet) {
  size_t i;
  int64_t block_duration = 0;
  int64_t def_duration   = new_packet->source->get_track_default_duration();
  int64_t group_size     = rg ? rg->m_durations.size() : 0;

  if (rg)
    for (i = 0; rg->m_durations.size() > i; ++i)
      block_duration += rg->m_durations[i];

  block_duration += new_packet->get_duration();

  mxdebug_if(m->debug_duration, fmt::format("must_duration_be_set: rg 0x{0} block_duration {1} group_size {2} rg->duration_mandatory {3} new_packet->duration_mandatory {4}\n",
                                            static_cast<void *>(rg), block_duration, group_size, !rg ? "—" : rg->m_duration_mandatory ? "1" : "0", new_packet->duration_mandatory));

  if ((rg && rg->m_duration_mandatory) || new_packet->duration_mandatory) {
    if (   (   (0 == block_duration)
            && ((0 != group_size) || (new_packet->duration_mandatory && new_packet->has_duration())))
        || (   (0 < block_duration)
            && (round_timestamp_scale(block_duration) != round_timestamp_scale((group_size + 1) * def_duration)))) {
      // if (!rg)
      //   mxinfo(fmt::format("YOYO 1 np mand {0} blodu {1} defdur {2} bloduRND {3} defdurRND {4} groupsize {5} ts {6}\n", new_packet->duration_mandatory, block_duration, (group_size + 1) * def_duration,
      //          round_timestamp_scale(block_duration), round_timestamp_scale((group_size + 1) * def_duration), group_size, mtx::string::format_timestamp(new_packet->assigned_timestamp)));
      return true;
    }

  } else if (   (   g_use_durations
                 || (0 < def_duration))
             && (0 < block_duration)
             && (round_timestamp_scale(block_duration) != round_timestamp_scale((group_size + 1) * def_duration))) {
    // if (!rg)
    //   mxinfo(fmt::format("YOYO 1 np blodu {0} defdur {1} bloduRND {2} defdurRND {3} ts {4}\n", block_duration, def_duration, round_timestamp_scale(block_duration), round_timestamp_scale((group_size + 1) * def_duration),
    //          mtx::string::format_timestamp(new_packet->assigned_timestamp)));
    return true;
  }

  return false;
}

/*
  <+Asylum> The chicken and the egg are lying in bed next to each
            other after a good hard shag, the chicken is smoking a
            cigarette, the egg says "Well that answers that bloody
            question doesn't it"
*/

int
cluster_helper_c::render() {

  std::vector<render_groups_cptr> render_groups;
  kax_cues_with_cleanup_c cues;
  set_global_timestamp_scale(cues, g_timestamp_scale);

  bool use_simpleblock     = !mtx::hacks::is_engaged(mtx::hacks::NO_SIMPLE_BLOCKS);

  auto lacing_type         = mtx::hacks::is_engaged(mtx::hacks::LACING_XIPH) ? libmatroska::LACING_XIPH : mtx::hacks::is_engaged(mtx::hacks::LACING_EBML) ? libmatroska::LACING_EBML : libmatroska::LACING_AUTO;

  int64_t min_cl_timestamp = std::numeric_limits<int64_t>::max();
  int64_t max_cl_timestamp = 0;

  int elements_in_cluster  = 0;
  bool added_to_cues       = false;

  // Splitpoint stuff
  if ((-1 == m->header_overhead) && splitting())
    m->header_overhead = m->out->getFilePointer() + g_tags_size;

  // Make sure that we don't have negative/wrapped around timestamps in the output file.
  // Can happend when we're splitting; so adjust timestamp_offset accordingly.
  m->timestamp_offset      = std::accumulate(m->packets.begin(), m->packets.end(), m->timestamp_offset, [](int64_t a, const packet_cptr &p) { return std::min(a, p->assigned_timestamp); });
  int64_t timestamp_offset = m->timestamp_offset + get_discarded_duration();

  for (auto &pack : m->packets) {
    generic_packetizer_c *source = pack->source;
    bool has_codec_state         = !!pack->codec_state;

    if (g_video_packetizer == source)
      m->max_video_timestamp_rendered = std::max(pack->assigned_timestamp + pack->get_duration(), m->max_video_timestamp_rendered);

    if (discarding()) {
      if (-1 == m->first_discarded_timestamp)
        m->first_discarded_timestamp = pack->assigned_timestamp;
      m->last_discarded_timestamp_and_duration = std::max(m->last_discarded_timestamp_and_duration, pack->assigned_timestamp + pack->get_duration());
      continue;
    }

    render_groups_c *render_group = nullptr;
    for (auto &rg : render_groups)
      if (rg->m_source == source) {
        render_group = rg.get();
        break;
      }

    if (!render_group) {
      render_groups.push_back(render_groups_cptr(new render_groups_c(source)));
      render_group = render_groups.back().get();
    }

    min_cl_timestamp                       = std::min(pack->assigned_timestamp, min_cl_timestamp);
    max_cl_timestamp                       = std::max(pack->assigned_timestamp, max_cl_timestamp);

    libmatroska::KaxTrackEntry &track_entry             = static_cast<libmatroska::KaxTrackEntry &>(*source->get_track_entry());

    kax_block_blob_c *previous_block_group = !render_group->m_groups.empty() ? render_group->m_groups.back().get() : nullptr;
    kax_block_blob_c *new_block_group      = previous_block_group;

    auto require_new_render_group          = !render_group->m_more_data
                                          || !pack->is_key_frame()
                                          || has_codec_state
                                          || pack->has_discard_padding()
                                          || render_group->m_has_discard_padding
                                          || render_group->follows_gap(*pack)
                                          || must_duration_be_set(nullptr, pack)
                                          || source->is_lacing_prevented();

    if (require_new_render_group) {
      set_duration(render_group);
      render_group->m_durations.clear();
      render_group->m_duration_mandatory = false;
      render_group->m_first_timestamp_rounding_error.reset();

      libmatroska::BlockBlobType this_block_blob_type
        = !use_simpleblock                         ? libmatroska::BLOCK_BLOB_NO_SIMPLE
        : must_duration_be_set(render_group, pack) ? libmatroska::BLOCK_BLOB_NO_SIMPLE
        : !pack->data_adds.empty()                 ? libmatroska::BLOCK_BLOB_NO_SIMPLE
        : has_codec_state                          ? libmatroska::BLOCK_BLOB_NO_SIMPLE
        : pack->has_discard_padding()              ? libmatroska::BLOCK_BLOB_NO_SIMPLE
        :                                            libmatroska::BLOCK_BLOB_ALWAYS_SIMPLE;

      render_group->m_groups.push_back(kax_block_blob_cptr(new kax_block_blob_c(this_block_blob_type)));
      new_block_group = render_group->m_groups.back().get();
      m->cluster->AddBlockBlob(new_block_group);
      new_block_group->SetParent(*m->cluster);

      added_to_cues = false;
    }

    if (!render_group->m_first_timestamp_rounding_error)
      render_group->m_first_timestamp_rounding_error = pack->unmodified_assigned_timestamp - pack->assigned_timestamp;

    for (auto const &extension : pack->extensions)
      if (packet_extension_c::BEFORE_ADDING_TO_CLUSTER_CB == extension->get_type())
        static_cast<before_adding_to_cluster_cb_packet_extension_c *>(extension.get())->get_callback()(pack, timestamp_offset);

    auto data_buffer = new libmatroska::DataBuffer(static_cast<uint8_t *>(pack->data->get_buffer()), pack->data->get_size());

    // Now put the packet into the cluster.
    render_group->m_more_data = new_block_group->add_frame_auto(track_entry, pack->assigned_timestamp - timestamp_offset, *data_buffer, lacing_type,
                                                                pack->has_bref() ? pack->bref - timestamp_offset : -1,
                                                                pack->has_fref() ? pack->fref - timestamp_offset : -1,
                                                                pack->key_flag, pack->discardable_flag);

    if (has_codec_state) {
      auto &bgroup = (libmatroska::KaxBlockGroup &)*new_block_group;
      auto cstate  = new libmatroska::KaxCodecState;
      bgroup.PushElement(*cstate);
      cstate->CopyBuffer(pack->codec_state->get_buffer(), pack->codec_state->get_size());
    }

    if (-1 == m->first_timestamp_in_file)
      m->first_timestamp_in_file = pack->assigned_timestamp;
    if (-1 == m->first_timestamp_in_part)
      m->first_timestamp_in_part = pack->assigned_timestamp;

    m->min_timestamp_in_file      = std::min(timestamp_c::ns(pack->assigned_timestamp),       m->min_timestamp_in_file.value_or_max());
    m->max_timestamp_in_file      = std::max(pack->assigned_timestamp,                        m->max_timestamp_in_file);
    m->max_timestamp_and_duration = std::max(pack->assigned_timestamp + pack->get_duration(), m->max_timestamp_and_duration);

    if (!pack->is_key_frame() || !track_entry.LacingEnabled())
      render_group->m_more_data = false;

    if (pack->has_duration())
      render_group->m_durations.push_back(pack->get_unmodified_duration());
    render_group->m_duration_mandatory |= pack->duration_mandatory;
    render_group->m_expected_next_timestamp = pack->assigned_timestamp + pack->get_duration();

    cues_c::get().set_duration_for_id_timestamp(source->get_track_num(), pack->assigned_timestamp - timestamp_offset, pack->get_duration());

    if (new_block_group) {
      // Set the reference priority if it was wanted.
      if ((0 < pack->ref_priority) && new_block_group->replace_simple_by_group())
        get_child<libmatroska::KaxReferencePriority>(*new_block_group).SetValue(pack->ref_priority);

      // Handle BlockAdditions if needed
      if (!pack->data_adds.empty() && new_block_group->ReplaceSimpleByGroup()) {
        auto &additions = add_empty_child<libmatroska::KaxBlockAdditions>(*new_block_group);

        for (auto const &add : pack->data_adds) {
          auto &block_more = add_empty_child<libmatroska::KaxBlockMore>(additions);

          block_more.PushElement(*new kax_block_add_id_c{m->always_write_block_add_ids});
          static_cast<kax_block_add_id_c &>(*block_more.GetElementList().back()).SetValue(add.id.value_or(1));
          get_child<libmatroska::KaxBlockAdditional>(block_more).CopyBuffer(static_cast<uint8_t *>(add.data->get_buffer()), add.data->get_size());
        }
      }

      if (pack->has_discard_padding()) {
        get_child<libmatroska::KaxDiscardPadding>(*new_block_group).SetValue(pack->discard_padding.to_ns());
        render_group->m_has_discard_padding = true;
      }
    }

    elements_in_cluster++;

    if (!new_block_group)
      new_block_group = previous_block_group;

    else if (g_write_cues && (!added_to_cues || has_codec_state)) {
      added_to_cues = add_to_cues_maybe(pack);
      if (added_to_cues)
        cues.AddBlockBlob(*new_block_group);
    }

    pack->group = new_block_group;

    pack->account(m->track_statistics[ source->get_uid() ], timestamp_offset);

    source->after_packet_rendered(*pack);
  }

  mtx::at_scope_exit_c cleanup([this]() {
    m->cluster->delete_non_blocks();
  });

  if (!discarding()) {
    if (0 < elements_in_cluster) {
      for (auto &rg : render_groups)
        set_duration(rg.get());

      set_previous_timestamp(*m->cluster, min_cl_timestamp - timestamp_offset - 1, g_timestamp_scale);
      m->cluster->set_min_timestamp(min_cl_timestamp - timestamp_offset);
      m->cluster->set_max_timestamp(max_cl_timestamp - timestamp_offset);

#if LIBEBML_VERSION >= 0x020000
      m->cluster->Render(*m->out, cues, std::bind(&cluster_helper_c::write_element_pred, this, std::placeholders::_1));
#else
      m->cluster->Render(*m->out, cues);
#endif
      g_doc_type_version_handler->account(*m->cluster);
      m->bytes_in_file += m->cluster->ElementSize();

      if (g_kax_sh_cues)
        g_kax_sh_cues->IndexThis(*m->cluster, *g_kax_segment);

      m->previous_cluster_ts = get_global_timestamp(*m->cluster);

      cues_c::get().postprocess_cues(cues, *m->cluster);

    } else
      m->previous_cluster_ts = -1;
  }

  m->min_timestamp_in_cluster = -1;
  m->max_timestamp_in_cluster = -1;

  return 1;
}

bool
cluster_helper_c::add_to_cues_maybe(packet_cptr const &pack) {
  auto &source  = *pack->source;
  auto strategy = source.get_cue_creation();

  // Update the cues (index table) either if cue entries for I frames were requested and this is an I frame...
  bool add = (CUE_STRATEGY_IFRAMES == strategy) && pack->is_key_frame();

  // ... or if a codec state change is present ...
  add = add || !!pack->codec_state;

  // ... or if the user requested entries for all frames ...
  add = add || (CUE_STRATEGY_ALL == strategy);

  // ... or if this is a key frame for an audio track, there is no
  // video track and the last cue entry was created more than 0.5s
  // ago.
  add = add || (   (CUE_STRATEGY_SPARSE == strategy)
                && (track_audio         == source.get_track_type())
                && !g_video_packetizer
                && pack->is_key_frame()
                && (   (0 > source.get_last_cue_timestamp())
                    || ((pack->assigned_timestamp - source.get_last_cue_timestamp()) >= 500'000'000)));

  if (!add)
    return false;

  source.set_last_cue_timestamp(pack->assigned_timestamp);

  ++m->num_cue_elements;
  g_cue_writing_requested = 1;

  return true;
}

int64_t
cluster_helper_c::get_duration()
  const {
  auto result = m->max_timestamp_and_duration - m->min_timestamp_in_file.to_ns(0) - m->discarded_duration;
  mxdebug_if(m->debug_duration,
             fmt::format("cluster_helper_c::get_duration(): max_tc_and_dur {0} - min_tc_in_file {1} - discarded_duration {2} = {3} ; first_tc_in_file = {4}\n",
                         m->max_timestamp_and_duration, m->min_timestamp_in_file.to_ns(0), m->discarded_duration, result, m->first_timestamp_in_file));
  return result;
}

int64_t
cluster_helper_c::get_discarded_duration()
  const {
  return m->discarded_duration;
}

void
cluster_helper_c::handle_discarded_duration(bool create_new_file,
                                            bool previously_discarding) {
  m->previous_discarded_duration = m->discarded_duration;

  if (create_new_file) { // || (!previously_discarding && m->discarding)) {
    mxdebug_if(m->debug_splitting,
               fmt::format("RESETTING discarded duration of {0}, create_new_file {1} previously_discarding {2} m->discarding {3}\n",
                           m->discarded_duration, create_new_file, previously_discarding, m->discarding));
    m->discarded_duration = 0;

  } else if (previously_discarding && !m->discarding) {
    auto diff              = m->last_discarded_timestamp_and_duration - std::max<int64_t>(m->first_discarded_timestamp, 0);
    m->discarded_duration += diff;

    mxdebug_if(m->debug_splitting,
               fmt::format("ADDING to discarded duration TC at {0} / {1} diff {2} new total {3} create_new_file {4} previously_discarding {5} m->discarding {6}\n",
                           m->first_discarded_timestamp, m->last_discarded_timestamp_and_duration, diff, m->discarded_duration,
                           create_new_file, previously_discarding, m->discarding));
  } else
    mxdebug_if(m->debug_splitting,
               fmt::format("KEEPING discarded duration at {0}, create_new_file {1} previously_discarding {2} m->discarding {3}\n",
                           m->discarded_duration, create_new_file, previously_discarding, m->discarding));

  m->first_discarded_timestamp             = -1;
  m->last_discarded_timestamp_and_duration =  0;
}

void
cluster_helper_c::add_split_point(const split_point_c &split_point) {
  m->split_points.push_back(split_point);

  if (m->split_points.size() != 1)
    return;

  m->discarding = m->split_points[0].m_discard;

  if (0 == m->split_points[0].m_point)
    ++m->current_split_point_idx;
}

bool
cluster_helper_c::split_mode_produces_many_files()
  const {
  if (g_splitting_by_all_chapters || !g_splitting_by_chapter_numbers.empty())
    return true;

  if (!splitting())
    return false;

  if (   (split_point_c::parts             != m->split_points.front().m_type)
      && (split_point_c::parts_frame_field != m->split_points.front().m_type))
    return true;

  bool first = true;
  for (auto &split_point : m->split_points)
    if (!split_point.m_discard && split_point.m_create_new_file) {
      if (!first)
        return true;
      first = false;
    }

  return false;
}

void
cluster_helper_c::discard_queued_packets() {
  m->packets.clear();
}

void
cluster_helper_c::dump_split_points()
  const {
  mxdebug_if(m->debug_splitting,
             fmt::format("Split points:{0}\n",
                         std::accumulate(m->split_points.begin(), m->split_points.end(), ""s, [](std::string const &accu, split_point_c const &point) { return accu + " " + point.str(); })));
}

void
cluster_helper_c::create_tags_for_track_statistics(libmatroska::KaxTags &tags,
                                                   std::string const &writing_app,
                                                   QDateTime const &writing_date) {
  std::optional<QDateTime> actual_writing_date;
  if (g_write_date)
    actual_writing_date = writing_date;

  for (auto const &ptzr : g_packetizers) {
    auto track_uid = ptzr.packetizer->get_uid();

    m->track_statistics[track_uid]
      .set_track_uid(track_uid)
      .set_source_id(ptzr.packetizer->get_source_id())
      .create_tags(tags, writing_app, actual_writing_date);
  }

  m->track_statistics.clear();
}

void
cluster_helper_c::enable_chapter_generation(chapter_generation_mode_e mode,
                                            mtx::bcp47::language_c const &language) {
  m->chapter_generation_mode     = mode;
  m->chapter_generation_language = language.is_valid() ? language : mtx::bcp47::language_c::parse("eng");
}

chapter_generation_mode_e
cluster_helper_c::get_chapter_generation_mode()
  const {
  return m->chapter_generation_mode;
}

void
cluster_helper_c::set_chapter_generation_interval(timestamp_c const &interval) {
  m->chapter_generation_interval = interval;
}

void
cluster_helper_c::verify_and_report_chapter_generation_parameters()
  const {
  if (chapter_generation_mode_e::none == m->chapter_generation_mode)
    return;

  if (!m->chapter_generation_reference_track)
    mxerror(Y("Chapter generation is only possible if at least one video or audio track is copied.\n"));

  mxinfo(fmt::format(FY("Using the track with the ID {0} from the file '{1}' as the reference for chapter generation.\n"),
                     m->chapter_generation_reference_track->m_ti.m_id, m->chapter_generation_reference_track->m_ti.m_fname));
}

void
cluster_helper_c::register_new_packetizer(generic_packetizer_c &ptzr) {
  auto new_track_type = ptzr.get_track_type();

  if (!g_video_packetizer && (track_video == new_track_type))
    g_video_packetizer = &ptzr;

  auto current_ptzr_prio = !m->chapter_generation_reference_track                                 ?   0
                         : m->chapter_generation_reference_track->get_track_type() == track_video ? 100
                         : m->chapter_generation_reference_track->get_track_type() == track_audio ?  80
                         :                                                                            0;

  auto new_ptzr_prio     = new_track_type                                          == track_video ? 100
                         : new_track_type                                          == track_audio ?  80
                         :                                                                            0;

  if (new_ptzr_prio > current_ptzr_prio)
    m->chapter_generation_reference_track = &ptzr;
}

void
cluster_helper_c::generate_chapters_if_necessary(packet_cptr const &packet) {
  if ((chapter_generation_mode_e::none == m->chapter_generation_mode) || !m->chapter_generation_reference_track)
    return;

  auto successor = m->chapter_generation_reference_track->get_connected_successor();
  if (successor) {
    if (chapter_generation_mode_e::when_appending == m->chapter_generation_mode)
      m->chapter_generation_last_generated.reset();

    while ((successor = m->chapter_generation_reference_track->get_connected_successor()))
      m->chapter_generation_reference_track = successor;
  }

  auto ptzr = packet->source;
  if (ptzr != m->chapter_generation_reference_track)
    return;

  if (chapter_generation_mode_e::when_appending == m->chapter_generation_mode) {
    if (packet->is_key_frame() && !m->chapter_generation_last_generated.valid())
      generate_one_chapter(timestamp_c::ns(packet->assigned_timestamp));

    return;
  }

  if (chapter_generation_mode_e::interval != m->chapter_generation_mode)
    return;

  auto now = timestamp_c::ns(packet->assigned_timestamp);

  while (true) {
    if (!m->chapter_generation_last_generated.valid())
      generate_one_chapter(timestamp_c::ns(0));

    auto next_chapter = m->chapter_generation_last_generated + m->chapter_generation_interval;
    if (next_chapter > now)
      break;

    generate_one_chapter(next_chapter);
  }
}

void
cluster_helper_c::generate_one_chapter(timestamp_c const &timestamp) {
  auto appended_file_name               = chapter_generation_mode_e::when_appending == m->chapter_generation_mode ? m->chapter_generation_reference_track->m_reader->m_ti.m_fname : std::string{};
  auto appended_title                   = chapter_generation_mode_e::when_appending == m->chapter_generation_mode ? m->chapter_generation_reference_track->m_reader->m_ti.m_title : std::string{};
  m->chapter_generation_number         += 1;
  m->chapter_generation_last_generated  = timestamp;
  auto name                             = mtx::chapters::format_name_template(mtx::chapters::g_chapter_generation_name_template.get_translated(), m->chapter_generation_number, timestamp, appended_file_name, appended_title);

  add_chapter_atom(timestamp, name, m->chapter_generation_language);
}

#if LIBEBML_VERSION >= 0x020000
bool
cluster_helper_c::write_element_pred(libebml::EbmlElement const &elt) {
  if (elt.GetClassId() == EBML_ID(libmatroska::KaxBlockAddID))
    return m->always_write_block_add_ids || !elt.IsDefaultValue();

  return libebml::EbmlElement::WriteSkipDefault(elt);
}
#endif

std::unique_ptr<cluster_helper_c> g_cluster_helper;
