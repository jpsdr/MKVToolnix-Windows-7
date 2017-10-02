#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/types.h"

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class RenumberSubChaptersParametersDialog;
}

class RenumberSubChaptersParametersDialog : public QDialog {
  Q_OBJECT;
private:
  std::unique_ptr<Ui::RenumberSubChaptersParametersDialog> m_ui;

public:
  enum class NameMatch {
      All = 1
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
  QString languageOfNamesToReplace() const;
  bool skipHidden() const;

protected:
  void setupUi(int firstChapterNumber, QStringList const &existingSubChapters, QStringList const &additionalLanguages);
};

}}}
