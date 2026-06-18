#include "common/common_pch.h"

#include <QAction>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QMenu>
#include <QSlider>
#include <QSpinBox>

#include "common/qt.h"
#include "common/list_utils.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/merge/tool.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/string.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui {

class PrefsRunProgramWidgetPrivate {
  friend class PrefsRunProgramWidget;

  std::unique_ptr<Ui::PrefsRunProgramWidget> ui;
  std::unique_ptr<QMenu> variableMenu;
  QString executable;
  QMap<QCheckBox *, Util::Settings::RunProgramForEvent> flagsByCheckbox;
  QMap<Util::Settings::RunProgramType, QWidget *> pagesByType;

  explicit PrefsRunProgramWidgetPrivate()
    : ui{new Ui::PrefsRunProgramWidget}
  {
  }
};


PrefsRunProgramWidget::PrefsRunProgramWidget(QWidget *parent,
                                             Util::Settings::RunProgramConfig const &cfg)
  : QWidget{parent}
  , p_ptr{new PrefsRunProgramWidgetPrivate{}}
{
  setupUi(cfg);
  setupToolTips();
  setupMenu();
  setupConnections();
}

PrefsRunProgramWidget::~PrefsRunProgramWidget() {
}

void
PrefsRunProgramWidget::enableControls() {
  auto p        = p_func();
  auto active   = p->ui->cbConfigurationActive->isChecked();
  auto controls = findChildren<QWidget *>();
  auto type     = static_cast<Util::Settings::RunProgramType>(p->ui->cbType->currentData().value<int>());

  for (auto const &control : controls)
    if (control == p->ui->pbExecuteNow)
      p->ui->pbExecuteNow->setEnabled(active && isValid() && (type != Util::Settings::RunProgramType::DeleteSourceFiles));

    else if (mtx::included_in(control, p->ui->lePowerShellScriptFile, p->ui->pbBrowsePowerShellScriptFile))
      control->setEnabled(p->ui->rbPowerShellScriptIsFile->isChecked());

    else if (control == p->ui->ptePowerShellScriptCode)
      control->setEnabled(!p->ui->rbPowerShellScriptIsFile->isChecked());

    else if (control != p->ui->cbConfigurationActive)
      control->setEnabled(active);
}

