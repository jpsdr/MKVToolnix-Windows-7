/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Helper functions for containers

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include <concepts>

#include <ebml/EbmlBinary.h>
#include <ebml/EbmlDate.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>

namespace mtx {

template<typename T>
concept derived_from_ebml_binary_cc = std::derived_from<T, libebml::EbmlBinary>;

template<typename T>
concept derived_from_ebml_date_cc = std::derived_from<T, libebml::EbmlDate>;

template<typename T>
concept derived_from_ebml_float_cc = std::derived_from<T, libebml::EbmlFloat>;

template<typename T>
concept derived_from_ebml_master_cc = std::derived_from<T, libebml::EbmlMaster>;

template<typename T>
concept not_derived_from_ebml_master_cc = !derived_from_ebml_master_cc<T>;

template<typename T>
concept derived_from_ebml_s_integer_cc = std::derived_from<T, libebml::EbmlSInteger>;

template<typename T>
concept derived_from_ebml_string_cc = std::derived_from<T, libebml::EbmlString>;

template<typename T>
concept derived_from_ebml_u_integer_cc = std::derived_from<T, libebml::EbmlUInteger>;

template<typename T>
concept derived_from_ebml_unicode_string_cc = std::derived_from<T, libebml::EbmlUnicodeString>;

} // mtx
