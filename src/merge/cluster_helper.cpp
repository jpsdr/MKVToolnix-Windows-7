/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   The cluster helper groups frames into blocks groups and those
   into clusters, sets the durations, renders the clusters etc.

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/date_time.h"
#include "common/ebml.h"
#include "common/hacks.h"
#include "common/math.h"
#include "common/strings/formatting.h"
#include "merge/cluster_helper.h"
#include "merge/cues.h"
#include "merge/libmatroska_extensions.h"
#include "merge/output_control.h"
#include "merge/private/cluster_helper.h"
#include "output/p_video.h"

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCuesData.h>
#include <matroska/KaxSeekHead.h>

cluster_helper_c::impl_t::impl_t()
  : cluster{}
  , cluster_content_size{}
  , max_timecode_and_duration{}
  , max_video_timecode_rendered{}
  , previous_cluster_tc{-1}
  , num_cue_elements{}
  , header_overhead{-1}
  , timecode_offset{}
  , bytes_in_file{}
  , first_timecode_in_file{-1}
  , first_timecode_in_part{-1}
  , first_discarded_timecode{-1}
  , last_discarded_timecode_and_duration{}
  , discarded_duration{}
  , previous_discarded_duration{}
  , min_timecode_in_file{}
  , max_timecode_in_file{-1}
  , min_timecode_in_cluster{-1}
  , max_timecode_in_cluster{-1}
  , frame_field_number{1}
  , first_video_keyframe_seen{}
  , out{}
  , current_split_point(split_points.begin())
  , discarding{}
  , splitting_and_processed_fully{}
  , debug_splitting{"cluster_helper|splitting"}
  , debug_packets{  "cluster_helper|cluster_helper_packets"}
  , debug_duration{ "cluster_helper|cluster_helper_duration"}
  , debug_rendering{"cluster_helper|cluster_helper_rendering"}
{
}