void
PrefsRunProgramWidget::enableControlsAndEmitTitleChanged() {
  enableControls();

  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::setupUi(Util::Settings::RunProgramConfig const &cfg) {
  auto p = p_func();

  // Setup UI controls.
  p->ui->setupUi(this);

  setupTypeControl(cfg);

  p->flagsByCheckbox[p->ui->cbAfterJobQueueStopped] = Util::Settings::RunAfterJobQueueFinishes;
  p->flagsByCheckbox[p->ui->cbAfterJobSuccessful]   = Util::Settings::RunAfterJobCompletesSuccessfully;
  p->flagsByCheckbox[p->ui->cbAfterJobWarnings]     = Util::Settings::RunAfterJobCompletesWithWarnings;
  p->flagsByCheckbox[p->ui->cbAfterJobError]        = Util::Settings::RunAfterJobCompletesWithErrors;

  p->ui->cbConfigurationActive->setChecked(cfg.m_active);

  p->executable = Util::replaceApplicationDirectoryWithMtxVariable(cfg.m_commandLine.value(0));
  p->ui->leName->setText(cfg.m_name);
  p->ui->leCommandLine->setText(Util::escape(cfg.m_commandLine, Util::EscapeShellUnix).join(" "));
  p->ui->leAudioFile->setText(QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(cfg.m_audioFile)));
  p->ui->sbVolume->setValue(cfg.m_volume);
  p->ui->slVolume->setValue(cfg.m_volume);
  p->ui->lePowerShellScriptFile->setText(QDir::toNativeSeparators(cfg.m_powerShellScriptFile));
  p->ui->ptePowerShellScriptCode->setPlainText(cfg.m_powerShellScriptCode);
  p->ui->rbPowerShellScriptIsFile->setChecked( cfg.m_powerShellScriptIsFile);
  p->ui->rbPowerShellScriptIsCode->setChecked(!cfg.m_powerShellScriptIsFile);

  for (auto const &checkBox : p->flagsByCheckbox.keys())
    if (cfg.m_forEvents & p->flagsByCheckbox[checkBox])
      checkBox->setChecked(true);

  auto html = QStringList{};
  html << u"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />"
            "<style type=\"text/css\">"
            "p, li { white-space: pre-wrap; }\n"
            "</style>"
            "</head><body>"
            "<h3>"_s
       << QYH("Usage and examples")
       << u"</h3><ul><li>"_s
       << QYH("The command line here uses Unix-style shell escaping via the backslash character even on Windows.")
       << QYH("This means that either backslashes must be doubled or the whole argument must be enclosed in single quotes.")
#if defined(SYS_WINDOWS)
       << QYH("Note that on Windows forward slashes can be used instead of backslashes in path names, too.")
#endif
       << QYH("See below for examples.")
       << u"</li>"_s
       << u"<li>"_s
       << QYH("Always use full path names even if the application is located in the same directory as the GUI.")
       << u"</li>"_s
#if !defined(SYS_APPLE)
       << u"<li>"_s
# if defined(SYS_WINDOWS)
       << QYH("Start file types other than executable files via cmd.exe.")
# else
       << QYH("Start file types other than executable files via xdg-open.")
# endif
       << QYH("See below for examples.")
       << u"</li>"_s
#endif
       << u"</ul><h3>"_s
       << QYH("Examples")
       << u"</h3><ul><li>"_s
       << QYH("Play a WAV file with the default application:")
#if defined(SYS_WINDOWS)
       << u"<code>"_s
       << QH("C:\\\\windows\\\\system32\\\\cmd.exe /c C:\\\\temp\\\\signal.wav")
       << u"</code>"_s
       << QYH("or")
       << u"<code>"_s
       << QH("C:/windows/system32/cmd.exe /c C:/temp/signal.wav")
       << u"</code>"_s
       << QYH("or")
       << u"<code>"_s
       << QH("'C:\\windows\\system32\\cmd.exe' /c 'C:\\temp\\signal.wav'")
       << u"</code>"_s
#elif defined(SYS_APPLE)
       << u"<code>"_s
       << QH("/usr/bin/afplay /Users/janedoe/Audio/signal.wav")
       << u"</code>"_s
#else
       << u"<code>"_s
       << QH("/usr/bin/xdg-open /home/janedoe/Audio/signal.wav")
       << u"</code>"_s
#endif
       << u"</li><li>"_s
       << QYH("Shut down the system in one minute:")
#if defined(SYS_WINDOWS)
       << u"<code>"_s
       << QH("'c:\\windows\\system32\\shutdown.exe' /s /t 60")
       << u"</code>"_s
#elif defined(SYS_APPLE)
       << u"<code>"_s
       << QH("/usr/bin/sudo /sbin/shutdown -h +1")
       << u"</code>"_s
#else
       << u"<code>"_s
       << QH("/usr/bin/sudo /sbin/shutdown --poweroff +1")
       << u"</code>"_s
#endif
       << u"</li>"_s
       << u"</li><li>"_s
       << QYH("Open the multiplexed file with a player:")
       << u"</li>"_s
#if defined(SYS_WINDOWS)
       << u"<code>"_s
       << QH("'C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe' '<MTX_DESTINATION_FILE_NAME>'")
       << u"</code>"_s
#elif defined(SYS_APPLE)
       << u"<code>"_s
       << QH("/usr/bin/vlc '<MTX_DESTINATION_FILE_NAME>'")
       << u"</code>"_s
#else
       << u"<code>"_s
       << QH("/usr/bin/vlc '<MTX_DESTINATION_FILE_NAME>'")
       << u"</code>"_s
#endif
       << u"</ul></body></html>"_s;

  p->ui->tbUsageNotes->setHtml(html.join(u" "_s));

#if !defined(SYS_APPLE)
  // Only macOS needs this button: there the bundled default lives inside the
  // .app, unreachable through the picker. Elsewhere it is navigable, so hide it.
  p->ui->pbRevertAudioFile->setVisible(false);
#endif

  enableControls();
}

