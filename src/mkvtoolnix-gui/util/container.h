#pragma once

#include "common/common_pch.h"

namespace mtx::gui::Util {

// Container stuff
template<typename Tstored, typename Tcontainer>
int
findPtr(Tstored *needle,
        Tcontainer const &haystack) {
  auto itr = std::find_if(haystack.begin(), haystack.end(), [needle](auto const &cmp) { return cmp.get() == needle; });
  return haystack.end() == itr ? -1 : std::distance(haystack.begin(), itr);
}

std::vector<std::string> toStdStringVector(QStringList const &strings, int offset = 0);
QStringList toStringList(std::vector<std::string> const &stdStrings, int offset = 0);

template<typename TQVector,
         typename TStdVector>
QVector<TQVector>
stdVectorToQVector(TStdVector const &v) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return QVector<TQVector>{v.begin(), v.end()};
#else
  return QVector<TQVector>::fromStdVector(v);
#endif
}

template<typename T>
QSet<typename T::value_type>
qListToSet(T const &l) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return QSet<typename T::value_type>{l.begin(), l.end()};
#else
  return l.toSet();
#endif
}

template<typename T>
QList<typename T::value_type>
qSetToList(T const &l) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return QList<typename T::value_type>{l.begin(), l.end()};
#else
  return l.toList();
#endif
}

}