cluster_helper_c::impl_t::~impl_t() {
  delete cluster;
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

KaxCluster *
cluster_helper_c::get_cluster() {
  return m->cluster;
}

int64_t
cluster_helper_c::get_first_timecode_in_file()
  const {
  return m->first_timecode_in_file;
}

int64_t
cluster_helper_c::get_first_timecode_in_part()
  const {
  return m->first_timecode_in_part;
}

int64_t
cluster_helper_c::get_max_timecode_in_file()
  const {
  return m->max_timecode_in_file;
}

int
cluster_helper_c::get_packet_count()
  const {
  return m->packets.size();
}

bool
cluster_helper_c::splitting()
  const {
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
cluster_helper_c::render_before_adding_if_necessary(packet_cptr &packet) {
  int64_t timecode        = get_timecode();
  int64_t timecode_delay  = (   (packet->assigned_timecode > m->max_timecode_in_cluster)
                             || (-1 == m->max_timecode_in_cluster))                       ? packet->assigned_timecode : m->max_timecode_in_cluster;
  timecode_delay         -= (   (-1 == m->min_timecode_in_cluster)
                             || (packet->assigned_timecode < m->min_timecode_in_cluster)) ? packet->assigned_timecode : m->min_timecode_in_cluster;
  timecode_delay          = (int64_t)(timecode_delay / g_timecode_scale);

  mxdebug_if(m->debug_packets,
             boost::format("cluster_helper_c::add_packet(): new packet { source %1%/%2% "
                           "timecode: %3% duration: %4% bref: %5% fref: %6% assigned_timecode: %7% timecode_delay: %8% }\n")
             % packet->source->m_ti.m_id % packet->source->m_ti.m_fname % packet->timecode          % packet->duration
             % packet->bref              % packet->fref                 % packet->assigned_timecode % format_timecode(timecode_delay));

  bool is_video_keyframe = (packet->source == g_video_packetizer) && packet->is_key_frame();
  bool do_render         = (std::numeric_limits<int16_t>::max() < timecode_delay)
                        || (std::numeric_limits<int16_t>::min() > timecode_delay)
                        || (   (std::max<int64_t>(0, m->min_timecode_in_cluster) > m->previous_cluster_tc)
                            && (packet->assigned_timecode                        > m->min_timecode_in_cluster)
                            && (!g_video_packetizer || !is_video_keyframe || m->first_video_keyframe_seen)
                            && (   (packet->gap_following && !m->packets.empty())
                                || ((packet->assigned_timecode - timecode) > g_max_ns_per_cluster)
                                || is_video_keyframe));

  if (is_video_keyframe)
    m->first_video_keyframe_seen = true;

  mxdebug_if(m->debug_rendering,
             boost::format("render check cur_tc %9% min_tc_ic %1% prev_cl_tc %2% test %3% is_vid_and_key %4% tc_delay %5% gap_following_and_not_empty %6% cur_tc>min_tc_ic %8% first_video_key_seen %10% do_render %7%\n")
             % m->min_timecode_in_cluster % m->previous_cluster_tc % (std::max<int64_t>(0, m->min_timecode_in_cluster) > m->previous_cluster_tc) % is_video_keyframe
             % timecode_delay % (packet->gap_following && !m->packets.empty()) % do_render % (packet->assigned_timecode > m->min_timecode_in_cluster) % packet->assigned_timecode % m->first_video_keyframe_seen);

  if (!do_render)
    return;

  render();
  prepare_new_cluster();
}

void
cluster_helper_c::render_after_adding_if_necessary(packet_cptr &packet) {
  // Render the cluster if it is full (according to my many criteria).
  auto timecode = get_timecode();
  if (   ((packet->assigned_timecode - timecode) > g_max_ns_per_cluster)
      || (m->packets.size()                      > static_cast<size_t>(g_max_blocks_per_cluster))
      || (get_cluster_content_size()             > 1500000)) {
    render();
    prepare_new_cluster();
  }
}

void
cluster_helper_c::split_if_necessary(packet_cptr &packet) {
  if (   !splitting()
      || (m->split_points.end() == m->current_split_point)
      || (g_file_num > g_split_max_num_files)
      || !packet->is_key_frame()
      || (   (packet->source->get_track_type() != track_video)
          && g_video_packetizer))
    return;

  bool split_now = false;

  // Maybe we want to start a new file now.
  if (split_point_c::size == m->current_split_point->m_type) {
    int64_t additional_size = 0;

    if (!m->packets.empty())
      // Cluster + Cluster timecode: roughly 21 bytes. Add all frame sizes & their overheaders, too.
      additional_size = 21 + boost::accumulate(m->packets, 0, [](size_t size, const packet_cptr &p) { return size + p->data->get_size() + (p->is_key_frame() ? 10 : p->is_p_frame() ? 13 : 16); });

    additional_size += 18 * m->num_cue_elements;

    mxdebug_if(m->debug_splitting,
               boost::format("cluster_helper split decision: header_overhead: %1%, additional_size: %2%, bytes_in_file: %3%, sum: %4%\n")
               % m->header_overhead % additional_size % m->bytes_in_file % (m->header_overhead + additional_size + m->bytes_in_file));
    if ((m->header_overhead + additional_size + m->bytes_in_file) >= m->current_split_point->m_point)
      split_now = true;

  } else if (   (split_point_c::duration == m->current_split_point->m_type)
             && (0 <= m->first_timecode_in_file)
             && (packet->assigned_timecode - m->first_timecode_in_file) >= m->current_split_point->m_point)
    split_now = true;

  else if (   (   (split_point_c::timecode == m->current_split_point->m_type)
               || (split_point_c::parts    == m->current_split_point->m_type))
           && (packet->assigned_timecode >= m->current_split_point->m_point))
    split_now = true;

  else if (   (   (split_point_c::frame_field       == m->current_split_point->m_type)
               || (split_point_c::parts_frame_field == m->current_split_point->m_type))
           && (m->frame_field_number >= m->current_split_point->m_point))
    split_now = true;

  if (!split_now)
    return;

  split(packet);
}

void
cluster_helper_c::split(packet_cptr &packet) {
  render();

  m->num_cue_elements = 0;

  bool create_new_file       = m->current_split_point->m_create_new_file;
  bool previously_discarding = m->discarding;

  mxdebug_if(m->debug_splitting, boost::format("Splitting: splitpoint %1% reached before timecode %2%, create new? %3%.\n") % m->current_split_point->str() % format_timecode(packet->assigned_timecode) % create_new_file);

  finish_file(false, create_new_file, previously_discarding);

  if (m->current_split_point->m_use_once) {
    if (   m->current_split_point->m_discard
        && (   (split_point_c::parts             == m->current_split_point->m_type)
            || (split_point_c::parts_frame_field == m->current_split_point->m_type))
        && (m->split_points.end() == (m->current_split_point + 1))) {
      mxdebug_if(m->debug_splitting, boost::format("Splitting: Last part in 'parts:' splitting mode finished\n"));
      m->splitting_and_processed_fully = true;
    }

    m->discarding = m->current_split_point->m_discard;
    ++m->current_split_point;
  }

  if (create_new_file) {
    create_next_output_file();
    if (g_no_linking) {
      m->previous_cluster_tc = -1;
      m->timecode_offset = g_video_packetizer ? m->max_video_timecode_rendered : packet->assigned_timecode;
    }

    m->bytes_in_file          =  0;
    m->first_timecode_in_file = -1;
    m->max_timecode_in_file   = -1;
    m->min_timecode_in_file.reset();
  }

  m->first_timecode_in_part = -1;

  handle_discarded_duration(create_new_file, previously_discarding);

  prepare_new_cluster();
}

void
cluster_helper_c::add_packet(packet_cptr packet) {
  if (!m->cluster)
    prepare_new_cluster();

  packet->normalize_timecodes();
  render_before_adding_if_necessary(packet);
  split_if_necessary(packet);

  m->packets.push_back(packet);
  m->cluster_content_size += packet->data->get_size();

  if (packet->assigned_timecode > m->max_timecode_in_cluster)
    m->max_timecode_in_cluster = packet->assigned_timecode;

  if ((-1 == m->min_timecode_in_cluster) || (packet->assigned_timecode < m->min_timecode_in_cluster))
    m->min_timecode_in_cluster = packet->assigned_timecode;

  render_after_adding_if_necessary(packet);

  if (g_video_packetizer == packet->source)
    ++m->frame_field_number;
}

int64_t
cluster_helper_c::get_timecode() {
  return m->packets.empty() ? 0 : m->packets.front()->assigned_timecode;
}

void
cluster_helper_c::prepare_new_cluster() {
  delete m->cluster;

  m->cluster              = new kax_cluster_c;
  m->cluster_content_size = 0;
  m->packets.clear();

  m->cluster->SetParent(*g_kax_segment);
  m->cluster->SetPreviousTimecode(std::max<int64_t>(0, m->previous_cluster_tc), (int64_t)g_timecode_scale);
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
             boost::format("cluster_helper::set_duration: block_duration %1% rounded duration %2% def_duration %3% use_durations %4% rg->m_duration_mandatory %5%\n")
             % block_duration % RND_TIMECODE_SCALE(block_duration) % def_duration % (g_use_durations ? 1 : 0) % (rg->m_duration_mandatory ? 1 : 0));

  if (rg->m_duration_mandatory) {
    if (   (0 == block_duration)
        || (   (0 < block_duration)
            && (block_duration != (static_cast<int64_t>(rg->m_durations.size()) * def_duration))))
      group->set_block_duration(RND_TIMECODE_SCALE(block_duration));

  } else if (   (   g_use_durations
                 || (0 < def_duration))
             && (0 < block_duration)
             && (RND_TIMECODE_SCALE(block_duration) != RND_TIMECODE_SCALE(rg->m_durations.size() * def_duration)))
    group->set_block_duration(RND_TIMECODE_SCALE(block_duration));
}

bool
cluster_helper_c::must_duration_be_set(render_groups_c *rg,
                                       packet_cptr &new_packet) {
  size_t i;
  int64_t block_duration = 0;
  int64_t def_duration   = rg->m_source->get_track_default_duration();

  for (i = 0; rg->m_durations.size() > i; ++i)
    block_duration += rg->m_durations[i];
  block_duration += new_packet->get_duration();

  if (rg->m_duration_mandatory || new_packet->duration_mandatory) {
    if (   (0 == block_duration)
        || (   (0 < block_duration)
            && (block_duration != ((static_cast<int64_t>(rg->m_durations.size()) + 1) * def_duration))))
      return true;

  } else if (   (   g_use_durations
                 || (0 < def_duration))
             && (0 < block_duration)
             && (RND_TIMECODE_SCALE(block_duration) != RND_TIMECODE_SCALE((rg->m_durations.size() + 1) * def_duration)))
    return true;

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
  KaxCues cues;
  cues.SetGlobalTimecodeScale(g_timecode_scale);

  bool use_simpleblock    = !hack_engaged(ENGAGE_NO_SIMPLE_BLOCKS);

  LacingType lacing_type  = hack_engaged(ENGAGE_LACING_XIPH) ? LACING_XIPH : hack_engaged(ENGAGE_LACING_EBML) ? LACING_EBML : LACING_AUTO;

  int64_t min_cl_timecode = std::numeric_limits<int64_t>::max();
  int64_t max_cl_timecode = 0;

  int elements_in_cluster = 0;
  bool added_to_cues      = false;

  // Splitpoint stuff
  if ((-1 == m->header_overhead) && splitting())
    m->header_overhead = m->out->getFilePointer() + g_tags_size;

  // Make sure that we don't have negative/wrapped around timecodes in the output file.
  // Can happend when we're splitting; so adjust timecode_offset accordingly.
  m->timecode_offset       = boost::accumulate(m->packets, m->timecode_offset, [](int64_t a, const packet_cptr &p) { return std::min(a, p->assigned_timecode); });
  int64_t timecode_offset = m->timecode_offset + get_discarded_duration();

  for (auto &pack : m->packets) {
    generic_packetizer_c *source = pack->source;
    bool has_codec_state         = !!pack->codec_state;

    if (g_video_packetizer == source)
      m->max_video_timecode_rendered = std::max(pack->assigned_timecode + pack->get_duration(), m->max_video_timecode_rendered);

    if (discarding()) {
      if (-1 == m->first_discarded_timecode)
        m->first_discarded_timecode = pack->assigned_timecode;
      m->last_discarded_timecode_and_duration = std::max(m->last_discarded_timecode_and_duration, pack->assigned_timecode + pack->get_duration());
      continue;
    }

    if (source->contains_gap())
      m->cluster->SetSilentTrackUsed();

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

    min_cl_timecode                        = std::min(pack->assigned_timecode, min_cl_timecode);
    max_cl_timecode                        = std::max(pack->assigned_timecode, max_cl_timecode);

    DataBuffer *data_buffer                = new DataBuffer((binary *)pack->data->get_buffer(), pack->data->get_size());

    KaxTrackEntry &track_entry             = static_cast<KaxTrackEntry &>(*source->get_track_entry());

    kax_block_blob_c *previous_block_group = !render_group->m_groups.empty() ? render_group->m_groups.back().get() : nullptr;
    kax_block_blob_c *new_block_group      = previous_block_group;

    if (!pack->is_key_frame() || has_codec_state || pack->has_discard_padding())
      render_group->m_more_data = false;

    if (!render_group->m_more_data) {
      set_duration(render_group);
      render_group->m_durations.clear();
      render_group->m_duration_mandatory = false;

      BlockBlobType this_block_blob_type
        = !use_simpleblock                         ? BLOCK_BLOB_NO_SIMPLE
        : must_duration_be_set(render_group, pack) ? BLOCK_BLOB_NO_SIMPLE
        : !pack->data_adds.empty()                 ? BLOCK_BLOB_NO_SIMPLE
        : has_codec_state                          ? BLOCK_BLOB_NO_SIMPLE
        : pack->has_discard_padding()              ? BLOCK_BLOB_NO_SIMPLE
        :                                            BLOCK_BLOB_ALWAYS_SIMPLE;

      render_group->m_groups.push_back(kax_block_blob_cptr(new kax_block_blob_c(this_block_blob_type)));
      new_block_group = render_group->m_groups.back().get();
      m->cluster->AddBlockBlob(new_block_group);
      new_block_group->SetParent(*m->cluster);

      added_to_cues = false;
    }

    // Now put the packet into the cluster.
    render_group->m_more_data = new_block_group->add_frame_auto(track_entry, pack->assigned_timecode - timecode_offset, *data_buffer, lacing_type,
                                                                pack->has_bref() ? pack->bref - timecode_offset : -1,
                                                                pack->has_fref() ? pack->fref - timecode_offset : -1);

    if (has_codec_state) {
      KaxBlockGroup &bgroup = (KaxBlockGroup &)*new_block_group;
      KaxCodecState *cstate = new KaxCodecState;
      bgroup.PushElement(*cstate);
      cstate->CopyBuffer(pack->codec_state->get_buffer(), pack->codec_state->get_size());
    }

    if (-1 == m->first_timecode_in_file)
      m->first_timecode_in_file = pack->assigned_timecode;
    if (-1 == m->first_timecode_in_part)
      m->first_timecode_in_part = pack->assigned_timecode;

    m->min_timecode_in_file      = std::min(timecode_c::ns(pack->assigned_timecode),        m->min_timecode_in_file.value_or_max());
    m->max_timecode_in_file      = std::max(pack->assigned_timecode,                        m->max_timecode_in_file);
    m->max_timecode_and_duration = std::max(pack->assigned_timecode + pack->get_duration(), m->max_timecode_and_duration);

    if (!pack->is_key_frame() || !track_entry.LacingEnabled())
      render_group->m_more_data = false;

    render_group->m_durations.push_back(pack->get_unmodified_duration());
    render_group->m_duration_mandatory |= pack->duration_mandatory;

    cues_c::get().set_duration_for_id_timecode(source->get_track_num(), pack->assigned_timecode - timecode_offset, pack->get_duration());

    if (new_block_group) {
      // Set the reference priority if it was wanted.
      if ((0 < pack->ref_priority) && new_block_group->replace_simple_by_group())
        GetChild<KaxReferencePriority>(*new_block_group).SetValue(pack->ref_priority);

      // Handle BlockAdditions if needed
      if (!pack->data_adds.empty() && new_block_group->ReplaceSimpleByGroup()) {
        KaxBlockAdditions &additions = AddEmptyChild<KaxBlockAdditions>(*new_block_group);

        size_t data_add_idx;
        for (data_add_idx = 0; pack->data_adds.size() > data_add_idx; ++data_add_idx) {
          auto &block_more = AddEmptyChild<KaxBlockMore>(additions);
          GetChild<KaxBlockAddID     >(block_more).SetValue(data_add_idx + 1);
          GetChild<KaxBlockAdditional>(block_more).CopyBuffer((binary *)pack->data_adds[data_add_idx]->get_buffer(), pack->data_adds[data_add_idx]->get_size());
        }
      }

      if (pack->has_discard_padding())
        GetChild<KaxDiscardPadding>(*new_block_group).SetValue(pack->discard_padding.to_ns());
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

    m->track_statistics[ source->get_uid() ].process(*pack);
  }

  if (!discarding()) {
    if (0 < elements_in_cluster) {
      for (auto &rg : render_groups)
        set_duration(rg.get());

      m->cluster->SetPreviousTimecode(min_cl_timecode - timecode_offset - 1, (int64_t)g_timecode_scale);
      m->cluster->set_min_timecode(min_cl_timecode - timecode_offset);
      m->cluster->set_max_timecode(max_cl_timecode - timecode_offset);

      m->cluster->Render(*m->out, cues);
      m->bytes_in_file += m->cluster->ElementSize();

      if (g_kax_sh_cues)
        g_kax_sh_cues->IndexThis(*m->cluster, *g_kax_segment);

      m->previous_cluster_tc = m->cluster->GlobalTimecode();

      cues_c::get().postprocess_cues(cues, *m->cluster);

    } else
      m->previous_cluster_tc = -1;
  }

  m->min_timecode_in_cluster = -1;
  m->max_timecode_in_cluster = -1;

  m->cluster->delete_non_blocks();

  return 1;
}

bool
cluster_helper_c::add_to_cues_maybe(packet_cptr &pack) {
  auto &source  = *pack->source;
  auto strategy = source.get_cue_creation();

  // Update the cues (index table) either if cue entries for I frames were requested and this is an I frame...
  bool add = (CUE_STRATEGY_IFRAMES == strategy) && pack->is_key_frame();

  // ... or if a codec state change is present ...
  add = add || !!pack->codec_state;

  // ... or if the user requested entries for all frames ...
  add = add || (CUE_STRATEGY_ALL == strategy);

  // ... or if this is an audio track, there is no video track and the
  // last cue entry was created more than 2s ago.
  add = add || (   (CUE_STRATEGY_SPARSE == strategy)
                && (track_audio         == source.get_track_type())
                && !g_video_packetizer
                && (   (0 > source.get_last_cue_timecode())
                    || ((pack->assigned_timecode - source.get_last_cue_timecode()) >= 2000000000)));

  if (!add)
    return false;

  source.set_last_cue_timecode(pack->assigned_timecode);

  ++m->num_cue_elements;
  g_cue_writing_requested = 1;

  return true;
}

int64_t
cluster_helper_c::get_duration()
  const {
  auto result = m->max_timecode_and_duration - m->min_timecode_in_file.to_ns(0) - m->discarded_duration;
  mxdebug_if(m->debug_duration,
             boost::format("cluster_helper_c::get_duration(): max_tc_and_dur %1% - min_tc_in_file %2% - discarded_duration %3% = %4% ; first_tc_in_file = %5%\n")
             % m->max_timecode_and_duration % m->min_timecode_in_file.to_ns(0) % m->discarded_duration % result % m->first_timecode_in_file);
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
               boost::format("RESETTING discarded duration of %1%, create_new_file %2% previously_discarding %3% m->discarding %4%\n")
               % format_timecode(m->discarded_duration) % create_new_file % previously_discarding % m->discarding);
    m->discarded_duration = 0;

  } else if (previously_discarding && !m->discarding) {
    auto diff              = m->last_discarded_timecode_and_duration - std::max<int64_t>(m->first_discarded_timecode, 0);
    m->discarded_duration += diff;

    mxdebug_if(m->debug_splitting,
               boost::format("ADDING to discarded duration TC at %1% / %2% diff %3% new total %4% create_new_file %5% previously_discarding %6% m->discarding %7%\n")
               % format_timecode(m->first_discarded_timecode) % format_timecode(m->last_discarded_timecode_and_duration) % format_timecode(diff) % format_timecode(m->discarded_duration)
               % create_new_file % previously_discarding % m->discarding);
  } else
    mxdebug_if(m->debug_splitting,
               boost::format("KEEPING discarded duration at %1%, create_new_file %2% previously_discarding %3% m->discarding %4%\n")
               % format_timecode(m->discarded_duration) % create_new_file % previously_discarding % m->discarding);

  m->first_discarded_timecode             = -1;
  m->last_discarded_timecode_and_duration =  0;
}

