/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions that need libebml/libmatroska

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlMaster.h>
#include <ebml/EbmlUnicodeString.h>

#include <matroska/KaxTracks.h>

namespace libmatroska {
class KaxCluster;
class KaxCues;
class KaxBlockGroup;
class KaxBlock;
class KaxSimpleBlock;
};

int64_t kt_get_default_duration(libmatroska::KaxTrackEntry &track);
int64_t kt_get_number(libmatroska::KaxTrackEntry &track);
int64_t kt_get_uid(libmatroska::KaxTrackEntry &track);
std::string kt_get_codec_id(libmatroska::KaxTrackEntry &track);
int kt_get_max_blockadd_id(libmatroska::KaxTrackEntry &track);

int kt_get_a_channels(libmatroska::KaxTrackEntry &track);
double kt_get_a_sfreq(libmatroska::KaxTrackEntry &track);
double kt_get_a_osfreq(libmatroska::KaxTrackEntry &track);
int kt_get_a_bps(libmatroska::KaxTrackEntry &track);

int kt_get_v_pixel_width(libmatroska::KaxTrackEntry &track);
int kt_get_v_pixel_height(libmatroska::KaxTrackEntry &track);

int write_ebml_element_head(mm_io_c &out, libebml::EbmlId const &id, int64_t content_size);

#if !defined(EBML_INFO)
#define EBML_INFO(ref)  ref::ClassInfos
#endif
#if !defined(EBML_ID)
#define EBML_ID(ref)  ref::ClassInfos.GlobalId
#endif
#if !defined(EBML_ID_VALUE)
#define EBML_ID_VALUE(id)  id.Value
#endif
#if !defined(EBML_ID_LENGTH)
#define EBML_ID_LENGTH(id)  id.Length
#endif
#if !defined(EBML_CLASS_CONTEXT)
#define EBML_CLASS_CONTEXT(ref) ref::ClassInfos.Context
#endif
#if !defined(EBML_CLASS_CALLBACK)
#define EBML_CLASS_CALLBACK(ref)   ref::ClassInfo()
#endif
#if !defined(EBML_CONTEXT)
#define EBML_CONTEXT(e)  e->Generic().Context
#endif
#if !defined(EBML_NAME)
#define EBML_NAME(e)  e->Generic().DebugName
#endif
#if !defined(EBML_INFO_ID)
#define EBML_INFO_ID(cb)      (cb).GlobalId
#endif
#if !defined(EBML_INFO_NAME)
#define EBML_INFO_NAME(cb)    (cb).DebugName
#endif
#if !defined(EBML_INFO_CONTEXT)
#define EBML_INFO_CONTEXT(cb) (cb).GetContext()
#endif
#if !defined(EBML_SEM_UNIQUE)
#define EBML_SEM_UNIQUE(s)  (s).Unique
#endif
#if !defined(EBML_SEM_CONTEXT)
#define EBML_SEM_CONTEXT(s) (s).GetCallbacks.Context
#endif
#if !defined(EBML_SEM_CREATE)
#define EBML_SEM_CREATE(s)  (s).GetCallbacks.Create()
#endif
#if !defined(EBML_CTX_SIZE)
#define EBML_CTX_SIZE(c) (c).Size
#endif
#if !defined(EBML_CTX_IDX)
#define EBML_CTX_IDX(c,i)   (c).MyTable[(i)]
#endif
#if !defined(EBML_CTX_IDX_INFO)
#define EBML_CTX_IDX_INFO(c,i) (c).MyTable[(i)].GetCallbacks
#endif
#if !defined(EBML_CTX_IDX_ID)
#define EBML_CTX_IDX_ID(c,i)   (c).MyTable[(i)].GetCallbacks.GlobalId
#endif
#if !defined(INVALID_FILEPOS_T)
#define INVALID_FILEPOS_T 0
#endif

template<typename T>
bool
Is(libebml::EbmlId const &id) {
  return id == EBML_ID(T);
}

template<typename T1, typename T2, typename... Trest>
bool
Is(libebml::EbmlId const &id) {
  return Is<T1>(id) || Is<T2, Trest...>(id);
}

template<typename T>
bool
Is(libebml::EbmlElement *e) {
  return !e ? false : (libebml::EbmlId(*e) == EBML_ID(T));
}

template<typename T1, typename T2, typename... Trest>
bool
Is(libebml::EbmlElement *e) {
  return !e ? false : Is<T1>(e) || Is<T2, Trest...>(e);
}

template<typename T>
bool
Is(libebml::EbmlElement const &e) {
  return libebml::EbmlId(e) == EBML_ID(T);
}

