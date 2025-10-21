/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlBinary.h>
#include <ebml/EbmlDate.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>

#include "common/strings/utf8.h"

namespace mtx {
namespace construct {

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlDate, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(value));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlUInteger, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(value));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlSInteger, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(value));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlFloat, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(value));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlString, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(value));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlUnicodeString, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  master->PushElement(object->SetValue(to_wide(value)));
}

template<typename Tobject,
         typename Tvalue>
inline typename std::enable_if< std::is_base_of<libebml::EbmlBinary, Tobject>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value) {
  if (!object)
    return;
  object->CopyBuffer(value->get_buffer(), value->get_size());
  master->PushElement(*object);
}

inline void
cons_impl(libebml::EbmlMaster *master,
          libebml::EbmlMaster *sub_master) {
  if (!sub_master)
    return;
  master->PushElement(*sub_master);
}

template<typename... Targs>
inline void
cons_impl(libebml::EbmlMaster *master,
          libebml::EbmlMaster *sub_master,
          Targs... args) {
  if (sub_master)
    master->PushElement(*sub_master);
  cons_impl(master, args...);
}

template<typename Tobject,
         typename Tvalue,
         typename... Targs>
inline typename std::enable_if< !std::is_convertible<Tobject *, libebml::EbmlMaster *>::value >::type
cons_impl(libebml::EbmlMaster *master,
          Tobject *object,
          Tvalue const &value,
          Targs... args) {
  if (object)
    cons_impl(master, object, value);
  cons_impl(master, args...);
}

template<typename T>
auto
master() -> T * {
  auto *master = new T;
  for (auto element : *master)
    delete element;
  master->RemoveAll();
  return master;
}

template<typename Tmaster>
inline auto
cons() -> Tmaster * {
  return master<Tmaster>();
}

template<typename Tmaster,
         typename... Targs>
inline auto
cons(Targs... args) -> Tmaster * {
  auto master = cons<Tmaster>();
  cons_impl(master, args...);

  return master;
}

}}
