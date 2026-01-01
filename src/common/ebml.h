/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions that need libebml/libmatroska

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlElement.h>
#include <ebml/EbmlId.h>
#include <ebml/EbmlDummy.h>
#include <ebml/EbmlMaster.h>
#include <ebml/EbmlUnicodeString.h>

#include <matroska/KaxTracks.h>

#include "common/ebml_concepts.h"

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
libebml::EbmlId get_ebml_id(libebml::EbmlElement const &e);

#if !defined(EBML_INFO)
#define EBML_INFO(ref)  ref::ClassInfos
#endif
#if !defined(EBML_ID)
#define EBML_ID(ref)  ref::ClassInfos.GlobalId
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
is_type(libebml::EbmlId const &id) {
  return id == EBML_ID(T);
}

template<typename T1, typename T2, typename... Trest>
bool
is_type(libebml::EbmlId const &id) {
  return is_type<T1>(id) || is_type<T2, Trest...>(id);
}

template<typename T>
bool
is_type(libebml::EbmlElement *e) {
  return !e ? false : (get_ebml_id(*e) == EBML_ID(T));
}

template<typename T1, typename T2, typename... Trest>
bool
is_type(libebml::EbmlElement *e) {
  return !e ? false : is_type<T1>(e) || is_type<T2, Trest...>(e);
}

template<typename T>
bool
is_type(libebml::EbmlElement const &e) {
  return get_ebml_id(e) == EBML_ID(T);
}

template<typename T1, typename T2, typename... Trest>
bool
is_type(libebml::EbmlElement const &e) {
  return is_type<T1>(e) || is_type<T2, Trest...>(e);
}

inline bool
is_dummy(libebml::EbmlElement const &e) {
  return !!dynamic_cast<libebml::EbmlDummy const *>(&e);
}

inline bool
is_dummy(libebml::EbmlElement const *e) {
  return !!dynamic_cast<libebml::EbmlDummy const *>(e);
}

