#include "common/common_pch.h"

#include <QComboBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/info/job_settings_widget.h"
#include "mkvtoolnix-gui/info/job_settings.h"
#include "mkvtoolnix-gui/info/job_settings_widget.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Info {

using namespace mtx::gui;

class JobSettingsWidgetPrivate {
public:
  std::unique_ptr<Ui::JobSettingsWidget> m_ui{new Ui::JobSettingsWidget};
};

JobSettingsWidget::JobSettingsWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new JobSettingsWidgetPrivate}
{
  auto p = p_func();

  // Setup UI controls.
  p->m_ui->setupUi(this);

  connect(p->m_ui->mode,           static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &JobSettingsWidget::enableControlsAccordingToMode);
  connect(p->m_ui->browseFileName, &QPushButton::clicked,                                                 this, &JobSettingsWidget::browseFileName);
  connect(p->m_ui->fileName,       &QLineEdit::textChanged,                                               this, &JobSettingsWidget::emitFileNameChangeSignal);
}

JobSettingsWidget::~JobSettingsWidget() {
}

void
JobSettingsWidget::setFileNameVisible(bool visible) {
  auto &ui = *p_func()->m_ui;

  for (auto &widget : QList<QWidget *>{ui.fileNameLabel, ui.fileName, ui.browseFileName})
    widget->setVisible(visible);
}

void
JobSettingsWidget::setSettings(JobSettings const &jobSettings) {
  auto &ui = *p_func()->m_ui;

  ui.fileName->setText(jobSettings.m_fileName);
  ui.mode->setCurrentIndex(jobSettings.m_mode == JobSettings::Mode::Tree ? 0 : 1);
  ui.verbosity->setCurrentIndex(jobSettings.m_verbosity == JobSettings::Verbosity::StopAtFirstCluster ? 0 : 1);
  ui.hexDumps->setCurrentIndex(jobSettings.m_hexDumps == JobSettings::HexDumps::None ? 0 : jobSettings.m_hexDumps == JobSettings::HexDumps::First16Bytes ? 1 : 2);
  ui.checksums->setChecked(jobSettings.m_checksums);
  ui.trackInfo->setChecked(jobSettings.m_trackInfo);
  ui.hexPositions->setChecked(jobSettings.m_hexPositions);

  enableControlsAccordingToMode();
}

JobSettings
JobSettingsWidget::settings() {
  auto &ui = *p_func()->m_ui;
  JobSettings jobSettings;

  jobSettings.m_fileName     = ui.fileName->text();
  jobSettings.m_mode         = ui.mode->currentIndex()      == 0 ? JobSettings::Mode::Tree                    : JobSettings::Mode::Summary;
  jobSettings.m_verbosity    = ui.verbosity->currentIndex() == 0 ? JobSettings::Verbosity::StopAtFirstCluster : JobSettings::Verbosity::AllElements;
  jobSettings.m_hexDumps     = ui.hexDumps->currentIndex()  == 0 ? JobSettings::HexDumps::None
                             : ui.hexDumps->currentIndex()  == 1 ? JobSettings::HexDumps::First16Bytes
                             :                                     JobSettings::HexDumps::AllBytes;
  jobSettings.m_checksums    = ui.checksums->isChecked();
  jobSettings.m_trackInfo    = ui.trackInfo->isChecked();
  jobSettings.m_hexPositions = ui.hexPositions->isChecked();

  return jobSettings;
}

void
JobSettingsWidget::enableControlsAccordingToMode() {
  auto &ui    = *p_func()->m_ui;
  bool isTree = ui.mode->currentIndex() == 0;

  for (auto &widget : QList<QWidget *>{ui.verbosityLabel, ui.verbosity, ui.hexDumpsLabel, ui.hexDumps, ui.checksums})
    widget->setEnabled(isTree);
}

void
JobSettingsWidget::browseFileName() {
  auto &ui       = *p_func()->m_ui;
  auto &settings = Util::Settings::get();
  auto path      = !ui.fileName->text().isEmpty() ? QFileInfo{ ui.fileName->text() }.absoluteDir() : Util::dirPath(settings.m_lastOpenDir.path());
  auto fileName  = Util::getSaveFileName(this, QY("Select destination file name"), Util::dirPath(settings.m_lastOpenDir.path()), QY("Text files") + Q(" (*.txt)"));

  if (fileName.isEmpty())
    return;

  auto fileInfo          = QFileInfo{ fileName };
  settings.m_lastOpenDir = fileInfo.absoluteDir();
  settings.save();

  ui.fileName->setText(fileName);
}

void
JobSettingsWidget::emitFileNameChangeSignal(QString const &fileName) {
  emit fileNameChanged(fileName);
}

}