void
cluster_helper_c::add_split_point(const split_point_c &split_point) {
  m->split_points.push_back(split_point);
  m->current_split_point = m->split_points.begin();
  m->discarding          = m->current_split_point->m_discard;

  if (0 == m->current_split_point->m_point)
    ++m->current_split_point;
}

bool
cluster_helper_c::split_mode_produces_many_files()
  const {
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
             boost::format("Split points:%1%\n")
             % boost::accumulate(m->split_points, std::string(""), [](std::string const &accu, split_point_c const &point) { return accu + " " + point.str(); }));
}

void
cluster_helper_c::create_tags_for_track_statistics(KaxTags &tags,
                                                   std::string const &writing_app,
                                                   boost::posix_time::ptime const &writing_date) {
  auto writing_date_str = !writing_date.is_not_a_date_time() ? mtx::date_time::to_string(writing_date, "%Y-%m-%d %H:%M:%S") : "1970-01-01 00:00:00";

  for (auto const &ptzr : g_packetizers) {
    auto track_uid    = ptzr.packetizer->get_uid();
    auto const &stats = m->track_statistics[track_uid];
    auto bps          = stats.get_bits_per_second();
    auto duration     = stats.get_duration();

    mtx::tags::remove_simple_tags_for<KaxTagTrackUID>(tags, track_uid, "BPS");
    mtx::tags::remove_simple_tags_for<KaxTagTrackUID>(tags, track_uid, "DURATION");
    mtx::tags::remove_simple_tags_for<KaxTagTrackUID>(tags, track_uid, "NUMBER_OF_FRAMES");
    mtx::tags::remove_simple_tags_for<KaxTagTrackUID>(tags, track_uid, "NUMBER_OF_BYTES");

    auto tag = mtx::tags::find_tag_for<KaxTagTrackUID>(tags, track_uid, mtx::tags::Movie, true);

    mtx::tags::set_target_type(*tag, mtx::tags::Movie, "MOVIE");

    mtx::tags::set_simple(*tag, "BPS",              to_string(bps ? *bps : 0));
    mtx::tags::set_simple(*tag, "DURATION",         format_timecode(duration ? *duration : 0));
    mtx::tags::set_simple(*tag, "NUMBER_OF_FRAMES", to_string(stats.get_num_frames()));
    mtx::tags::set_simple(*tag, "NUMBER_OF_BYTES",  to_string(stats.get_num_bytes()));

    mtx::tags::set_simple(*tag, "_STATISTICS_WRITING_APP",      writing_app);
    mtx::tags::set_simple(*tag, "_STATISTICS_WRITING_DATE_UTC", writing_date_str);
    mtx::tags::set_simple(*tag, "_STATISTICS_TAGS",             "BPS DURATION NUMBER_OF_FRAMES NUMBER_OF_BYTES");
  }

  m->track_statistics.clear();
}

cluster_helper_c *g_cluster_helper = nullptr;
