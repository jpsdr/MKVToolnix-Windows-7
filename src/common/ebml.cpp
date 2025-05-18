/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions that need libebml/libmatroska

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <unordered_set>

#include <ebml/EbmlDummy.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSegment.h>

#include "common/date_time.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/memory.h"
#include "common/unique_numbers.h"
#include "common/version.h"

namespace {

void
remove_elements_recursively_if(libebml::EbmlMaster &master,
                               std::function<bool(libebml::EbmlElement &child)> predicate) {
  auto idx = 0u;

  while (idx < master.ListSize()) {
    auto child = master[idx];

    if (predicate(*child)) {
      delete child;
      master.Remove(idx);
      continue;
    }

    if (dynamic_cast<libebml::EbmlMaster *>(child))
      remove_elements_recursively_if(*static_cast<libebml::EbmlMaster *>(child), predicate);

    ++idx;
  }
}

}

libebml::EbmlElement *
empty_ebml_master(libebml::EbmlElement *e) {
  libebml::EbmlMaster *m;

  m = dynamic_cast<libebml::EbmlMaster *>(e);
  if (!m)
    return e;

  while (m->ListSize() > 0) {
    delete (*m)[0];
    m->Remove(0);
  }

  return m;
}

libebml::EbmlElement *
create_ebml_element(const libebml::EbmlCallbacks &callbacks,
                    const libebml::EbmlId &id) {
  const libebml::EbmlSemanticContext &context_e = EBML_INFO_CONTEXT(callbacks);
  size_t i;

//   if (id == get_ebml_id(*parent))
//     return empty_ebml_master(&parent->Generic().Create());

  if (EBML_CTX_SIZE(context_e) == 0)
    return nullptr;

  const auto &context = EBML_INFO_CONTEXT(static_cast<ebml_callbacks_master_c const &>(callbacks));

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return empty_ebml_master(&EBML_SEM_CREATE(EBML_CTX_IDX(context,i)));

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    libebml::EbmlElement *e;

    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;

    e = create_ebml_element(EBML_CTX_IDX_INFO(context, i), id);
    if (e)
      return e;
  }

  return nullptr;
}

libebml::EbmlElement &
create_ebml_element(const libebml::EbmlSemantic &semantic) {
#if LIBEBML_VERSION < 0x020000
  return semantic.Create();
#else
  return EBML_INFO_CREATE(semantic.GetCallbacks());
#endif
}

