#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H

#include "common/common_pch.h"

#include <QDialog>
#include <QFlags>

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class MassModificationDialog;
}

class MassModificationDialog : public QDialog {
  Q_OBJECT;
public:
  enum Action {
    Shift       = 0x01,
    Sort        = 0x02,
    Constrict   = 0x04,
    Expand      = 0x08,
    SetLanguage = 0x10,
    SetCountry  = 0x20,
    Multiply    = 0x40,
  };
  Q_DECLARE_FLAGS(Actions, Action);

private:
  std::unique_ptr<Ui::MassModificationDialog> m_ui;
  bool m_editionOrChapterSelected;

public:
  explicit MassModificationDialog(QWidget *parent, bool editionOrChapterSelected, QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  ~MassModificationDialog();

  Actions actions() const;
  int64_t shiftBy() const;
  double multiplyBy() const;
  QString language() const;
  QString country() const;

  void setupUi(QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  void retranslateUi();

public slots:
  void verifyOptions();

protected:
  bool isShiftByValid() const;
  bool isMultiplyByValid() const;
};

}}}

Q_DECLARE_OPERATORS_FOR_FLAGS(mtx::gui::ChapterEditor::MassModificationDialog::Actions);

#endif // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_MASS_MODIFICATION__DIALOG_H
