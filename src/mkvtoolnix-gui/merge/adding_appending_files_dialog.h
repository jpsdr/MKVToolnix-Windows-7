#ifndef MTX_MKVTOOLNIX_GUI_MERGE_ADDING_APPENDING_FILES_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_MERGE_ADDING_APPENDING_FILES_DIALOG_H

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/merge/source_file.h"

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class AddingAppendingFilesDialog;
}

class AddingAppendingFilesDialog : public QDialog {
  Q_OBJECT;
public:
  enum class Decision {
    Add,
    AlwaysAdd,
    Append,
    AddAdditionalParts,
    AddToNew,
  };

protected:
  std::unique_ptr<Ui::AddingAppendingFilesDialog> ui;

public:
  explicit AddingAppendingFilesDialog(QWidget *parent, QList<SourceFilePtr> const &files);
  ~AddingAppendingFilesDialog();

  Decision decision() const;
  int fileIndex() const;

public slots:
  void selectionChanged();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_MERGE_ADDING_APPENDING_FILES_DIALOG_H
