#pragma once

#include "common/common_pch.h"

#include <QModelIndex>
#include <QStringList>

namespace mtx::gui::Merge {

class SourceFile;
using SourceFilePtr = std::shared_ptr<SourceFile>;

struct IdentificationPack {
  enum class FileType { Regular,    Chapters, Tags, SegmentInfo };
  enum class AddMode  { UserChoice, Add,      Append, AddDontAsk };

  struct IdentifiedFile {
    FileType m_type{FileType::Regular};
    QString m_fileName;
    SourceFilePtr m_sourceFile;
  };

  AddMode m_addMode{AddMode::UserChoice};
  uint64_t m_tabId{};
  QModelIndex m_sourceFileIdx{};
  Qt::MouseButtons m_mouseButtons{};
  QStringList m_fileNames;
  QVector<IdentifiedFile> m_identifiedSourceFiles, m_identifiedNonSourceFiles;

  QVector<SourceFilePtr>
  sourceFiles() {
    QVector<SourceFilePtr> result;
    result.reserve(m_identifiedSourceFiles.count());

    for (auto &identifiedSourceFile : m_identifiedSourceFiles)
      result << identifiedSourceFile.m_sourceFile;

    return result;
  }
};

}