static libebml::EbmlCallbacks const *
do_find_ebml_callbacks(libebml::EbmlCallbacks const &base,
                       libebml::EbmlId const &id) {
  const libebml::EbmlSemanticContext &context_e = EBML_INFO_CONTEXT(base);
  const libebml::EbmlCallbacks *result;
  size_t i;

  if (EBML_INFO_ID(base) == id)
    return &base;

  if (EBML_CTX_SIZE(context_e) == 0)
    return nullptr;

  const auto &context = EBML_INFO_CONTEXT(static_cast<ebml_callbacks_master_c const &>(base));

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &EBML_CTX_IDX_INFO(context, i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = do_find_ebml_callbacks(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

libebml::EbmlCallbacks const *
find_ebml_callbacks(libebml::EbmlCallbacks const &base,
                    libebml::EbmlId const &id) {
  static std::unordered_map<uint32_t, libebml::EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_callbacks(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

static libebml::EbmlCallbacks const *
do_find_ebml_callbacks(libebml::EbmlCallbacks const &base,
                       char const *debug_name) {
  const libebml::EbmlSemanticContext &context_e = EBML_INFO_CONTEXT(base);
  const libebml::EbmlCallbacks *result;
  size_t i;

  if (!strcmp(debug_name, EBML_INFO_NAME(base)))
    return &base;

  if (EBML_CTX_SIZE(context_e) == 0)
    return nullptr;

  const auto &context = EBML_INFO_CONTEXT(static_cast<ebml_callbacks_master_c const &>(base));

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (!strcmp(debug_name, EBML_INFO_NAME(EBML_CTX_IDX_INFO(context, i))))
      return &EBML_CTX_IDX_INFO(context, i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = do_find_ebml_callbacks(EBML_CTX_IDX_INFO(context, i), debug_name);
    if (result)
      return result;
  }

  return nullptr;
}

libebml::EbmlCallbacks const *
find_ebml_callbacks(libebml::EbmlCallbacks const &base,
                    char const *debug_name) {
  static std::unordered_map<std::string, libebml::EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(debug_name);
  if (itr != s_cache.end())
    return itr->second;

  auto result         = do_find_ebml_callbacks(base, debug_name);
  s_cache[debug_name] = result;

  return result;
}

static libebml::EbmlCallbacks const *
do_find_ebml_parent_callbacks(libebml::EbmlCallbacks const &base,
                              libebml::EbmlId const &id) {
  const libebml::EbmlSemanticContext &context_e = EBML_INFO_CONTEXT(base);
  const libebml::EbmlCallbacks *result;
  size_t i;

  if (EBML_CTX_SIZE(context_e) == 0)
    return nullptr;

  const auto &context = EBML_INFO_CONTEXT(static_cast<ebml_callbacks_master_c const &>(base));

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &base;

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = do_find_ebml_parent_callbacks(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

libebml::EbmlCallbacks const *
find_ebml_parent_callbacks(libebml::EbmlCallbacks const &base,
                           libebml::EbmlId const &id) {
  static std::unordered_map<uint32_t, libebml::EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_parent_callbacks(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

static libebml::EbmlSemantic const *
do_find_ebml_semantic(libebml::EbmlCallbacks const &base,
                      libebml::EbmlId const &id) {
  const libebml::EbmlSemanticContext &context_e = EBML_INFO_CONTEXT(base);
  const libebml::EbmlSemantic *result;
  size_t i;

  if (EBML_CTX_SIZE(context_e) == 0)
    return nullptr;

  const auto &context = EBML_INFO_CONTEXT(static_cast<ebml_callbacks_master_c const &>(base));

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return &EBML_CTX_IDX(context,i);

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;
    result = do_find_ebml_semantic(EBML_CTX_IDX_INFO(context, i), id);
    if (result)
      return result;
  }

  return nullptr;
}

libebml::EbmlSemantic const *
find_ebml_semantic(libebml::EbmlCallbacks const &base,
                   libebml::EbmlId const &id) {
  static std::unordered_map<uint32_t, libebml::EbmlSemantic const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_semantic(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

libebml::EbmlMaster *
sort_ebml_master(libebml::EbmlMaster *m) {
  if (!m)
    return m;

  int first_element = -1;
  int first_master  = -1;
  size_t i;
  for (i = 0; i < m->ListSize(); i++) {
    if (dynamic_cast<libebml::EbmlMaster *>((*m)[i]) && (-1 == first_master))
      first_master = i;
    else if (!dynamic_cast<libebml::EbmlMaster *>((*m)[i]) && (-1 != first_master) && (-1 == first_element))
      first_element = i;
    if ((first_master != -1) && (first_element != -1))
      break;
  }

  if (first_master == -1)
    return m;

  while (first_element != -1) {
    libebml::EbmlElement *e = (*m)[first_element];
    m->Remove(first_element);
    m->InsertElement(*e, first_master);
    first_master++;
    for (first_element++; first_element < static_cast<int>(m->ListSize()); first_element++)
      if (!dynamic_cast<libebml::EbmlMaster *>((*m)[first_element]))
        break;
    if (first_element >= static_cast<int>(m->ListSize()))
      first_element = -1;
  }

  for (i = 0; i < m->ListSize(); i++)
    if (dynamic_cast<libebml::EbmlMaster *>((*m)[i]))
      sort_ebml_master(dynamic_cast<libebml::EbmlMaster *>((*m)[i]));

  return m;
}

void
move_children(libebml::EbmlMaster &source,
              libebml::EbmlMaster &destination) {
  for (auto child : source)
    destination.PushElement(*child);
}

// ------------------------------------------------------------------------

int64_t
kt_get_default_duration(libmatroska::KaxTrackEntry &track) {
  return find_child_value<libmatroska::KaxTrackDefaultDuration>(track);
}

int64_t
kt_get_number(libmatroska::KaxTrackEntry &track) {
  return find_child_value<libmatroska::KaxTrackNumber>(track);
}

int64_t
kt_get_uid(libmatroska::KaxTrackEntry &track) {
  return find_child_value<libmatroska::KaxTrackUID>(track);
}

std::string
kt_get_codec_id(libmatroska::KaxTrackEntry &track) {
  return find_child_value<libmatroska::KaxCodecID>(track);
}

int
kt_get_max_blockadd_id(libmatroska::KaxTrackEntry &track) {
  return find_child_value<libmatroska::KaxMaxBlockAdditionID>(track);
}

int
kt_get_a_channels(libmatroska::KaxTrackEntry &track) {
  auto audio = find_child<libmatroska::KaxTrackAudio>(track);
  return audio ? find_child_value<libmatroska::KaxAudioChannels>(audio, 1u) : 1;
}

double
kt_get_a_sfreq(libmatroska::KaxTrackEntry &track) {
  auto audio = find_child<libmatroska::KaxTrackAudio>(track);
  return audio ? find_child_value<libmatroska::KaxAudioSamplingFreq>(audio, 8000.0) : 8000.0;
}

double
kt_get_a_osfreq(libmatroska::KaxTrackEntry &track) {
  auto audio = find_child<libmatroska::KaxTrackAudio>(track);
  return audio ? find_child_value<libmatroska::KaxAudioOutputSamplingFreq>(audio, 8000.0) : 8000.0;
}

int
kt_get_a_bps(libmatroska::KaxTrackEntry &track) {
  auto audio = find_child<libmatroska::KaxTrackAudio>(track);
  return audio ? find_child_value<libmatroska::KaxAudioBitDepth, int>(audio, -1) : -1;
}

int
kt_get_v_pixel_width(libmatroska::KaxTrackEntry &track) {
  auto video = find_child<libmatroska::KaxTrackVideo>(track);
  return video ? find_child_value<libmatroska::KaxVideoPixelWidth>(video) : 0;
}

int
kt_get_v_pixel_height(libmatroska::KaxTrackEntry &track) {
  auto video = find_child<libmatroska::KaxTrackVideo>(track);
  return video ? find_child_value<libmatroska::KaxVideoPixelHeight>(video) : 0;
}

libebml::EbmlElement *
find_ebml_element_by_id(libebml::EbmlMaster *master,
                        const libebml::EbmlId &id) {
  for (auto child : *master)
    if (get_ebml_id(*child) == id)
      return child;

  return nullptr;
}

std::pair<libebml::EbmlMaster *, size_t>
find_element_in_master(libebml::EbmlMaster *master,
                       libebml::EbmlElement *element_to_find) {
  if (!master || !element_to_find)
    return std::make_pair<libebml::EbmlMaster *, size_t>(nullptr, 0);

  auto &elements = master->GetElementList();
  auto itr       = std::find(elements.begin(), elements.end(), element_to_find);

  if (itr != elements.end())
    return std::make_pair(master, std::distance(elements.begin(), itr));

  for (auto &sub_element : elements) {
    auto sub_master = dynamic_cast<libebml::EbmlMaster *>(sub_element);
    if (!sub_master)
      continue;

    auto result = find_element_in_master(sub_master, element_to_find);
    if (result.first)
      return result;
  }

  return std::make_pair<libebml::EbmlMaster *, size_t>(nullptr, 0);
}

static std::unordered_map<uint32_t, bool> const &
get_deprecated_elements_by_id() {
  static std::unordered_map<uint32_t, bool> s_elements;

  if (!s_elements.empty())
    return s_elements;

  s_elements[EBML_ID(libmatroska::KaxAudioPosition).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxBlockVirtual).GetValue()]               = true;
  s_elements[EBML_ID(libmatroska::KaxClusterSilentTrackNumber).GetValue()]   = true;
  s_elements[EBML_ID(libmatroska::KaxClusterSilentTracks).GetValue()]        = true;
  s_elements[EBML_ID(libmatroska::KaxCodecDecodeAll).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxCodecDownloadURL).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxCodecInfoURL).GetValue()]               = true;
  s_elements[EBML_ID(libmatroska::KaxCodecSettings).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxContentSigAlgo).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxContentSigHashAlgo).GetValue()]         = true;
  s_elements[EBML_ID(libmatroska::KaxContentSigKeyID).GetValue()]            = true;
  s_elements[EBML_ID(libmatroska::KaxContentSignature).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxCueRefCluster).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxCueRefCodecState).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxCueRefNumber).GetValue()]               = true;
  s_elements[EBML_ID(libmatroska::KaxEncryptedBlock).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxFileReferral).GetValue()]               = true;
  s_elements[EBML_ID(libmatroska::KaxFileUsedEndTime).GetValue()]            = true;
  s_elements[EBML_ID(libmatroska::KaxFileUsedStartTime).GetValue()]          = true;
  s_elements[EBML_ID(libmatroska::KaxOldStereoMode).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxReferenceFrame).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxReferenceOffset).GetValue()]            = true;
  s_elements[EBML_ID(kax_reference_timestamp_c).GetValue()]                  = true;
  s_elements[EBML_ID(libmatroska::KaxReferenceVirtual).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxSliceBlockAddID).GetValue()]            = true;
  s_elements[EBML_ID(libmatroska::KaxSliceDelay).GetValue()]                 = true;
  s_elements[EBML_ID(libmatroska::KaxSliceDuration).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxSliceFrameNumber).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxSliceLaceNumber).GetValue()]            = true;
  s_elements[EBML_ID(libmatroska::KaxSlices).GetValue()]                     = true;
  s_elements[EBML_ID(libmatroska::KaxTagDefaultBogus).GetValue()]            = true;
  s_elements[EBML_ID(libmatroska::KaxTimeSlice).GetValue()]                  = true;
  s_elements[EBML_ID(libmatroska::KaxTrackAttachmentLink).GetValue()]        = true;
  s_elements[EBML_ID(libmatroska::KaxTrackMaxCache).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxTrackMinCache).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxTrackOffset).GetValue()]                = true;
  s_elements[EBML_ID(kax_track_timestamp_scale_c).GetValue()]                = true;
  s_elements[EBML_ID(libmatroska::KaxTrickMasterTrackSegmentUID).GetValue()] = true;
  s_elements[EBML_ID(libmatroska::KaxTrickMasterTrackUID).GetValue()]        = true;
  s_elements[EBML_ID(libmatroska::KaxTrickTrackFlag).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxTrickTrackSegmentUID).GetValue()]       = true;
  s_elements[EBML_ID(libmatroska::KaxTrickTrackUID).GetValue()]              = true;
  s_elements[EBML_ID(libmatroska::KaxVideoAspectRatio).GetValue()]           = true;
  s_elements[EBML_ID(libmatroska::KaxVideoFrameRate).GetValue()]             = true;
  s_elements[EBML_ID(libmatroska::KaxVideoGamma).GetValue()]                 = true;

  return s_elements;
}

