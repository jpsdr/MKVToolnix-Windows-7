#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/types.h"

namespace mtx::bcp47 {
class language_c;
}

namespace mtx::gui::ChapterEditor {

namespace Ui {
class RenumberSubChaptersParametersDialog;
}

class RenumberSubChaptersParametersDialog : public QDialog {
  Q_OBJECT
private:
  std::unique_ptr<Ui::RenumberSubChaptersParametersDialog> m_ui;

public:
  enum class NameMatch {
      All = 0
    , First
    , ByLanguage
  };

public:
  explicit RenumberSubChaptersParametersDialog(QWidget *parent, int firstChapterNumber, QStringList const &existingSubChapters, QStringList const &additionalLanguages);
  ~RenumberSubChaptersParametersDialog();

  int firstEntryToRenumber() const;
  int numberOfEntries() const;
  int firstChapterNumber() const;
  QString nameTemplate() const;
  NameMatch nameMatchingMode() const;
  mtx::bcp47::language_c languageOfNamesToReplace() const;
  bool skipHidden() const;

public Q_SLOTS:
  void enableControls();

protected:
  void setupUi(int firstChapterNumber, QStringList const &existingSubChapters, QStringList const &additionalLanguages);
};

}
