#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_GENERATE_SUB_CHAPTERS_PARAMETERS_DIALOG_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_GENERATE_SUB_CHAPTERS_PARAMETERS_DIALOG_H

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
  explicit GenerateSubChaptersParametersDialog(QWidget *parent, int firstChapterNumber, uint64_t startTimecode, QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  ~GenerateSubChaptersParametersDialog();

  int numberOfEntries() const;
  uint64_t durationInNs() const;
  int firstChapterNumber() const;
  uint64_t startTimecode() const;
  QString nameTemplate() const;
  QString language() const;
  OptQString country() const;

public slots:
  void verifyStartTimecode();

protected:
  void setupUi(int firstChapterNumber, uint64_t startTimecode, QStringList const &additionalLanguages, QStringList const &additionalCountryCodes);
  void retranslateUi();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_GENERATE_SUB_CHAPTERS_PARAMETERS_DIALOG_H