template<typename T>
void
fix_elements_set_to_default_value_if_unset(libebml::EbmlElement *e) {
  static debugging_option_c s_debug{"fix_elements_in_master"};

  auto t = static_cast<T *>(e);

  if (!has_default_value(t) || t->ValueIsSet())
    return;

  mxdebug_if(s_debug,
             fmt::format("fix_elements_in_master: element has default, but value is no set; setting: ID {0:08x} name {1}\n",
                         get_ebml_id(*t).GetValue(), EBML_NAME(t)));
  t->SetValue(t->GetValue());
}

void
fix_elements_in_master(libebml::EbmlMaster *master) {
  static debugging_option_c s_debug{"fix_elements_in_master"};

  if (!master)
    return;

  auto callbacks = find_ebml_callbacks(EBML_INFO(libmatroska::KaxSegment), get_ebml_id(*master));
  if (!callbacks) {
    mxdebug_if(s_debug, fmt::format("fix_elements_in_master: No callbacks found for ID {0:08x}\n", get_ebml_id(*master).GetValue()));
    return;
  }

  std::unordered_map<uint32_t, bool> is_present;

  auto &deprecated_elements    = get_deprecated_elements_by_id();
  auto deprecated_elements_end = deprecated_elements.end();
  auto idx                     = 0u;

  // 1. Remove deprecated elements.
  // 2. Record which elements are already present.
  // 3. If the element has a default value and is currently unset, set
  //    to the default value.

  while (idx < master->ListSize()) {
    auto child    = (*master)[idx];
    auto child_id = get_ebml_id(*child).GetValue();
    auto itr      = deprecated_elements.find(child_id);

    if (itr != deprecated_elements_end) {
      delete child;
      master->Remove(idx);
      continue;

    }

    ++idx;
    is_present[child_id] = true;

    if (dynamic_cast<libebml::EbmlMaster *>(child))
      fix_elements_in_master(static_cast<libebml::EbmlMaster *>(child));

    else if (dynamic_cast<libebml::EbmlDate *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlDate>(child);

    else if (dynamic_cast<libebml::EbmlFloat *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlFloat>(child);

    else if (dynamic_cast<libebml::EbmlSInteger *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlSInteger>(child);

    else if (dynamic_cast<libebml::EbmlString *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlString>(child);

    else if (dynamic_cast<libebml::EbmlUInteger *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlUInteger>(child);

    else if (dynamic_cast<libebml::EbmlUnicodeString *>(child))
      fix_elements_set_to_default_value_if_unset<libebml::EbmlUnicodeString>(child);
  }

  // 4. Take care of certain mandatory elements without default values
  //    that we can provide sensible values for, e.g. create UIDs
  //    ourselves.

  // 4.1. Info
  if (dynamic_cast<libmatroska::KaxInfo *>(master)) {
    auto info_data = get_default_segment_info_data();

    if (!is_present[EBML_ID(libmatroska::KaxMuxingApp).GetValue()])
      add_empty_child<libmatroska::KaxMuxingApp>(master).SetValueUTF8(info_data.muxing_app);

    if (!is_present[EBML_ID(libmatroska::KaxWritingApp).GetValue()])
      add_empty_child<libmatroska::KaxWritingApp>(master).SetValueUTF8(info_data.writing_app);
  }

  // 4.2. Tracks
  else if (dynamic_cast<libmatroska::KaxTrackEntry *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxTrackUID).GetValue()])
      add_empty_child<libmatroska::KaxTrackUID>(master).SetValue(create_unique_number(UNIQUE_TRACK_IDS));
  }

  // 4.3. Chapters
  else if (dynamic_cast<libmatroska::KaxEditionEntry *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxEditionUID).GetValue()])
      add_empty_child<libmatroska::KaxEditionUID>(master).SetValue(create_unique_number(UNIQUE_EDITION_IDS));


  } else if (dynamic_cast<libmatroska::KaxChapterAtom *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxChapterUID).GetValue()])
      add_empty_child<libmatroska::KaxChapterUID>(master).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));

    if (!is_present[EBML_ID(libmatroska::KaxChapterTimeStart).GetValue()])
      add_empty_child<libmatroska::KaxChapterTimeStart>(master).SetValue(0);

  } else if (dynamic_cast<libmatroska::KaxChapterDisplay *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxChapterString).GetValue()])
      add_empty_child<libmatroska::KaxChapterString>(master).SetValueUTF8("");

  }

  // 4.4. Tags
  else if (dynamic_cast<libmatroska::KaxTag *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxTagTargets).GetValue()])
      fix_elements_in_master(&add_empty_child<libmatroska::KaxTagTargets>(master));

    else if (!is_present[EBML_ID(libmatroska::KaxTagSimple).GetValue()])
      fix_elements_in_master(&add_empty_child<libmatroska::KaxTagSimple>(master));

  } else if (dynamic_cast<libmatroska::KaxTagTargets *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxTagTargetTypeValue).GetValue()])
      add_empty_child<libmatroska::KaxTagTargetTypeValue>(master).SetValue(50); // = movie

  } else if (dynamic_cast<libmatroska::KaxTagSimple *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxTagName).GetValue()])
      add_empty_child<libmatroska::KaxTagName>(master).SetValueUTF8("");

  }

  // 4.5. Attachments
  else if (dynamic_cast<libmatroska::KaxAttached *>(master)) {
    if (!is_present[EBML_ID(libmatroska::KaxFileUID).GetValue()])
      add_empty_child<libmatroska::KaxFileUID>(master).SetValue(create_unique_number(UNIQUE_ATTACHMENT_IDS));
  }
}