void
PrefsRunProgramWidget::setupTypeControl(Util::Settings::RunProgramConfig const &cfg) {
  auto p                  = p_func();
  auto addItemIfSupported = [p, &cfg](QString const &title, Util::Settings::RunProgramType type) {
    if (App::programRunner().isRunProgramTypeSupported(type)) {
      p->ui->cbType->addItem(title, static_cast<int>(type));

      if (cfg.m_type == type)
        p->ui->cbType->setCurrentIndex(p->ui->cbType->count() - 1);
    }
  };

  addItemIfSupported(QY("Execute a program"),                        Util::Settings::RunProgramType::ExecuteProgram);
  addItemIfSupported(QY("Execute a PowerShell script"),              Util::Settings::RunProgramType::ExecutePowerShellScript);
  addItemIfSupported(QY("Play an audio file"),                       Util::Settings::RunProgramType::PlayAudioFile);
  addItemIfSupported(QY("Show a desktop notification"),              Util::Settings::RunProgramType::ShowDesktopNotification);
  addItemIfSupported(QY("Shut down the computer"),                   Util::Settings::RunProgramType::ShutDownComputer);
  addItemIfSupported(QY("Hibernate the computer"),                   Util::Settings::RunProgramType::HibernateComputer);
  addItemIfSupported(QY("Sleep the computer"),                       Util::Settings::RunProgramType::SleepComputer);
  addItemIfSupported(QY("Delete source files for multiplexer jobs"), Util::Settings::RunProgramType::DeleteSourceFiles);
  addItemIfSupported(QY("Quit MKVToolNix"),                          Util::Settings::RunProgramType::QuitMKVToolNix);

  p->pagesByType[Util::Settings::RunProgramType::ExecuteProgram]          = p->ui->executeProgramTypePage;
  p->pagesByType[Util::Settings::RunProgramType::ExecutePowerShellScript] = p->ui->executePowerShellScriptTypePage;
  p->pagesByType[Util::Settings::RunProgramType::PlayAudioFile]           = p->ui->playAudioFileTypePage;
  p->pagesByType[Util::Settings::RunProgramType::ShowDesktopNotification] = p->ui->emptyTypePage;
  p->pagesByType[Util::Settings::RunProgramType::ShutDownComputer]        = p->ui->emptyTypePage;
  p->pagesByType[Util::Settings::RunProgramType::HibernateComputer]       = p->ui->emptyTypePage;
  p->pagesByType[Util::Settings::RunProgramType::SleepComputer]           = p->ui->emptyTypePage;
  p->pagesByType[Util::Settings::RunProgramType::DeleteSourceFiles]       = p->ui->emptyTypePage;
  p->pagesByType[Util::Settings::RunProgramType::QuitMKVToolNix]          = p->ui->emptyTypePage;

  showPageForType(cfg.m_type);

  if (p->ui->cbType->count() > 1)
    return;

  p->ui->lType->setVisible(false);
  p->ui->cbType->setVisible(false);
}

void
PrefsRunProgramWidget::setupToolTips() {
  auto p = p_func();

  auto conditionsToolTip = u"%1 %2"_s
    .arg(QY("If any of these checkboxes is checked, the action will be executed when the corresponding condition is met."))
    .arg(QY("Independent of the checkboxes, every active configuration can be triggered manually from the \"job output\" tool."));

  Util::setToolTip(p->ui->leName, QY("This is an arbitrary name the GUI can use to refer to this particular configuration."));
  Util::setToolTip(p->ui->cbConfigurationActive, QY("Deactivating this checkbox is a way to disable a configuration temporarily without having to change its parameters."));
  Util::setToolTip(p->ui->pbExecuteNow, u"%1 %2"_s.arg(QY("Executes the action immediately.")).arg(QY("Note that most <MTX_…> variables are empty and will be removed for actions that can take variables as arguments.")));
  Util::setToolTip(p->ui->cbAfterJobQueueStopped, conditionsToolTip);
  Util::setToolTip(p->ui->cbAfterJobSuccessful,   conditionsToolTip);
  Util::setToolTip(p->ui->cbAfterJobError,        conditionsToolTip);

  Util::setToolTip(p->ui->pbRevertAudioFile, QY("Reset the audio file to the default sound."));
}

