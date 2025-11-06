#include "common/common_pch.h"

#include <QAction>
#include <QCursor>
#include <QDir>
#include <QMenu>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/jobs/program_runner.h"
#include "mkvtoolnix-gui/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
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

    else if (control != p->ui->cbConfigurationActive)
      control->setEnabled(active);
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

  for (auto const &checkBox : p->flagsByCheckbox.keys())
    if (cfg.m_forEvents & p->flagsByCheckbox[checkBox])
      checkBox->setChecked(true);

  auto html = QStringList{};
  html << Q("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />"
            "<style type=\"text/css\">"
            "p, li { white-space: pre-wrap; }\n"
            "</style>"
            "</head><body>"
            "<h3>")
       << QYH("Usage and examples")
       << Q("</h3><ul><li>")
       << QYH("The command line here uses Unix-style shell escaping via the backslash character even on Windows.")
       << QYH("This means that either backslashes must be doubled or the whole argument must be enclosed in single quotes.")
#if defined(SYS_WINDOWS)
       << QYH("Note that on Windows forward slashes can be used instead of backslashes in path names, too.")
#endif
       << QYH("See below for examples.")
       << Q("</li>")
       << Q("<li>")
       << QYH("Always use full path names even if the application is located in the same directory as the GUI.")
       << Q("</li>")
#if !defined(SYS_APPLE)
       << Q("<li>")
# if defined(SYS_WINDOWS)
       << QYH("Start file types other than executable files via cmd.exe.")
# else
       << QYH("Start file types other than executable files via xdg-open.")
# endif
       << QYH("See below for examples.")
       << Q("</li>")
#endif
       << Q("</ul><h3>")
       << QYH("Examples")
       << Q("</h3><ul><li>")
       << QYH("Play a WAV file with the default application:")
#if defined(SYS_WINDOWS)
       << Q("<code>")
       << QH("C:\\\\windows\\\\system32\\\\cmd.exe /c C:\\\\temp\\\\signal.wav")
       << Q("</code>")
       << QYH("or")
       << Q("<code>")
       << QH("C:/windows/system32/cmd.exe /c C:/temp/signal.wav")
       << Q("</code>")
       << QYH("or")
       << Q("<code>")
       << QH("'C:\\windows\\system32\\cmd.exe' /c 'C:\\temp\\signal.wav'")
       << Q("</code>")
#elif defined(SYS_APPLE)
       << Q("<code>")
       << QH("/usr/bin/afplay /Users/janedoe/Audio/signal.wav")
       << Q("</code>")
#else
       << Q("<code>")
       << QH("/usr/bin/xdg-open /home/janedoe/Audio/signal.wav")
       << Q("</code>")
#endif
       << Q("</li><li>")
       << QYH("Shut down the system in one minute:")
#if defined(SYS_WINDOWS)
       << Q("<code>")
       << QH("'c:\\windows\\system32\\shutdown.exe' /s /t 60")
       << Q("</code>")
#elif defined(SYS_APPLE)
       << Q("<code>")
       << QH("/usr/bin/sudo /sbin/shutdown -h +1")
       << Q("</code>")
#else
       << Q("<code>")
       << QH("/usr/bin/sudo /sbin/shutdown --poweroff +1")
       << Q("</code>")
#endif
       << Q("</li>")
       << Q("</li><li>")
       << QYH("Open the multiplexed file with a player:")
       << Q("</li>")
#if defined(SYS_WINDOWS)
       << Q("<code>")
       << QH("'C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe' '<MTX_DESTINATION_FILE_NAME>'")
       << Q("</code>")
#elif defined(SYS_APPLE)
       << Q("<code>")
       << QH("/usr/bin/vlc '<MTX_DESTINATION_FILE_NAME>'")
       << Q("</code>")
#else
       << Q("<code>")
       << QH("/usr/bin/vlc '<MTX_DESTINATION_FILE_NAME>'")
       << Q("</code>")