void
fix_mandatory_elements(libebml::EbmlElement *master) {
  if (dynamic_cast<libebml::EbmlMaster *>(master))
    fix_elements_in_master(static_cast<libebml::EbmlMaster *>(master));
}

void
remove_voids_from_master(libebml::EbmlElement *element) {
  auto master = dynamic_cast<libebml::EbmlMaster *>(element);
  if (master)
    delete_children<libebml::EbmlVoid>(master);
}

int
write_ebml_element_head(mm_io_c &out,
                        libebml::EbmlId const &id,
                        int64_t content_size) {
  int id_size    = id.GetLength();
  int coded_size = libebml::CodedSizeLength(content_size, 0);
  uint8_t buffer[4 + 8];

  id.Fill(buffer);
  libebml::CodedValueLength(content_size, coded_size, &buffer[id_size]);

  return out.write(buffer, id_size + coded_size);
}

bool
remove_master_from_parent_if_empty_or_only_defaults(libebml::EbmlMaster *parent,
                                                    libebml::EbmlMaster *child,
                                                    std::unordered_map<libebml::EbmlMaster *, bool> &handled) {
  if (!parent || !child || handled[child])
    return false;

  if (0 < child->ListSize()) {
    auto all_set_to_default_value = true;

    for (auto const &childs_child : *child)
      if (   !childs_child->IsDefaultValue()
          || !(   dynamic_cast<libebml::EbmlBinary        *>(childs_child)
               || dynamic_cast<libebml::EbmlDate          *>(childs_child)
               || dynamic_cast<libebml::EbmlFloat         *>(childs_child)
               || dynamic_cast<libebml::EbmlSInteger      *>(childs_child)
               || dynamic_cast<libebml::EbmlString        *>(childs_child)
               || dynamic_cast<libebml::EbmlUInteger      *>(childs_child)
               || dynamic_cast<libebml::EbmlUnicodeString *>(childs_child))) {
        all_set_to_default_value = false;
        break;
      }

    if (!all_set_to_default_value)
      return false;

    for (auto &childs_child : *child)
      delete childs_child;

    child->RemoveAll();
  }

  handled[child] = true;

  auto itr = std::find(parent->begin(), parent->end(), child);
  if (itr != parent->end())
    parent->Remove(itr);

  delete child;

  return true;
}

