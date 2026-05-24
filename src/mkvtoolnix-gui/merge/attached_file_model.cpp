#include "common/common_pch.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardItem>

#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/attached_file_model.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/model.h"

namespace mtx::gui::Merge {

AttachedFileModel::AttachedFileModel(QObject *parent)
  : QStandardItemModel{parent}
  , m_yesIcon{Util::fixStandardItemIcon(MainWindow::yesIcon())}
  , m_noIcon{Util::fixStandardItemIcon(MainWindow::noIcon())}
{
  setSortRole(Util::SortRole);

  retranslateUi();
}

AttachedFileModel::~AttachedFileModel() {
}

void
AttachedFileModel::reset() {
  beginResetModel();
  removeRows(0, rowCount());
  m_attachedFilesMap.clear();
  endResetModel();
}

QList<QStandardItem *>
AttachedFileModel::createRowItems(Track const &attachedFile)
  const {
  auto list = QList<QStandardItem *>{};
  for (auto column = 0; column < NumberOfColumns; ++column)
    list << new QStandardItem{};

  list[0]->setData(reinterpret_cast<quint64>(&attachedFile), Util::AttachmentRole);

  return list;
}

void
AttachedFileModel::setRowData(QList<QStandardItem *> const &items,
                              Track const &attachedFile) {
  auto info = QFileInfo{attachedFile.m_file->m_fileName};
  auto size = QNY("%1 byte (%2)", "%1 bytes (%2)", attachedFile.m_size).arg(attachedFile.m_size).arg(Q(mtx::string::format_file_size(attachedFile.m_size)));

  items[NameColumn       ]->setText(attachedFile.m_name);
  items[MIMETypeColumn   ]->setText(attachedFile.m_codec);
  items[MuxThisColumn    ]->setText(attachedFile.m_muxThis ? QY("Yes") : QY("No"));
  items[DescriptionColumn]->setText(attachedFile.m_attachmentDescription);
  items[SourceFileColumn ]->setText(info.fileName());
  items[SourceDirColumn  ]->setText(QDir::toNativeSeparators(info.path()));
  items[SizeColumn       ]->setText(size);

  items[SizeColumn       ]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  items[NameColumn       ]->setData(attachedFile.m_name.toLower(),                  Util::SortRole);
  items[MIMETypeColumn   ]->setData(attachedFile.m_codec.toLower(),                 Util::SortRole);
  items[MuxThisColumn    ]->setData(attachedFile.m_muxThis ? QY("Yes") : QY("No"),  Util::SortRole);
  items[DescriptionColumn]->setData(attachedFile.m_attachmentDescription.toLower(), Util::SortRole);
  items[SourceFileColumn ]->setData(info.fileName().toLower(),                      Util::SortRole);
  items[SourceDirColumn  ]->setData(info.path().toLower(),                          Util::SortRole);
  items[SizeColumn       ]->setData(static_cast<quint64>(attachedFile.m_size),      Util::SortRole);

  items[NameColumn       ]->setCheckable(true);
  items[NameColumn       ]->setCheckState(attachedFile.m_muxThis ? Qt::Checked : Qt::Unchecked);

  items[MuxThisColumn    ]->setIcon(      attachedFile.m_muxThis ? m_yesIcon   : m_noIcon);

  Util::setItemForegroundColorDisabled(items, !attachedFile.m_muxThis);
}

void
AttachedFileModel::attachedFileUpdated(Track const &attachedFile) {
  auto row = rowForAttachedFile(attachedFile);
  if (row)
    setRowData(itemsForRow(*row), attachedFile);
}

void
AttachedFileModel::retranslateUi() {
  Util::setDisplayableAndSymbolicColumnNames(*this, {
    { QY("Name"),             Q("name")           },
    { QY("MIME type"),        Q("mimeType")       },
    { QY("Copy attachment"),  Q("muxThis")        },
    { QY("Description"),      Q("description")    },
    { QY("Source file name"), Q("sourceFileName") },
    { QY("Directory"),        Q("directory")      },
    { QY("Size"),             Q("size")           },
  });

  horizontalHeaderItem(SizeColumn)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    setRowData(itemsForRow(row), *attachedFileForRow(row));
}

void
AttachedFileModel::addAttachedFiles(QList<TrackPtr> const &attachedFilesToAdd) {
  for (auto const &attachedFile : attachedFilesToAdd) {
    auto newRow = createRowItems(*attachedFile);
    setRowData(newRow, *attachedFile);
    m_attachedFilesMap[reinterpret_cast<quint64>(attachedFile.get())] = attachedFile;
    appendRow(newRow);
  }
}

void
AttachedFileModel::removeAttachedFiles(QList<TrackPtr> const &attachedFilesToRemove) {
  for (auto const &attachedFile : attachedFilesToRemove) {
    auto row = rowForAttachedFile(*attachedFile);
    if (row) {
      removeRow(*row);
      m_attachedFilesMap.remove(reinterpret_cast<quint64>(attachedFile.get()));
    }
  }
}

void
AttachedFileModel::setAttachedFiles(QList<TrackPtr> const &attachedFilesToAdd) {
  reset();
  addAttachedFiles(attachedFilesToAdd);
}

TrackPtr
AttachedFileModel::attachedFileForRow(int row)
  const {
  return m_attachedFilesMap.value(item(row)->data(Util::AttachmentRole).value<quint64>(), TrackPtr{});
}

std::optional<int>
AttachedFileModel::rowForAttachedFile(Track const &attachedFile)
  const {
  for (auto row = 0, numRows = rowCount(); row < numRows; ++row)
    if (attachedFileForRow(row).get() == &attachedFile)
      return row;

  return std::nullopt;
}

QList<QStandardItem *>
AttachedFileModel::itemsForRow(int row) {
  auto list = QList<QStandardItem *>{};
  for (auto column = 0; column < NumberOfColumns; ++column)
    list << item(row, column);

  return list;
}

}