template<typename T1, typename T2, typename... Trest>
bool
Is(libebml::EbmlElement const &e) {
  return Is<T1>(e) || Is<T2, Trest...>(e);
}

template <typename type>type &
GetEmptyChild(libebml::EbmlMaster &master) {
  libebml::EbmlElement *e;
  libebml::EbmlMaster *m;

  e = master.FindFirstElt(EBML_INFO(type), true);
  if ((m = dynamic_cast<libebml::EbmlMaster *>(e))) {
    while (m->ListSize() > 0) {
      delete (*m)[0];
      m->Remove(0);
    }
  }

  return *(static_cast<type *>(e));
}

template <typename type>type &
GetNextEmptyChild(libebml::EbmlMaster &master,
                  const type &past_elt) {
  libebml::EbmlMaster *m;
  libebml::EbmlElement *e = master.FindNextElt(past_elt, true);
  if ((m = dynamic_cast<libebml::EbmlMaster *>(e))) {
    while (m->ListSize() > 0) {
      delete (*m)[0];
      m->Remove(0);
    }
  }

  return *(static_cast<type *>(e));
}

template <typename T>
T &
AddEmptyChild(libebml::EbmlMaster &master) {
  libebml::EbmlMaster *m;
  libebml::EbmlElement *e = new T;
  if ((m = dynamic_cast<libebml::EbmlMaster *>(e))) {
    while (m->ListSize() > 0) {
      delete (*m)[0];
      m->Remove(0);
    }
  }
  master.PushElement(*e);

  return *(static_cast<T *>(e));
}

template <typename T>
T &
AddEmptyChild(libebml::EbmlMaster *master) {
  return AddEmptyChild<T>(*master);
}

template <typename T>
T *
FindChild(libebml::EbmlMaster const &m) {
  return static_cast<T *>(m.FindFirstElt(EBML_INFO(T)));
}

template <typename T>
T *
FindChild(libebml::EbmlElement const &e) {
  auto &m = dynamic_cast<libebml::EbmlMaster const &>(e);
  return static_cast<T *>(m.FindFirstElt(EBML_INFO(T)));
}

template <typename A> A*
FindChild(libebml::EbmlMaster const *m) {
  return static_cast<A *>(m->FindFirstElt(EBML_INFO(A)));
}

template <typename A> A*
FindChild(libebml::EbmlElement const *e) {
  auto m = dynamic_cast<libebml::EbmlMaster const *>(e);
  assert(m);
  return static_cast<A *>(m->FindFirstElt(EBML_INFO(A)));
}

template<typename A> A &
GetChild(libebml::EbmlMaster *m) {
  return GetChild<A>(*m);
}

template<typename A> A &
GetChildEmptyIfNew(libebml::EbmlMaster &m) {
  auto *child = FindChild<A>(m);
  return child ? *child : GetEmptyChild<A>(m);
}

template<typename A> A &
GetChildEmptyIfNew(libebml::EbmlMaster *m) {
  return GetChildEmptyIfNew<A>(*m);
}

template <typename A>A &
GetFirstOrNextChild(libebml::EbmlMaster &master,
                    A *previous_child) {
  return !previous_child ? GetChild<A>(master) : libebml::GetNextChild<A>(master, *previous_child);
}

template <typename A>A &
GetFirstOrNextChild(libebml::EbmlMaster *master,
                    A *previous_child) {
  return !previous_child ? GetChild<A>(*master) : libebml::GetNextChild<A>(*master, *previous_child);
}

template<typename T>
void
DeleteChildren(libebml::EbmlMaster &master) {
  for (auto idx = master.ListSize(); 0 < idx; --idx) {
    auto element = master[idx - 1];
    if (Is<T>(element)) {
      delete element;
      master.Remove(idx - 1);
    }
  }
}

template<typename T>
void
DeleteChildren(libebml::EbmlMaster *master) {
  if (master)
    DeleteChildren<T>(*master);
}

template<typename T>
void
RemoveChildren(libebml::EbmlMaster &master) {
  for (auto idx = master.ListSize(); 0 < idx; --idx)
    if (Is<T>(master[idx - 1]))
      master.Remove(idx - 1);
}

template<typename T>
void
RemoveChildren(libebml::EbmlMaster *master) {
  if (master)
    RemoveChildren<T>(*master);
}

inline void
RemoveDeleteChildImpl(libebml::EbmlMaster &master,
                      libebml::EbmlElement *element,
                      bool do_delete) {
  if (!element)
    return;

  for (auto idx = master.ListSize(); idx > 0; --idx) {
    if (master[idx - 1] == element)
      master.Remove(idx - 1);
  }

  if (do_delete)
    delete element;
}