void
remove_ietf_language_elements(libebml::EbmlMaster &master) {
  remove_elements_recursively_if(master, [](auto &child) {
    return dynamic_cast<libmatroska::KaxLanguageIETF *>(&child)
        || dynamic_cast<libmatroska::KaxChapLanguageIETF *>(&child)
        || dynamic_cast<libmatroska::KaxTagLanguageIETF *>(&child);
  });
}

void
remove_mandatory_elements_set_to_their_default(libebml::EbmlMaster &master) {
  std::unordered_map<uint32_t, unsigned int> num_elements_by_type;
  std::unordered_set<libebml::EbmlElement *> elements_to_remove;

  for (int idx = 0, num_elements = master.ListSize(); idx < num_elements; ++idx) {
    auto child = master[idx];

    if (dynamic_cast<libebml::EbmlMaster *>(child)) {
      remove_mandatory_elements_set_to_their_default(*static_cast<libebml::EbmlMaster *>(child));
      continue;
    }

    ++num_elements_by_type[ get_ebml_id(*child).GetValue() ];

    if (!child->IsDefaultValue())
      continue;

    auto semantic = find_ebml_semantic(EBML_INFO(libmatroska::KaxSegment), get_ebml_id(*child));

    if (!semantic || !semantic->IsMandatory())
      continue;

    elements_to_remove.insert(child);
  }

  // Don't remove elements if there is more than one of them in the
  // same master. For example, ChapterLanguage is mandatory with
  // default "eng", but it is also multiple. If a single
  // <ChapterDisplay> contains two ChapterLanguage, one of them being
  // "eng", then that one must not be removed; otherwise information
  // is lost.

  int idx = 0;
  while (idx < static_cast<int>(master.ListSize())) {
    auto child = master[idx];

    if (elements_to_remove.find(child) == elements_to_remove.end()) {
      ++idx;
      continue;
    }

    if (num_elements_by_type[ get_ebml_id(*child).GetValue() ] == 1) {
      delete child;
      master.Remove(idx);

    } else
      ++idx;
  }
}