void
PrefsRunProgramWidget::setupMenu() {
  auto p = p_func();

#if defined(SYS_WINDOWS)
  auto name        = u"C:\\data\\movies\\A.new.hope.mkv"_s;
#else
  auto name        = u"/data/movies/A.new.hope.mkv"_s;
#endif
  auto nameFI      = QFileInfo{name};
  auto exName      = u" (%1)"_s.arg(name);
  auto exSuffix    = u" (%1 → %2)"_s.arg(name).arg(nameFI.suffix());
  auto exBaseName  = u" (%1 → %2)"_s.arg(name).arg(nameFI.completeBaseName());
  auto exDirectory = u" (%1 → %2)"_s.arg(name).arg(QDir::toNativeSeparators(nameFI.absolutePath()));

  QList<std::pair<QString, QString> > entries{
    { QY("Variables for all job types"),                                 u""_s                           },
    { QY("Job type ('multiplexer' or 'info')"),                          u"JOB_TYPE"_s                   },
    { QY("Job description"),                                             u"JOB_DESCRIPTION"_s            },
    { QY("Job start date && time in ISO 8601 format"),                   u"JOB_START_TIME"_s             },
    { QY("Job end date && time in ISO 8601 format"),                     u"JOB_END_TIME"_s               },
    { QY("Exit code (0: ok, 1: warnings occurred, 2: errors occurred)"), u"JOB_EXIT_CODE"_s              },

    { QY("Variables for multiplex jobs"),                                u""_s                           },
    { QY("Destination file's absolute path") + exName,                   u"DESTINATION_FILE_NAME"_s      },
    { QY("Destination folders's absolute path") + exDirectory,           u"DESTINATION_FILE_DIRECTORY"_s },
    { QY("Destination file's base name") + exBaseName,                   u"DESTINATION_FILE_BASE_NAME"_s },
    { QY("Destination file's suffix") + exSuffix,                        u"DESTINATION_FILE_SUFFIX"_s    },
    { QY("Source files' absolute paths"),                                u"SOURCE_FILE_NAMES"_s          },
    { QY("Chapter file's absolute path"),                                u"CHAPTERS_FILE_NAME"_s         },

    { QY("General variables"),                                           u""_s                           },
    { QY("Current date && time in ISO 8601 format"),                     u"CURRENT_TIME"_s               },
    { QY("MKVToolNix GUI's installation directory"),                     u"INSTALLATION_DIRECTORY"_s     },
  };

  p->variableMenu.reset(new QMenu{this});

  for (auto const &entry : entries)
    if (entry.second.isEmpty())
      p->variableMenu->addSection(entry.first);

    else {
      auto action = p->variableMenu->addAction(u"<MTX_%1> – %2"_s.arg(entry.second).arg(entry.first));
      connect(action, &QAction::triggered, [this, entry]() {
        this->addVariable(u"<MTX_%1>"_s.arg(entry.second));
      });
    }
}

