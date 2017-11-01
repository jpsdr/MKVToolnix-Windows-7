/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   helper functions that need libebml/libmatroska

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>

#include "common/date_time.h"
#include "common/ebml.h"
#include "common/memory.h"
#include "common/unique_numbers.h"
#include "common/version.h"

using namespace libebml;

EbmlElement *
empty_ebml_master(EbmlElement *e) {
  EbmlMaster *m;

  m = dynamic_cast<EbmlMaster *>(e);
  if (!m)
    return e;

  while (m->ListSize() > 0) {
    delete (*m)[0];
    m->Remove(0);
  }

  return m;
}

EbmlElement *
create_ebml_element(const EbmlCallbacks &callbacks,
                    const EbmlId &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(callbacks);
  size_t i;

//   if (id == EbmlId(*parent))
//     return empty_ebml_master(&parent->Generic().Create());

  for (i = 0; i < EBML_CTX_SIZE(context); i++)
    if (id == EBML_CTX_IDX_ID(context,i))
      return empty_ebml_master(&EBML_SEM_CREATE(EBML_CTX_IDX(context,i)));

  for (i = 0; i < EBML_CTX_SIZE(context); i++) {
    EbmlElement *e;

    if (!(context != EBML_SEM_CONTEXT(EBML_CTX_IDX(context,i))))
      continue;

    e = create_ebml_element(EBML_CTX_IDX_INFO(context, i), id);
    if (e)
      return e;
  }

  return nullptr;
}