static bool
must_be_present_in_master_by_id(libebml::EbmlId const &id) {
  static debugging_option_c s_debug{"must_be_present_in_master"};

  auto semantic = find_ebml_semantic(EBML_INFO(libmatroska::KaxSegment), id);
  if (!semantic || !semantic->IsMandatory()) {
    mxdebug_if(s_debug, fmt::format("ID {0:08x}: 0 (either no semantic or not mandatory)\n", id.GetValue()));
    return false;
  }

  auto elt         = std::shared_ptr<libebml::EbmlElement>(&create_ebml_element(*semantic));
  auto has_default = has_default_value(*elt);

  mxdebug_if(s_debug, fmt::format("ID {0:08x}: {1} (does {2}have a default value)\n", id.GetValue(), !has_default, has_default ? "" : "not "));

  return !has_default;
}

bool
must_be_present_in_master(libebml::EbmlId const &id) {
  static std::unordered_map<uint32_t, bool> s_must_be_present;

  auto itr = s_must_be_present.find(id.GetValue());

  if (itr != s_must_be_present.end())
    return itr->second;

  auto result                      = must_be_present_in_master_by_id(id);
  s_must_be_present[id.GetValue()] = result;

  return result;
}


bool
must_be_present_in_master(libebml::EbmlElement const &element) {
  return must_be_present_in_master(get_ebml_id(element));
}