inline void
RemoveChild(libebml::EbmlMaster &master,
            libebml::EbmlElement *element) {
  RemoveDeleteChildImpl(master, element, false);
}

inline void
DeleteChild(libebml::EbmlMaster &master,
            libebml::EbmlElement *element) {
  RemoveDeleteChildImpl(master, element, true);
}

template<typename T>
void
FixMandatoryElement(libebml::EbmlMaster &master) {
  auto &element = GetChild<T>(master);
  element.SetValue(element.GetValue());
}

template<typename Tfirst,
         typename Tsecond,
         typename... Trest>
void
FixMandatoryElement(libebml::EbmlMaster &master) {
  FixMandatoryElement<Tfirst>(master);
  FixMandatoryElement<Tsecond, Trest...>(master);
}

template<typename... Trest>
void
FixMandatoryElement(libebml::EbmlMaster *master) {
  if (!master)
    return;
  FixMandatoryElement<Trest...>(*master);
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
Tvalue
FindChildValue(libebml::EbmlMaster const &master,
               Tvalue const &default_value = Tvalue{}) {
  auto child = FindChild<Telement>(master);
  return child ? static_cast<Tvalue>(child->GetValue()) : default_value;
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
Tvalue
FindChildValue(libebml::EbmlMaster const *master,
               Tvalue const &default_value = Tvalue{}) {
  auto child = FindChild<Telement>(*master);
  return child ? static_cast<Tvalue>(child->GetValue()) : default_value;
}

template<typename T>
memory_cptr
FindChildValue(libebml::EbmlMaster const &master,
               bool clone = true,
               typename std::enable_if< std::is_base_of<libebml::EbmlBinary, T>::value >::type * = nullptr) {
  auto child = FindChild<T>(master);
  return !child ? memory_cptr()
       : clone  ? memory_c::clone(child->GetBuffer(), child->GetSize())
       :          memory_c::borrow(child->GetBuffer(), child->GetSize());
}

template<typename T>
memory_cptr
FindChildValue(libebml::EbmlMaster const *master,
               bool clone = true,
               typename std::enable_if< std::is_base_of<libebml::EbmlBinary, T>::value >::type * = nullptr) {
  return FindChildValue<T>(*master, clone);
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
std::optional<Tvalue>
FindOptionalChildValue(libebml::EbmlMaster const &master) {
  auto child = FindChild<Telement>(master);
  if (child)
    return static_cast<Tvalue>(child->GetValue());
  return std::nullopt;
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
std::optional<Tvalue>
FindOptionalChildValue(libebml::EbmlMaster const *master) {
  return FindOptionalChildValue<Telement>(*master);
}

template<typename Telement>
std::optional<bool>
FindOptionalChildBoolValue(libebml::EbmlMaster const &master) {
  auto child = FindChild<Telement>(master);
  if (child)
    return static_cast<bool>(!!child->GetValue());
  return std::nullopt;
}

template<typename Telement>
std::optional<bool>
FindOptionalChildBoolValue(libebml::EbmlMaster const *master) {
  return FindOptionalChildBoolValue<Telement>(*master);
}

template<typename Telement>
decltype(Telement().GetValue())
GetChildValue(libebml::EbmlMaster &master) {
  return GetChild<Telement>(master).GetValue();
}

template<typename Telement>
decltype(Telement().GetValue())
GetChildValue(libebml::EbmlMaster *master) {
  return GetChild<Telement>(master).GetValue();
}

libebml::EbmlElement *empty_ebml_master(libebml::EbmlElement *e);
libebml::EbmlElement *create_ebml_element(const libebml::EbmlCallbacks &callbacks, const libebml::EbmlId &id);
libebml::EbmlMaster *sort_ebml_master(libebml::EbmlMaster *e);
void remove_voids_from_master(libebml::EbmlElement *element);
void move_children(libebml::EbmlMaster &source, libebml::EbmlMaster &destination);
bool remove_master_from_parent_if_empty_or_only_defaults(libebml::EbmlMaster *parent, libebml::EbmlMaster *child, std::unordered_map<libebml::EbmlMaster *, bool> &handled);
void remove_ietf_language_elements(libebml::EbmlMaster &master);
void remove_mandatory_elements_set_to_their_default(libebml::EbmlMaster &master);
void remove_dummy_elements(libebml::EbmlMaster &master);
void remove_unrenderable_elements(libebml::EbmlMaster &master, bool with_default);

const libebml::EbmlCallbacks *find_ebml_callbacks(const libebml::EbmlCallbacks &base, const libebml::EbmlId &id);
const libebml::EbmlCallbacks *find_ebml_callbacks(const libebml::EbmlCallbacks &base, const char *debug_name);
const libebml::EbmlCallbacks *find_ebml_parent_callbacks(const libebml::EbmlCallbacks &base, const libebml::EbmlId &id);
const libebml::EbmlSemantic *find_ebml_semantic(const libebml::EbmlCallbacks &base, const libebml::EbmlId &id);

libebml::EbmlElement *find_ebml_element_by_id(libebml::EbmlMaster *master, const libebml::EbmlId &id);
std::pair<libebml::EbmlMaster *, size_t> find_element_in_master(libebml::EbmlMaster *master, libebml::EbmlElement *element_to_find);

void fix_mandatory_elements(libebml::EbmlElement *master);
bool must_be_present_in_master(libebml::EbmlId const &id);
bool must_be_present_in_master(libebml::EbmlElement const &element);

using ebml_element_cptr = std::shared_ptr<libebml::EbmlElement>;
using ebml_master_cptr  = std::shared_ptr<libebml::EbmlMaster>;

template<typename T>
std::shared_ptr<T>
clone(T const &e) {
  return std::shared_ptr<T>{static_cast<T *>(e.Clone())};
}

template<typename T>
std::shared_ptr<T>
clone(T *e) {
  return clone(*e);
}

template <typename T>
std::shared_ptr<T>
clone(std::shared_ptr<T> const &e) {
  return clone(*e);
}

template <typename T>
std::shared_ptr<T>
clone(std::unique_ptr<T> const &e) {
  return clone(*e);
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
bool
change_values(libebml::EbmlMaster &master,
              std::unordered_map<Tvalue, Tvalue> const &changes) {
  auto modified = false;

  for (auto const &child : master) {
    if (dynamic_cast<libebml::EbmlMaster *>(child)) {
      if (change_values<Telement>(static_cast<libebml::EbmlMaster &>(*child), changes))
        modified = true;

    } else if (dynamic_cast<Telement *>(child)) {
      auto &child_elt    = static_cast<Telement &>(*child);
      auto current_value = child_elt.GetValue();

      for (auto const &change : changes) {
        if (current_value != change.first)
          continue;

        child_elt.SetValue(change.second);
        modified = true;
        break;
      }
    }
  }

  return modified;
}

bool has_default_value(libebml::EbmlElement const &elt);
bool has_default_value(libebml::EbmlElement const *elt);

bool found_in(libebml::EbmlElement &haystack, libebml::EbmlElement const *needle);

uint64_t get_global_timestamp_scale(libmatroska::KaxBlockGroup const &block);
void init_timestamp(libmatroska::KaxCluster &cluster, uint64_t timestamp, int64_t timestamp_scale);
void set_previous_timestamp(libmatroska::KaxCluster &cluster, uint64_t timestamp, int64_t timestamp_scale);

#if LIBEBML_VERSION >= 0x020000

template<typename T> uint64_t
get_global_timestamp(T const &block) {
  return block.GlobalTimestamp();
}

template<typename T> void
set_global_timestamp_scale(T &elt,
                           uint64_t timestamp_scale) {
  elt.SetGlobalTimestampScale(timestamp_scale);
}

libebml::EbmlElement::ShouldWrite render_should_write_arg(bool with_default);

using kax_cluster_timestamp_c     = libmatroska::KaxClusterTimestamp;
using kax_reference_timestamp_c   = libmatroska::KaxReferenceTimestamp;
using kax_timestamp_scale_c       = libmatroska::KaxTimestampScale;
using kax_track_timestamp_scale_c = libmatroska::KaxTrackTimestampScale;

#else // LIBEBML_VERSION >= 0x020000

template<typename T> uint64_t
get_global_timestamp(T const &block) {
  return block.GlobalTimecode();
}

template<typename T> void
set_global_timestamp_scale(T &elt,
                           uint64_t timestamp_scale) {
  elt.SetGlobalTimecodeScale(timestamp_scale);
}

bool render_should_write_arg(bool with_default);

namespace libebml {
using filepos_t = ::filepos_t;
}

using kax_cluster_timestamp_c     = libmatroska::KaxClusterTimecode;
using kax_reference_timestamp_c   = libmatroska::KaxReferenceTimeCode;
using kax_timestamp_scale_c       = libmatroska::KaxTimecodeScale;
using kax_track_timestamp_scale_c = libmatroska::KaxTrackTimecodeScale;

#endif
