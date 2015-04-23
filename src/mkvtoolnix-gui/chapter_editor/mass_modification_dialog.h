#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H

#include "common/common_pch.h"

#include <QDialog>

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class MassModificationDialog;
}

class MassModificationDialog : public QDialog {
  Q_OBJECT;
public:
  enum class Decision {
    Shift,
    Sort,
    Constrict,
    Expand,
  };

private:
  std::unique_ptr<Ui::MassModificationDialog> ui;

public:
  explicit MassModificationDialog(QWidget *parent, bool editionOrChapterSelected);
  ~MassModificationDialog();

  Decision decision() const;
  int64_t shiftBy() const;

public slots:
  void selectionOrShiftByChanged();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H
