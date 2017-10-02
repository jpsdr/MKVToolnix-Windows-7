#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/types.h"

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class GenerateSubChaptersParametersDialog;
}

class GenerateSubChaptersParametersDialog : public QDialog {
  Q_OBJECT;
private:
  std::unique_ptr<Ui::GenerateSubChaptersParametersDialog> m_ui;

public:
  explicit GenerateSubChaptersParametersDialog(QWidget *parent, int firstChapterNumber, uint64_t startTimestamp, QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  ~GenerateSubChaptersParametersDialog();

  int numberOfEntries() const;
  uint64_t durationInNs() const;
  int firstChapterNumber() const;
  uint64_t startTimestamp() const;
  QString nameTemplate() const;
  QString language() const;
  OptQString country() const;

public slots:
  void verifyStartTimestamp();

protected:
  void setupUi(int firstChapterNumber, uint64_t startTimestamp, QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  void retranslateUi();
};

}}}
