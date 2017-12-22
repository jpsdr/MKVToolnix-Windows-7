/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions used by Qt GUIs

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include <QString>

#include <ebml/EbmlString.h>
#include <ebml/EbmlUnicodeString.h>

#define Q(s)  to_qs(s)
#define QH(s)  to_qs(s).toHtmlEscaped()
#define QY(s) to_qs(Y(s))
#define QYH(s) to_qs(Y(s)).toHtmlEscaped()
#define QNY(singular, plural, count) to_qs(NY(singular, plural, count))
#define QTR(s, dummy) to_qs(Y(s))

#define MTX_DECLARE_PRIVATE(Class) \
  inline Class##Private* p_func() { return reinterpret_cast<Class##Private *>(&(*p_ptr)); } \
  inline const Class##Private* p_func() const { return reinterpret_cast<const Class##Private *>(&(*p_ptr)); } \
  friend class Class##Private;

#define MTX_DECLARE_PRIVATE_P(Pptr, Class) \
  inline Class##Private* p_func() { return reinterpret_cast<Class##Private *>(&(*Pptr)); } \
  inline const Class##Private* p_func() const { return reinterpret_cast<const Class##Private *>(&(*Pptr)); } \
  friend class Class##Private;

#define MTX_DECLARE_PUBLIC(Class)                                    \
  inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
  inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
  friend class Class;

inline QChar
to_qs(char const source) {
  return QChar{source};
}

inline QString
to_qs(char const *source) {
  return QString{source};
}

inline QString
to_qs(std::string const &source) {
  return QString::fromUtf8(source.c_str());
}

inline QString
to_qs(std::wstring const &source) {
  return QString::fromStdWString(source);
}

inline QString
to_qs(boost::format const &source) {
  return QString::fromUtf8(source.str().c_str());
}

inline QString
to_qs(::libebml::EbmlString const &s) {
  return to_qs(static_cast<std::string const &>(s));
}

inline QString
to_qs(::libebml::EbmlString *s) {
  if (!s)
    return QString{};
  return to_qs(static_cast<std::string const &>(*s));
}

inline QString
to_qs(::libebml::UTFstring const &s) {
  return to_qs(s.GetUTF8());
}

inline QString
to_qs(::libebml::UTFstring *s) {
  if (!s)
    return QString{};
  return to_qs(*s);
}

inline std::string
to_utf8(QString const &source) {
  return std::string{ source.toUtf8().data() };
}

inline std::wstring
to_wide(QString const &source) {
  return source.toStdWString();
}

inline void
mxinfo(QString const &s) {
  mxinfo(to_utf8(s));
}

inline std::ostream &
operator <<(std::ostream &out,
            QString const &string) {
  out << std::string{string.toUtf8().data()};
  return out;
}

inline std::wostream &
operator <<(std::wostream &out,
            QString const &string) {
  out << string.toStdWString();
  return out;
}