void
PrefsRunProgramWidget::setupConnections() {
  auto p = p_func();

  connect(p->ui->cbConfigurationActive,        &QCheckBox::toggled,                                                    this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->leName,                       &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->cbType,                       static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PrefsRunProgramWidget::typeChanged);
  connect(p->ui->leCommandLine,                &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::commandLineEdited);
  connect(p->ui->pbBrowseExecutable,           &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeExecutable);
  connect(p->ui->pbAddVariable,                &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::selectVariableToAdd);
  connect(p->ui->pbExecuteNow,                 &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::executeNow);
  connect(p->ui->leAudioFile,                  &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->pbBrowseAudioFile,            &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeAudioFile);
  connect(p->ui->pbRevertAudioFile,            &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::revertAudioFileToDefault);
  connect(p->ui->slVolume, &QSlider::valueChanged,  p->ui->sbVolume, &QSpinBox::setValue);
  connect(p->ui->sbVolume, &QSpinBox::valueChanged, p->ui->slVolume, &QSlider::setValue);
  connect(p->ui->lePowerShellScriptFile,       &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->pbBrowsePowerShellScriptFile, &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changePowerShellScriptFile);
  connect(p->ui->ptePowerShellScriptCode,      &QPlainTextEdit::textChanged,                                           this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->rbPowerShellScriptIsFile,     &QRadioButton::toggled,                                                 this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
  connect(p->ui->rbPowerShellScriptIsCode,     &QRadioButton::toggled,                                                 this, &PrefsRunProgramWidget::enableControlsAndEmitTitleChanged);
}

void
PrefsRunProgramWidget::selectVariableToAdd() {
  p_func()->variableMenu->exec(QCursor::pos());
}

void
PrefsRunProgramWidget::addVariable(QString const &variable) {
  changeArguments([&variable](QStringList &arguments) {
    arguments << variable;
  });
}

bool
PrefsRunProgramWidget::isValid()
  const {
  return config()->isValid();
}

void
PrefsRunProgramWidget::executeNow() {
  if (!isValid())
    return;

  auto setupVariables = [](Jobs::ProgramRunner::VariableMap &variables) {
    auto now = QDateTime::currentDateTime();

    // dummy values for jobs/job.cpp:
    variables[u"JOB_START_TIME"_s]  << now.addSecs(-1 * (4 * 60 + 20)).toString(Qt::ISODate);
    variables[u"JOB_END_TIME"_s]    << now.toString(Qt::ISODate);
    variables[u"JOB_DESCRIPTION"_s] << QY("example values from the current multiplexer tab");
    variables[u"JOB_EXIT_CODE"_s]   << u"0"_s;

    // current tab's values for jobs/mux_job.cpp:
    MainWindow::mergeTool()->runProgramSetupVariablesForCurrentTab(variables);
  };

  App::programRunner().run(Util::Settings::RunNever, setupVariables, config());
}

void
PrefsRunProgramWidget::changeExecutable() {
  auto p       = p_func();
  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + u" (*.exe *.bat *.cmd)"_s;
#endif
  filters << QY("All files") + u" (*)"_s;

  auto realExecutable = Util::replaceMtxVariableWithApplicationDirectory(p->executable);
  auto newExecutable  = Util::getOpenFileName(this, QY("Select executable"), realExecutable, filters.join(u";;"_s));
  newExecutable       = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(newExecutable));

  if (newExecutable.isEmpty() || (newExecutable == p->executable))
    return;

  changeArguments([&newExecutable](QStringList &arguments) {
    if (arguments.isEmpty())
      arguments << newExecutable;
    else
      arguments[0] = newExecutable;
  });

  p->executable = newExecutable;

  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::commandLineEdited(QString const &commandLine) {
  auto p             = p_func();
  auto arguments     = Util::unescapeSplit(commandLine, Util::EscapeShellUnix);
  auto newExecutable = arguments.value(0);

  if (p->executable == newExecutable)
    return;

  p->executable = newExecutable;

  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::changeArguments(std::function<void(QStringList &)> const &worker) {
  auto p         = p_func();
  auto arguments = Util::unescapeSplit(p->ui->leCommandLine->text(), Util::EscapeShellUnix);

  worker(arguments);

  p->ui->leCommandLine->setText(Util::escape(arguments, Util::EscapeShellUnix).join(u" "_s));
  p->ui->leCommandLine->setFocus();
}

Util::Settings::RunProgramConfigPtr
PrefsRunProgramWidget::config()
  const {
  auto p                        = p_func();
  auto cfg                      = std::make_shared<Util::Settings::RunProgramConfig>();
  auto cmdLine                  = p->ui->leCommandLine->text().replace(QRegularExpression{"^\\s+"}, u""_s);
  cfg->m_name                   = p->ui->leName->text();
  cfg->m_type                   = static_cast<Util::Settings::RunProgramType>(p->ui->cbType->currentData().value<int>());
  cfg->m_commandLine            = Util::unescapeSplit(cmdLine, Util::EscapeShellUnix);
  cfg->m_active                 = p->ui->cbConfigurationActive->isChecked();
  cfg->m_audioFile              = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(p->ui->leAudioFile->text()));
  cfg->m_volume                 = p->ui->sbVolume->value();
  cfg->m_powerShellScriptIsFile = p->ui->rbPowerShellScriptIsFile->isChecked();
  cfg->m_powerShellScriptFile   = QDir::toNativeSeparators(p->ui->lePowerShellScriptFile->text());
  cfg->m_powerShellScriptCode   = p->ui->ptePowerShellScriptCode->toPlainText();

  for (auto const &checkBox : p->flagsByCheckbox.keys())
    if (checkBox->isChecked())
      cfg->m_forEvents |= p->flagsByCheckbox[checkBox];

  return cfg;
}

void
PrefsRunProgramWidget::revertAudioFileToDefault() {
  auto p = p_func();

  p->ui->leAudioFile->setText(QDir::toNativeSeparators(App::programRunner().defaultAudioFileName()));
  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::changeAudioFile() {
  auto p             = p_func();
  auto filters       = QStringList{} << QY("All files") + u" (*)"_s;

  auto realAudioFile = Util::replaceMtxVariableWithApplicationDirectory(p->ui->leAudioFile->text());
  auto startDir      = realAudioFile.isEmpty() ? Util::Settings::get().m_lastProgramRunnerAudioDir.path() : realAudioFile;
  auto newAudioFile  = Util::getOpenFileName(this, QY("Select audio file"), startDir, filters.join(u";;"_s));

  if (newAudioFile.isEmpty())
    return;

  Util::Settings::change([&newAudioFile](Util::Settings &settings) {
    settings.m_lastProgramRunnerAudioDir.setPath(QFileInfo{newAudioFile}.path());
  });

  newAudioFile = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(newAudioFile));

  p->ui->leAudioFile->setText(newAudioFile);

  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::changePowerShellScriptFile() {
  auto p             = p_func();
  auto filters       = QStringList{} << (QY("PowerShell scripts") + u" (*.ps1)"_s) << (QY("All files") + u" (*)"_s);

  auto scriptFile    = p->ui->lePowerShellScriptFile->text();
  auto startDir      = scriptFile.isEmpty() ? Util::Settings::get().m_lastOpenDir.path() : scriptFile;
  auto newScriptFile = Util::getOpenFileName(this, QY("Select PowerShell script file"), startDir, filters.join(u";;"_s));

  if (newScriptFile.isEmpty())
    return;

  Util::Settings::change([&newScriptFile](Util::Settings &settings) {
    settings.m_lastOpenDir.setPath(QFileInfo{newScriptFile}.path());
  });

  p->ui->lePowerShellScriptFile->setText(newScriptFile);

  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::typeChanged(int index) {
  auto p    = p_func();
  auto type = static_cast<Util::Settings::RunProgramType>(p->ui->cbType->itemData(index).value<int>());

  showPageForType(type);

  enableControlsAndEmitTitleChanged();
}

void
PrefsRunProgramWidget::showPageForType(Util::Settings::RunProgramType type) {
  auto p = p_func();

  p->ui->typeWidgets->setCurrentWidget(p->pagesByType[type]);
  p->ui->gbTypeSpecificSettings->setVisible(p->pagesByType[type] != p->ui->emptyTypePage);
}

QString
PrefsRunProgramWidget::validate()
  const {
  auto cfg = config();
  return cfg->m_active ? cfg->validate() : QString{};
}

}