static EbmlCallbacks const *
do_find_ebml_callbacks(EbmlCallbacks const &base,
                       EbmlId const &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

  if (EBML_INFO_ID(base) == id)
    return &base;

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

EbmlCallbacks const *
find_ebml_callbacks(EbmlCallbacks const &base,
                    EbmlId const &id) {
  static std::unordered_map<uint32_t, EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_callbacks(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

static EbmlCallbacks const *
do_find_ebml_callbacks(EbmlCallbacks const &base,
                       char const *debug_name) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

  if (!strcmp(debug_name, EBML_INFO_NAME(base)))
    return &base;

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

EbmlCallbacks const *
find_ebml_callbacks(EbmlCallbacks const &base,
                    char const *debug_name) {
  static std::unordered_map<std::string, EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(debug_name);
  if (itr != s_cache.end())
    return itr->second;

  auto result         = do_find_ebml_callbacks(base, debug_name);
  s_cache[debug_name] = result;

  return result;
}

static EbmlCallbacks const *
do_find_ebml_parent_callbacks(EbmlCallbacks const &base,
                              EbmlId const &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlCallbacks *result;
  size_t i;

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

EbmlCallbacks const *
find_ebml_parent_callbacks(EbmlCallbacks const &base,
                           EbmlId const &id) {
  static std::unordered_map<uint32_t, EbmlCallbacks const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_parent_callbacks(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

static EbmlSemantic const *
do_find_ebml_semantic(EbmlCallbacks const &base,
                      EbmlId const &id) {
  const EbmlSemanticContext &context = EBML_INFO_CONTEXT(base);
  const EbmlSemantic *result;
  size_t i;

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

EbmlSemantic const *
find_ebml_semantic(EbmlCallbacks const &base,
                   EbmlId const &id) {
  static std::unordered_map<uint32_t, EbmlSemantic const *> s_cache;

  auto itr = s_cache.find(id.GetValue());
  if (itr != s_cache.end())
    return itr->second;

  auto result            = do_find_ebml_semantic(base, id);
  s_cache[id.GetValue()] = result;

  return result;
}

EbmlMaster *
sort_ebml_master(EbmlMaster *m) {
  if (!m)
    return m;

  int first_element = -1;
  int first_master  = -1;
  size_t i;
  for (i = 0; i < m->ListSize(); i++) {
    if (dynamic_cast<EbmlMaster *>((*m)[i]) && (-1 == first_master))
      first_master = i;
    else if (!dynamic_cast<EbmlMaster *>((*m)[i]) && (-1 != first_master) && (-1 == first_element))
      first_element = i;
    if ((first_master != -1) && (first_element != -1))
      break;
  }

  if (first_master == -1)
    return m;

  while (first_element != -1) {
    EbmlElement *e = (*m)[first_element];
    m->Remove(first_element);
    m->InsertElement(*e, first_master);
    first_master++;
    for (first_element++; first_element < static_cast<int>(m->ListSize()); first_element++)
      if (!dynamic_cast<EbmlMaster *>((*m)[first_element]))
        break;
    if (first_element >= static_cast<int>(m->ListSize()))
      first_element = -1;
  }

  for (i = 0; i < m->ListSize(); i++)
    if (dynamic_cast<EbmlMaster *>((*m)[i]))
      sort_ebml_master(dynamic_cast<EbmlMaster *>((*m)[i]));

  return m;
}

void
move_children(EbmlMaster &source,
              EbmlMaster &destination) {
  for (auto child : source)
    destination.PushElement(*child);
}

// ------------------------------------------------------------------------

int64_t
kt_get_default_duration(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackDefaultDuration>(track);
}

int64_t
kt_get_number(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackNumber>(track);
}

int64_t
kt_get_uid(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackUID>(track);
}

std::string
kt_get_codec_id(KaxTrackEntry &track) {
  return FindChildValue<KaxCodecID>(track);
}

std::string
kt_get_language(KaxTrackEntry &track) {
  return FindChildValue<KaxTrackLanguage>(track);
}

int
kt_get_max_blockadd_id(KaxTrackEntry &track) {
  return FindChildValue<KaxMaxBlockAdditionID>(track);
}

int
kt_get_a_channels(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioChannels>(audio, 1u) : 1;
}

double
kt_get_a_sfreq(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioSamplingFreq>(audio, 8000.0) : 8000.0;
}

double
kt_get_a_osfreq(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioOutputSamplingFreq>(audio, 8000.0) : 8000.0;
}

int
kt_get_a_bps(KaxTrackEntry &track) {
  auto audio = FindChild<KaxTrackAudio>(track);
  return audio ? FindChildValue<KaxAudioBitDepth, int>(audio, -1) : -1;
}

int
kt_get_v_pixel_width(KaxTrackEntry &track) {
  auto video = FindChild<KaxTrackVideo>(track);
  return video ? FindChildValue<KaxVideoPixelWidth>(video) : 0;
}

int
kt_get_v_pixel_height(KaxTrackEntry &track) {
  auto video = FindChild<KaxTrackVideo>(track);
  return video ? FindChildValue<KaxVideoPixelHeight>(video) : 0;
}

EbmlElement *
find_ebml_element_by_id(EbmlMaster *master,
                        const EbmlId &id) {
  for (auto child : *master)
    if (EbmlId(*child) == id)
      return child;

  return nullptr;
}

std::pair<EbmlMaster *, size_t>
find_element_in_master(EbmlMaster *master,
                       EbmlElement *element_to_find) {
  if (!master || !element_to_find)
    return std::make_pair<EbmlMaster *, size_t>(nullptr, 0);

  auto &elements = master->GetElementList();
  auto itr       = brng::find(elements, element_to_find);

  if (itr != elements.end())
    return std::make_pair(master, std::distance(elements.begin(), itr));

  for (auto &sub_element : elements) {
    auto sub_master = dynamic_cast<EbmlMaster *>(sub_element);
    if (!sub_master)
      continue;

    auto result = find_element_in_master(sub_master, element_to_find);
    if (result.first)
      return result;
  }

  return std::make_pair<EbmlMaster *, size_t>(nullptr, 0);
}

static std::unordered_map<uint32_t, bool> const &
get_deprecated_elements_by_id() {
  static std::unordered_map<uint32_t, bool> s_elements;

  if (!s_elements.empty())
    return s_elements;

  s_elements[KaxBlockVirtual::ClassInfos.GlobalId.GetValue()]       = true;
  s_elements[KaxReferenceVirtual::ClassInfos.GlobalId.GetValue()]   = true;
  s_elements[KaxSliceFrameNumber::ClassInfos.GlobalId.GetValue()]   = true;
  s_elements[KaxSliceBlockAddID::ClassInfos.GlobalId.GetValue()]    = true;
  s_elements[KaxSliceDelay::ClassInfos.GlobalId.GetValue()]         = true;
  s_elements[KaxSliceDuration::ClassInfos.GlobalId.GetValue()]      = true;
  s_elements[KaxEncryptedBlock::ClassInfos.GlobalId.GetValue()]     = true;
  s_elements[KaxTrackTimecodeScale::ClassInfos.GlobalId.GetValue()] = true;
  s_elements[KaxTrackOffset::ClassInfos.GlobalId.GetValue()]        = true;
  s_elements[KaxCodecSettings::ClassInfos.GlobalId.GetValue()]      = true;
  s_elements[KaxCodecInfoURL::ClassInfos.GlobalId.GetValue()]       = true;
  s_elements[KaxCodecDownloadURL::ClassInfos.GlobalId.GetValue()]   = true;
  s_elements[KaxOldStereoMode::ClassInfos.GlobalId.GetValue()]      = true;
  s_elements[KaxVideoGamma::ClassInfos.GlobalId.GetValue()]         = true;
  s_elements[KaxVideoFrameRate::ClassInfos.GlobalId.GetValue()]     = true;
  s_elements[KaxAudioPosition::ClassInfos.GlobalId.GetValue()]      = true;
  s_elements[KaxCueRefCluster::ClassInfos.GlobalId.GetValue()]      = true;
  s_elements[KaxCueRefNumber::ClassInfos.GlobalId.GetValue()]       = true;
  s_elements[KaxCueRefCodecState::ClassInfos.GlobalId.GetValue()]   = true;
  s_elements[KaxFileReferral::ClassInfos.GlobalId.GetValue()]       = true;

  return s_elements;
}

template<typename T>
void
fix_elements_set_to_default_value_if_unset(EbmlElement *e) {
  static debugging_option_c s_debug{"fix_elements_in_master"};

  auto t = static_cast<T *>(e);

  if (!t->DefaultISset() || t->ValueIsSet())
    return;

  mxdebug_if(s_debug,
             boost::format("fix_elements_in_master: element has default, but value is no set; setting: ID %|1$08x| name %2%\n")
             % t->Generic().GlobalId.GetValue() % t->Generic().DebugName);
  t->SetValue(t->GetValue());
}

void
fix_elements_in_master(EbmlMaster *master) {
  static debugging_option_c s_debug{"fix_elements_in_master"};

  if (!master)
    return;

  auto callbacks = find_ebml_callbacks(KaxSegment::ClassInfos, master->Generic().GlobalId);
  if (!callbacks) {
    mxdebug_if(s_debug, boost::format("fix_elements_in_master: No callbacks found for ID %|1$08x|\n") % master->Generic().GlobalId.GetValue());
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
    auto child_id = child->Generic().GlobalId.GetValue();
    auto itr      = deprecated_elements.find(child_id);

    if (itr != deprecated_elements_end) {
      delete child;
      master->Remove(idx);
      continue;

    }

    ++idx;
    is_present[child_id] = true;

    if (dynamic_cast<EbmlMaster *>(child))
      fix_elements_in_master(static_cast<EbmlMaster *>(child));

    else if (dynamic_cast<EbmlDate *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlDate>(child);

    else if (dynamic_cast<EbmlFloat *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlFloat>(child);

    else if (dynamic_cast<EbmlSInteger *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlSInteger>(child);

    else if (dynamic_cast<EbmlString *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlString>(child);

    else if (dynamic_cast<EbmlUInteger *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlUInteger>(child);

    else if (dynamic_cast<EbmlUnicodeString *>(child))
      fix_elements_set_to_default_value_if_unset<EbmlUnicodeString>(child);
  }

  // 4. Take care of certain mandatory elements without default values
  //    that we can provide sensible values for, e.g. create UIDs
  //    ourselves.

  // 4.1. Info
  if (dynamic_cast<KaxInfo *>(master)) {
    auto info_data = get_default_segment_info_data();

    if (!is_present[KaxMuxingApp::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxMuxingApp>(master).SetValueUTF8(info_data.muxing_app);

    if (!is_present[KaxWritingApp::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxWritingApp>(master).SetValueUTF8(info_data.writing_app);
  }

  // 4.2. Tracks
  else if (dynamic_cast<KaxTrackEntry *>(master)) {
    if (!is_present[KaxTrackUID::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxTrackUID>(master).SetValue(create_unique_number(UNIQUE_TRACK_IDS));
  }

  // 4.3. Chapters
  else if (dynamic_cast<KaxEditionEntry *>(master)) {
    if (!is_present[KaxEditionUID::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxEditionUID>(master).SetValue(create_unique_number(UNIQUE_EDITION_IDS));


  } else if (dynamic_cast<KaxChapterAtom *>(master)) {
    if (!is_present[KaxChapterUID::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxChapterUID>(master).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));

    if (!is_present[KaxChapterTimeStart::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxChapterTimeStart>(master).SetValue(0);

  } else if (dynamic_cast<KaxChapterDisplay *>(master)) {
    if (!is_present[KaxChapterString::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxChapterString>(master).SetValueUTF8("");

  }

  // 4.4. Tags
  else if (dynamic_cast<KaxTag *>(master)) {
    if (!is_present[KaxTagTargets::ClassInfos.GlobalId.GetValue()])
      fix_elements_in_master(&AddEmptyChild<KaxTagTargets>(master));

    else if (!is_present[KaxTagSimple::ClassInfos.GlobalId.GetValue()])
      fix_elements_in_master(&AddEmptyChild<KaxTagSimple>(master));

  } else if (dynamic_cast<KaxTagTargets *>(master)) {
    if (!is_present[KaxTagTargetTypeValue::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxTagTargetTypeValue>(master).SetValue(50); // = movie

  } else if (dynamic_cast<KaxTagSimple *>(master)) {
    if (!is_present[KaxTagName::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxTagName>(master).SetValueUTF8("");

  }

  // 4.5. Attachments
  else if (dynamic_cast<KaxAttached *>(master)) {
    if (!is_present[KaxFileUID::ClassInfos.GlobalId.GetValue()])
      AddEmptyChild<KaxFileUID>(master).SetValue(create_unique_number(UNIQUE_ATTACHMENT_IDS));
  }
}

void
fix_mandatory_elements(EbmlElement *master) {
  if (dynamic_cast<EbmlMaster *>(master))
    fix_elements_in_master(static_cast<EbmlMaster *>(master));
}

void
remove_voids_from_master(EbmlElement *element) {
  auto master = dynamic_cast<EbmlMaster *>(element);
  if (master)
    DeleteChildren<EbmlVoid>(master);
}

int
write_ebml_element_head(mm_io_c &out,
                        EbmlId const &id,
                        int64_t content_size) {
	int id_size    = EBML_ID_LENGTH(id);
	int coded_size = CodedSizeLength(content_size, 0);
  uint8_t buffer[4 + 8];

	id.Fill(buffer);
	CodedValueLength(content_size, coded_size, &buffer[id_size]);

  return out.write(buffer, id_size + coded_size);
}

bool
remove_master_from_parent_if_empty_or_only_defaults(EbmlMaster *parent,
                                                    EbmlMaster *child,
                                                    std::unordered_map<EbmlMaster *, bool> &handled) {
  if (!parent || !child || handled[child])
    return false;

  if (0 < child->ListSize()) {
    auto all_set_to_default_value = true;

    for (auto const &childs_child : *child)
      if (   !childs_child->IsDefaultValue()
          || !(   dynamic_cast<EbmlBinary        *>(childs_child)
               || dynamic_cast<EbmlDate          *>(childs_child)
               || dynamic_cast<EbmlFloat         *>(childs_child)
               || dynamic_cast<EbmlSInteger      *>(childs_child)
               || dynamic_cast<EbmlString        *>(childs_child)
               || dynamic_cast<EbmlUInteger      *>(childs_child)
               || dynamic_cast<EbmlUnicodeString *>(childs_child))) {
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

static bool
must_be_present_in_master_by_id(EbmlId const &id) {
  static debugging_option_c s_debug{"must_be_present_in_master"};

  auto semantic = find_ebml_semantic(KaxSegment::ClassInfos, id);
  if (!semantic || !semantic->IsMandatory()) {
    mxdebug_if(s_debug, boost::format("ID %|1$08x|: 0 (either no semantic or not mandatory)\n") % id.GetValue());
    return false;
  }

  auto elt = std::shared_ptr<EbmlElement>(&semantic->Create());

  mxdebug_if(s_debug, boost::format("ID %|1$08x|: %2% (default is %3%set)\n") % id.GetValue() % (!elt->DefaultISset()) % (elt->DefaultISset() ? "" : "not "));

  return !elt->DefaultISset();
}

bool
must_be_present_in_master(EbmlCallbacks const &callbacks) {
  static std::unordered_map<uint32_t, bool> s_must_be_present;

  auto id  = callbacks.ClassId();
  auto itr = s_must_be_present.find(id.GetValue());

  if (itr != s_must_be_present.end())
    return itr->second;

  auto result                      = must_be_present_in_master_by_id(id);
  s_must_be_present[id.GetValue()] = result;

  return result;
}


bool
must_be_present_in_master(EbmlElement const &element) {
  return must_be_present_in_master(element.Generic());
}