template <typename type>type &
get_empty_child(libebml::EbmlMaster &master) {
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

template <typename T>
T &
add_empty_child(libebml::EbmlMaster &master) {
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
add_empty_child(libebml::EbmlMaster *master) {
  return add_empty_child<T>(*master);
}

template <typename T>
T *
find_child(libebml::EbmlMaster const &m) {
  return static_cast<T *>(m.FindFirstElt(EBML_INFO(T)));
}

template <typename T>
T*
find_child(libebml::EbmlMaster const *m) {
  return static_cast<T *>(m->FindFirstElt(EBML_INFO(T)));
}

template<typename T>
T &
get_child(libebml::EbmlMaster *m) {
  return libebml::GetChild<T>(*m);
}

template<typename T>
T &
get_child(libebml::EbmlMaster &m) {
  return libebml::GetChild<T>(m);
}

template<typename A> A &
get_child_empty_if_new(libebml::EbmlMaster &m) {
  auto *child = find_child<A>(m);
  return child ? *child : get_empty_child<A>(m);
}

template<typename A> A &
get_child_empty_if_new(libebml::EbmlMaster *m) {
  return get_child_empty_if_new<A>(*m);
}

template <typename A>A &
get_first_or_next_child(libebml::EbmlMaster &master,
                    A *previous_child) {
  return !previous_child ? get_child<A>(master) : libebml::GetNextChild<A>(master, *previous_child);
}

template<typename T>
void
delete_children(libebml::EbmlMaster &master) {
  for (auto idx = master.ListSize(); 0 < idx; --idx) {
    auto element = master[idx - 1];
    if (is_type<T>(element)) {
      delete element;
      master.Remove(idx - 1);
    }
  }
}

template<typename T>
void
delete_children(libebml::EbmlMaster *master) {
  if (master)
    delete_children<T>(*master);
}

template<typename T>
void
remove_children(libebml::EbmlMaster &master) {
  for (auto idx = master.ListSize(); 0 < idx; --idx)
    if (is_type<T>(master[idx - 1]))
      master.Remove(idx - 1);
}

template<typename T>
void
remove_children(libebml::EbmlMaster *master) {
  if (master)
    remove_children<T>(*master);
}

inline void
remove_delete_child_impl(libebml::EbmlMaster &master,
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
delete_child(libebml::EbmlMaster &master,
             libebml::EbmlElement *element) {
  remove_delete_child_impl(master, element, true);
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
Tvalue
find_child_value(libebml::EbmlMaster const &master,
               Tvalue const &default_value = Tvalue{}) {
  auto child = find_child<Telement>(master);
  return child ? static_cast<Tvalue>(child->GetValue()) : default_value;
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
Tvalue
find_child_value(libebml::EbmlMaster const *master,
               Tvalue const &default_value = Tvalue{}) {
  auto child = find_child<Telement>(*master);
  return child ? static_cast<Tvalue>(child->GetValue()) : default_value;
}

template<mtx::derived_from_ebml_binary_cc T>
memory_cptr
find_child_value(libebml::EbmlMaster const &master,
               bool clone = true) {
  auto child = find_child<T>(master);
  return !child ? memory_cptr()
       : clone  ? memory_c::clone(child->GetBuffer(), child->GetSize())
       :          memory_c::borrow(child->GetBuffer(), child->GetSize());
}

template<mtx::derived_from_ebml_binary_cc T>
memory_cptr
find_child_value(libebml::EbmlMaster const *master,
               bool clone = true) {
  return find_child_value<T>(*master, clone);
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
std::optional<Tvalue>
find_optional_child_value(libebml::EbmlMaster const &master) {
  auto child = find_child<Telement>(master);
  if (child)
    return static_cast<Tvalue>(child->GetValue());
  return std::nullopt;
}

template<typename Telement,
         typename Tvalue = decltype(Telement().GetValue())>
std::optional<Tvalue>
find_optional_child_value(libebml::EbmlMaster const *master) {
  return find_optional_child_value<Telement>(*master);
}

template<typename Telement>
std::optional<bool>
find_optional_child_bool_value(libebml::EbmlMaster const &master) {
  auto child = find_child<Telement>(master);
  if (child)
    return static_cast<bool>(!!child->GetValue());
  return std::nullopt;
}

template<typename Telement>
std::optional<bool>
find_optional_child_bool_value(libebml::EbmlMaster const *master) {
  return find_optional_child_bool_value<Telement>(*master);
}

template<typename Telement>
decltype(Telement().GetValue())
get_child_value(libebml::EbmlMaster &master) {
  return get_child<Telement>(master).GetValue();
}

template<typename Telement>
decltype(Telement().GetValue())
get_child_value(libebml::EbmlMaster *master) {
  return get_child<Telement>(master).GetValue();
}

libebml::EbmlElement *empty_ebml_master(libebml::EbmlElement *e);
libebml::EbmlElement *create_ebml_element(const libebml::EbmlCallbacks &callbacks, const libebml::EbmlId &id);
libebml::EbmlElement &create_ebml_element(const libebml::EbmlSemantic &semantic);
libebml::EbmlMaster *sort_ebml_master(libebml::EbmlMaster *e);
void remove_voids_from_master(libebml::EbmlElement *element);
void move_children(libebml::EbmlMaster &source, libebml::EbmlMaster &destination);
bool remove_master_from_parent_if_empty_or_only_defaults(libebml::EbmlMaster *parent, libebml::EbmlMaster *child, std::unordered_map<libebml::EbmlMaster *, bool> &handled);
void remove_ietf_language_elements(libebml::EbmlMaster &master);
void remove_mandatory_elements_set_to_their_default(libebml::EbmlMaster &master);
void remove_dummy_elements(libebml::EbmlMaster &master);
void remove_deprecated_elements(libebml::EbmlMaster &master);
void remove_unrenderable_elements(libebml::EbmlMaster &master, bool with_default);

libebml::EbmlId create_ebml_id_from(libebml::EbmlBinary const &b);
libebml::EbmlId create_ebml_id_from(uint32_t value, std::size_t length);

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

std::size_t get_head_size(libebml::EbmlElement const &e);


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

using ebml_callbacks_master_c     = libebml::EbmlCallbacksMaster;
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

using ebml_callbacks_master_c     = libebml::EbmlCallbacks;
using kax_cluster_timestamp_c     = libmatroska::KaxClusterTimecode;
using kax_reference_timestamp_c   = libmatroska::KaxReferenceTimeCode;
using kax_timestamp_scale_c       = libmatroska::KaxTimecodeScale;
using kax_track_timestamp_scale_c = libmatroska::KaxTrackTimecodeScale;

#endif