bool
found_in(libebml::EbmlElement &haystack,
         libebml::EbmlElement const *needle) {
  if (!needle)
    return false;

  if (needle == &haystack)
    return true;

  auto master = dynamic_cast<libebml::EbmlMaster *>(&haystack);
  if (!master)
    return false;

  for (auto &child : *master) {
    if (child == needle)
      return true;

    if (dynamic_cast<libebml::EbmlMaster *>(child) && found_in(*child, needle))
      return true;
  }

  return false;
}

void
remove_dummy_elements(libebml::EbmlMaster &master) {
  remove_elements_recursively_if(master, [](auto &child) { return is_dummy(child); });
}

void
remove_deprecated_elements(libebml::EbmlMaster &master) {
  auto const &deprecated_ids = get_deprecated_elements_by_id();
  auto const &end            = deprecated_ids.end();

  remove_elements_recursively_if(master, [&deprecated_ids, &end](auto &child) {
    return deprecated_ids.find(get_ebml_id(child).GetValue()) != end;
  });
}

void
remove_unrenderable_elements(libebml::EbmlMaster &master,
                             bool with_default) {
  remove_deprecated_elements(master);

  remove_elements_recursively_if(master, [with_default](auto &child) {
    auto renderable = child.ValueIsSet() || (with_default && has_default_value(child));
    return !renderable;
  });
}

