#pragma once

#include "common/common_pch.h"

#include <QString>
#include <QDialog>

namespace mtx { namespace gui { namespace Info {

namespace Ui {
class JobSettingsDialog;
}

class JobSettings;
class JobSettingsDialogPrivate;
class JobSettingsDialog : public QDialog {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(JobSettingsDialogPrivate)

  std::unique_ptr<JobSettingsDialogPrivate> const p_ptr;

  explicit JobSettingsDialog(JobSettingsDialogPrivate &p);

public:
  explicit JobSettingsDialog(QWidget *parent, QString const &fileName);
  ~JobSettingsDialog();

  JobSettings settings();

public slots:
  virtual void accept() override;
  void enableOkButton(QString const &fileName);
};

}}}