#endif
       << Q("</ul></body></html>");

  p->ui->tbUsageNotes->setHtml(html.join(Q(" ")));

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
  addItemIfSupported(QY("Play an audio file"),                       Util::Settings::RunProgramType::PlayAudioFile);
  addItemIfSupported(QY("Show a desktop notification"),              Util::Settings::RunProgramType::ShowDesktopNotification);
  addItemIfSupported(QY("Shut down the computer"),                   Util::Settings::RunProgramType::ShutDownComputer);
  addItemIfSupported(QY("Hibernate the computer"),                   Util::Settings::RunProgramType::HibernateComputer);
  addItemIfSupported(QY("Sleep the computer"),                       Util::Settings::RunProgramType::SleepComputer);
  addItemIfSupported(QY("Delete source files for multiplexer jobs"), Util::Settings::RunProgramType::DeleteSourceFiles);
  addItemIfSupported(QY("Quit MKVToolNix"),                          Util::Settings::RunProgramType::QuitMKVToolNix);

  p->pagesByType[Util::Settings::RunProgramType::ExecuteProgram]          = p->ui->executeProgramTypePage;
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

  auto conditionsToolTip = Q("%1 %2")
    .arg(QY("If any of these checkboxes is checked, the action will be executed when the corresponding condition is met."))
    .arg(QY("Independent of the checkboxes, every active configuration can be triggered manually from the \"job output\" tool."));

  Util::setToolTip(p->ui->leName, QY("This is an arbitrary name the GUI can use to refer to this particular configuration."));
  Util::setToolTip(p->ui->cbConfigurationActive, QY("Deactivating this checkbox is a way to disable a configuration temporarily without having to change its parameters."));
  Util::setToolTip(p->ui->pbExecuteNow, Q("%1 %2").arg(QY("Executes the action immediately.")).arg(QY("Note that most <MTX_…> variables are empty and will be removed for actions that can take variables as arguments.")));
  Util::setToolTip(p->ui->cbAfterJobQueueStopped, conditionsToolTip);
  Util::setToolTip(p->ui->cbAfterJobSuccessful,   conditionsToolTip);
  Util::setToolTip(p->ui->cbAfterJobError,        conditionsToolTip);
}

void
PrefsRunProgramWidget::setupMenu() {
  auto p = p_func();

  QList<std::pair<QString, QString> > entries{
    { QY("Variables for all job types"),                                 Q("")                           },
    { QY("Job type ('multiplexer' or 'info')"),                          Q("JOB_TYPE")                   },
    { QY("Job description"),                                             Q("JOB_DESCRIPTION")            },
    { QY("Job start date && time in ISO 8601 format"),                   Q("JOB_START_TIME")             },
    { QY("Job end date && time in ISO 8601 format"),                     Q("JOB_END_TIME")               },
    { QY("Exit code (0: ok, 1: warnings occurred, 2: errors occurred)"), Q("JOB_EXIT_CODE")              },

    { QY("Variables for multiplex jobs"),                                Q("")                           },
    { QY("Destination file's absolute path"),                            Q("DESTINATION_FILE_NAME")      },
    { QY("Destination folders's absolute path"),                         Q("DESTINATION_FILE_DIRECTORY") },
    { QY("Source files' absolute paths"),                                Q("SOURCE_FILE_NAMES")          },
    { QY("Chapter file's absolute path"),                                Q("CHAPTERS_FILE_NAME")         },

    { QY("General variables"),                                           Q("")                           },
    { QY("Current date && time in ISO 8601 format"),                     Q("CURRENT_TIME")               },
    { QY("MKVToolNix GUI's installation directory"),                     Q("INSTALLATION_DIRECTORY")     },
  };

  p->variableMenu.reset(new QMenu{this});

  for (auto const &entry : entries)
    if (entry.second.isEmpty())
      p->variableMenu->addSection(entry.first);

    else {
      auto action = p->variableMenu->addAction(Q("<MTX_%1> – %2").arg(entry.second).arg(entry.first));
      connect(action, &QAction::triggered, [this, entry]() {
        this->addVariable(Q("<MTX_%1>").arg(entry.second));
      });
    }
}