bool
has_default_value(libebml::EbmlElement const *elt) {
  return elt ? has_default_value(*elt) : false;
}

#if LIBEBML_VERSION >= 0x020000
libebml::EbmlElement::ShouldWrite
render_should_write_arg(bool with_default) {
  return with_default ? libebml::EbmlElement::WriteAll : libebml::EbmlElement::WriteSkipDefault;
}

bool
has_default_value(libebml::EbmlElement const &elt) {
  return elt.ElementSpec().HasDefault();
}

uint64_t
get_global_timestamp_scale(libmatroska::KaxBlockGroup const &block) {
  return block.GlobalTimestampScale();
}

void
init_timestamp(libmatroska::KaxCluster &cluster,
               uint64_t timestamp,
               int64_t timestamp_scale) {
  cluster.InitTimestamp(timestamp, timestamp_scale);
}

void
set_previous_timestamp(libmatroska::KaxCluster &cluster,
                       uint64_t timestamp,
                       int64_t timestamp_scale) {
  cluster.SetPreviousTimestamp(timestamp, timestamp_scale);
}

libebml::EbmlId
create_ebml_id_from(libebml::EbmlBinary const &b) {
  uint32_t raw_id = get_uint_be(b.GetBuffer(), b.GetSize());
  return { raw_id };
}

libebml::EbmlId
create_ebml_id_from(uint32_t value,
             [[maybe_unused]] std::size_t length) {
  return { value };
}

libebml::EbmlId
get_ebml_id(libebml::EbmlElement const &e) {
  return e.GetClassId();
}

std::size_t
get_head_size(libebml::EbmlElement const &e) {
  return e.GetDataStart() - e.GetElementPosition();
}

#else // LIBEBML_VERSION >= 0x020000

bool
render_should_write_arg(bool with_default) {
  return with_default;
}

bool
has_default_value(libebml::EbmlElement const &elt) {
  return elt.DefaultISset();
}

uint64_t
get_global_timestamp_scale(libmatroska::KaxBlockGroup const &block) {
  return block.GlobalTimecodeScale();
}

void
init_timestamp(libmatroska::KaxCluster &cluster,
               uint64_t timestamp,
               int64_t timestamp_scale) {
  cluster.InitTimecode(timestamp, timestamp_scale);
}

void
set_previous_timestamp(libmatroska::KaxCluster &cluster,
                       uint64_t timestamp,
                       int64_t timestamp_scale) {
  cluster.SetPreviousTimecode(timestamp, timestamp_scale);
}

libebml::EbmlId
create_ebml_id_from(libebml::EbmlBinary const &b) {
  return { b.GetBuffer(), static_cast<unsigned int>(b.GetSize()) };
}

libebml::EbmlId
create_ebml_id_from(uint32_t value,
             std::size_t length) {
  return { value, static_cast<unsigned int>(length) };
}

libebml::EbmlId
get_ebml_id(libebml::EbmlElement const &e) {
  return EbmlId(e);
}

std::size_t
get_head_size(libebml::EbmlElement const &e) {
  return e.HeadSize();
}
#endif
