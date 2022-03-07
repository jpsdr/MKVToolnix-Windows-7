#pragma once

#include "common/common_pch.h"

#include <QDialog>
#include <QString>

#include "common/bcp47.h"

namespace mtx::gui {

class PrefsLanguageShortcutDialogPrivate;
class PrefsLanguageShortcutDialog : public QDialog {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(PrefsLanguageShortcutDialogPrivate)

  std::unique_ptr<PrefsLanguageShortcutDialogPrivate> const p_ptr;

public:
  explicit PrefsLanguageShortcutDialog(QWidget *parent, bool isNew);
  virtual ~PrefsLanguageShortcutDialog();

  void setLanguage(mtx::bcp47::language_c const &language);
  void setTrackName(QString const &trackName);

  mtx::bcp47::language_c language() const;
  QString trackName() const;

protected Q_SLOTS:
  void enableControls();

protected:
  void setupConnections();
};

}
