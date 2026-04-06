#pragma once

#include "common/common_pch.h"

#include <QDialog>
#include <QFlags>

#include "common/bcp47.h"

namespace mtx::gui::ChapterEditor {

namespace Ui {
class MassModificationDialog;
}

class MassModificationDialog : public QDialog {
  Q_OBJECT
public:
  enum Action {
    Shift               = 0x0001,
    Sort                = 0x0002,
    Constrict           = 0x0004,
    Expand              = 0x0008,
    SetLanguage         = 0x0010,
    Multiply            = 0x0020,
    SetEndTimestamps    = 0x0040,
    RemoveEndTimestamps = 0x0080,
    RemoveNames         = 0x0100,
  };
  Q_DECLARE_FLAGS(Actions, Action)

  struct Options {
    Actions actions;
    int64_t shiftBy;
    double multiplyBy;
    bool applyToAllTabs;
    mtx::bcp47::language_c language;
  };

private:
  std::unique_ptr<Ui::MassModificationDialog> m_ui;
  bool m_editionOrChapterSelected;

public:
  explicit MassModificationDialog(QWidget *parent, bool editionOrChapterSelected, QStringList const &additionalLanguages);
  ~MassModificationDialog();

  Options const options() const;

  void setupUi(QStringList const &additionalLanguages);
  void retranslateUi();

public Q_SLOTS:
  void verifyOptions();

protected:
  bool isShiftByValid() const;
  bool isMultiplyByValid() const;

  Actions actions() const;
  int64_t shiftBy() const;
  double multiplyBy() const;
  bool applyToAllTabs() const;
  mtx::bcp47::language_c language() const;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(mtx::gui::ChapterEditor::MassModificationDialog::Actions)
