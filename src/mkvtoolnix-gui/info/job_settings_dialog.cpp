#include "common/common_pch.h"

#include <QDialogButtonBox>
#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/info/job_settings_dialog.h"
#include "mkvtoolnix-gui/info/job_settings.h"
#include "mkvtoolnix-gui/info/job_settings_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Info {

using namespace mtx::gui;

class JobSettingsDialogPrivate {
public:
  std::unique_ptr<Ui::JobSettingsDialog> m_ui{new Ui::JobSettingsDialog};
};

JobSettingsDialog::JobSettingsDialog(QWidget *parent,
                                     QString const &fileName)
  : QDialog{parent}
  , p_ptr{new JobSettingsDialogPrivate}
{
  auto p = p_func();

  // Setup UI controls.
  p->m_ui->setupUi(this);

  auto settings       = Util::Settings::get().m_defaultInfoJobSettings;
  settings.m_fileName = fileName;

  p->m_ui->settings->setSettings(settings);

  enableOkButton(fileName);

  connect(p->m_ui->buttons,  &QDialogButtonBox::accepted,         this, &JobSettingsDialog::accept);
  connect(p->m_ui->buttons,  &QDialogButtonBox::rejected,         this, &QDialog::reject);
  connect(p->m_ui->settings, &JobSettingsWidget::fileNameChanged, this, &JobSettingsDialog::enableOkButton);
}

JobSettingsDialog::~JobSettingsDialog() {
}

void
JobSettingsDialog::accept() {
  auto &ui = *p_func()->m_ui;

  if (ui.saveAsDefault->isChecked()) {
    auto &cfg                    = Util::Settings::get();
    cfg.m_defaultInfoJobSettings = ui.settings->settings();
    cfg.save();
  }

  QDialog::accept();
}

JobSettings
JobSettingsDialog::settings() {
  return p_func()->m_ui->settings->settings();
}

void
JobSettingsDialog::enableOkButton(QString const &fileName) {
  Util::buttonForRole(p_func()->m_ui->buttons)->setEnabled(!fileName.isEmpty());
}

}}}
