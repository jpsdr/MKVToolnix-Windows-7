#pragma once

#include "common/common_pch.h"

#include <QDialog>

#include "mkvtoolnix-gui/util/settings.h"

class QListWidget;
class QVariant;

namespace mtx { namespace gui {

class SelectCharacterSetDialogPrivate;
class SelectCharacterSetDialog : public QDialog {
  Q_OBJECT;
  Q_DECLARE_PRIVATE(SelectCharacterSetDialog);

  QScopedPointer<SelectCharacterSetDialogPrivate> const d_ptr;

public:
  explicit SelectCharacterSetDialog(QWidget *parent, QString const &fileName, QString const &initialCharacterSet = QString{}, QStringList const &additionalCharacterSets = QStringList{});
  virtual ~SelectCharacterSetDialog();

  void setUserData(QVariant const &data);
  QVariant const &userData() const;

signals:
  void characterSetSelected(QString const &characterSet);

public slots:
  virtual void updatePreview();
  virtual void emitResult();
  virtual void retranslateUi();

protected:
  QString selectedCharacterSet() const;
};

}}