void
PrefsRunProgramWidget::setupConnections() {
  auto p = p_func();

  connect(p->ui->cbConfigurationActive, &QCheckBox::toggled,                                                    this, &PrefsRunProgramWidget::enableControls);
  connect(p->ui->leName,                &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::nameEdited);
  connect(p->ui->cbType,                static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PrefsRunProgramWidget::typeChanged);
  connect(p->ui->leCommandLine,         &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::commandLineEdited);
  connect(p->ui->pbBrowseExecutable,    &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeExecutable);
  connect(p->ui->pbAddVariable,         &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::selectVariableToAdd);
  connect(p->ui->pbExecuteNow,          &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::executeNow);
  connect(p->ui->leAudioFile,           &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::audioFileEdited);
  connect(p->ui->pbBrowseAudioFile,     &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeAudioFile);
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
  if (isValid())
    App::programRunner().run(Util::Settings::RunNever, [](Jobs::ProgramRunner::VariableMap &) {}, config());
}

void
PrefsRunProgramWidget::changeExecutable() {
  auto p       = p_func();
  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + Q(" (*.exe *.bat *.cmd)");
#endif
  filters << QY("All files") + Q(" (*)");

  auto realExecutable = Util::replaceMtxVariableWithApplicationDirectory(p->executable);
  auto newExecutable  = Util::getOpenFileName(this, QY("Select executable"), realExecutable, filters.join(Q(";;")));
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

  enableControls();

  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::commandLineEdited(QString const &commandLine) {
  auto p             = p_func();
  auto arguments     = Util::unescapeSplit(commandLine, Util::EscapeShellUnix);
  auto newExecutable = arguments.value(0);

  if (p->executable == newExecutable)
    return;

  p->executable = newExecutable;

  enableControls();

  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::nameEdited() {
  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::changeArguments(std::function<void(QStringList &)> const &worker) {
  auto p         = p_func();
  auto arguments = Util::unescapeSplit(p->ui->leCommandLine->text(), Util::EscapeShellUnix);

  worker(arguments);

  p->ui->leCommandLine->setText(Util::escape(arguments, Util::EscapeShellUnix).join(Q(" ")));
}

Util::Settings::RunProgramConfigPtr
PrefsRunProgramWidget::config()
  const {
  auto p             = p_func();
  auto cfg           = std::make_shared<Util::Settings::RunProgramConfig>();
  auto cmdLine       = p->ui->leCommandLine->text().replace(QRegularExpression{"^\\s+"}, Q(""));
  cfg->m_name        = p->ui->leName->text();
  cfg->m_type        = static_cast<Util::Settings::RunProgramType>(p->ui->cbType->currentData().value<int>());
  cfg->m_commandLine = Util::unescapeSplit(cmdLine, Util::EscapeShellUnix);
  cfg->m_active      = p->ui->cbConfigurationActive->isChecked();
  cfg->m_audioFile   = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(p->ui->leAudioFile->text()));
  cfg->m_volume      = p->ui->sbVolume->value();

  for (auto const &checkBox : p->flagsByCheckbox.keys())
    if (checkBox->isChecked())
      cfg->m_forEvents |= p->flagsByCheckbox[checkBox];

  return cfg;
}

void
PrefsRunProgramWidget::changeAudioFile() {
  auto p             = p_func();
  auto filters       = QStringList{} << QY("All files") + Q(" (*)");

  auto realAudioFile = Util::replaceMtxVariableWithApplicationDirectory(p->ui->leAudioFile->text());
  auto newAudioFile  = Util::getOpenFileName(this, QY("Select audio file"), realAudioFile, filters.join(Q(";;")));
  newAudioFile       = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(newAudioFile));

  if (newAudioFile.isEmpty())
    return;

  p->ui->leAudioFile->setText(newAudioFile);

  enableControls();

  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::audioFileEdited() {
  enableControls();

  Q_EMIT titleChanged();
}

void
PrefsRunProgramWidget::typeChanged(int index) {
  auto p    = p_func();
  auto type = static_cast<Util::Settings::RunProgramType>(p->ui->cbType->itemData(index).value<int>());

  showPageForType(type);

  Q_EMIT titleChanged();

  enableControls();
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
