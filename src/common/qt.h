/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used by Qt GUIs

   Written by Moritz Bunkus <mo@bunkus.online>.
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

inline QChar
to_qs(char const source) {
  return QChar{source};
}

inline QString
to_qs(QString const &source) {
  return source;
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

inline QString
to_qs(boost::filesystem::path const &source) {
  return to_qs(source.string());
}

inline std::string
to_utf8(QString const &source) {
  return std::string{ source.toUtf8().data() };
}

inline std::wstring
to_wide(QString const &source) {
  return source.toStdWString();
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

#if FMT_VERSION >= 90000
template <> struct fmt::formatter<QString> : ostream_formatter {};
#endif  // FMT_VERSION >= 90000
