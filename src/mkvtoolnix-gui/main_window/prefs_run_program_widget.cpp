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

namespace mtx { namespace gui {

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
  , d_ptr{new PrefsRunProgramWidgetPrivate{}}
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
  Q_D(PrefsRunProgramWidget);

  auto active   = d->ui->cbConfigurationActive->isChecked();
  auto controls = findChildren<QWidget *>();

  for (auto const &control : controls)
    if (control == d->ui->pbExecuteNow)
      d->ui->pbExecuteNow->setEnabled(active && isValid());

    else if (control != d->ui->cbConfigurationActive)
      control->setEnabled(active);
}

void
PrefsRunProgramWidget::setupUi(Util::Settings::RunProgramConfig const &cfg) {
  Q_D(PrefsRunProgramWidget);

  // Setup UI controls.
  d->ui->setupUi(this);

  setupTypeControl(cfg);

  d->flagsByCheckbox[d->ui->cbAfterJobQueueStopped] = Util::Settings::RunAfterJobQueueFinishes;
  d->flagsByCheckbox[d->ui->cbAfterJobSuccessful]   = Util::Settings::RunAfterJobCompletesSuccessfully;
  d->flagsByCheckbox[d->ui->cbAfterJobError]        = Util::Settings::RunAfterJobCompletesWithErrors;

  d->ui->cbConfigurationActive->setChecked(cfg.m_active);

  d->executable = Util::replaceApplicationDirectoryWithMtxVariable(cfg.m_commandLine.value(0));
  d->ui->leName->setText(cfg.m_name);
  d->ui->leCommandLine->setText(Util::escape(cfg.m_commandLine, Util::EscapeShellUnix).join(" "));
  d->ui->leAudioFile->setText(QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(cfg.m_audioFile)));
  d->ui->sbVolume->setValue(cfg.m_volume);

  for (auto const &checkBox : d->flagsByCheckbox.keys())
    if (cfg.m_forEvents & d->flagsByCheckbox[checkBox])
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

  d->ui->tbUsageNotes->setHtml(html.join(Q(" ")));

  enableControls();
}

void
PrefsRunProgramWidget::setupTypeControl(Util::Settings::RunProgramConfig const &cfg) {
  Q_D(PrefsRunProgramWidget);

  auto addItemIfSupported = [d, &cfg](QString const &title, Util::Settings::RunProgramType type) {
    if (App::programRunner().isRunProgramTypeSupported(type)) {
      d->ui->cbType->addItem(title, static_cast<int>(type));

      if (cfg.m_type == type)
        d->ui->cbType->setCurrentIndex(d->ui->cbType->count() - 1);
    }
  };

  addItemIfSupported(QY("Execute a program"),      Util::Settings::RunProgramType::ExecuteProgram);
  addItemIfSupported(QY("Play an audio file"),     Util::Settings::RunProgramType::PlayAudioFile);
  addItemIfSupported(QY("Shut down the computer"), Util::Settings::RunProgramType::ShutDownComputer);
  addItemIfSupported(QY("Hibernate the computer"), Util::Settings::RunProgramType::HibernateComputer);
  addItemIfSupported(QY("Sleep the computer"),     Util::Settings::RunProgramType::SleepComputer);

  d->pagesByType[Util::Settings::RunProgramType::ExecuteProgram]    = d->ui->executeProgramTypePage;
  d->pagesByType[Util::Settings::RunProgramType::PlayAudioFile]     = d->ui->playAudioFileTypePage;
  d->pagesByType[Util::Settings::RunProgramType::ShutDownComputer]  = d->ui->emptyTypePage;
  d->pagesByType[Util::Settings::RunProgramType::HibernateComputer] = d->ui->emptyTypePage;
  d->pagesByType[Util::Settings::RunProgramType::SleepComputer]     = d->ui->emptyTypePage;

  showPageForType(cfg.m_type);

  if (d->ui->cbType->count() > 1)
    return;

  d->ui->lType->setVisible(false);
  d->ui->cbType->setVisible(false);
}

void
PrefsRunProgramWidget::setupToolTips() {
  Q_D(PrefsRunProgramWidget);

  auto conditionsToolTip = Q("%1 %2")
    .arg(QY("If any of these checkboxes is checked, the action will be executed when the corresponding condition is met."))
    .arg(QY("Independent of the checkboxes, every active configuration can be triggered manually from the \"job output\" tool."));

  Util::setToolTip(d->ui->leName, QY("This is an arbitrary name the GUI can use to refer to this particular configuration."));
  Util::setToolTip(d->ui->cbConfigurationActive, QY("Deactivating this checkbox is a way to disable a configuration temporarily without having to change its parameters."));
  Util::setToolTip(d->ui->pbExecuteNow, Q("%1 %2").arg(QY("Executes the action immediately.")).arg(QY("Note that most <MTX_…> variables are empty and will be removed for actions that can take variables as arguments.")));
  Util::setToolTip(d->ui->cbAfterJobQueueStopped, conditionsToolTip);
  Util::setToolTip(d->ui->cbAfterJobSuccessful,   conditionsToolTip);
  Util::setToolTip(d->ui->cbAfterJobError,        conditionsToolTip);
}

void
PrefsRunProgramWidget::setupMenu() {
  Q_D(PrefsRunProgramWidget);

  QList<std::pair<QString, QString> > entries{
    { QY("Variables for all job types"),                                 Q("")                           },
    { QY("Job description"),                                             Q("JOB_DESCRIPTION")            },
    { QY("Job start date && time in ISO 8601 format"),                   Q("JOB_START_TIME")             },
    { QY("Job end date && time in ISO 8601 format"),                     Q("JOB_END_TIME")               },
    { QY("Exit code (0: ok, 1: warnings occurred, 2: errors occurred)"), Q("JOB_EXIT_CODE")              },

    { QY("Variables for multiplex jobs"),                                Q("")                           },
    { QY("Destination file's name"),                                     Q("DESTINATION_FILE_NAME")      },
    { QY("Destination file's directory"),                                Q("DESTINATION_FILE_DIRECTORY") },
    { QY("Source file names"),                                           Q("SOURCE_FILE_NAMES")          },

    { QY("General variables"),                                           Q("")                           },
    { QY("Current date && time in ISO 8601 format"),                     Q("CURRENT_TIME")               },
    { QY("MKVToolNix GUI's installation directory"),                     Q("INSTALLATION_DIRECTORY")     },
  };

  d->variableMenu.reset(new QMenu{this});

  for (auto const &entry : entries)
    if (entry.second.isEmpty())
      d->variableMenu->addSection(entry.first);

    else {
      auto action = d->variableMenu->addAction(Q("<MTX_%1> – %2").arg(entry.second).arg(entry.first));
      connect(action, &QAction::triggered, [this, entry]() {
        this->addVariable(Q("<MTX_%1>").arg(entry.second));
      });
    }
}

void
PrefsRunProgramWidget::setupConnections() {
  Q_D(PrefsRunProgramWidget);

  connect(d->ui->cbConfigurationActive, &QCheckBox::toggled,                                                    this, &PrefsRunProgramWidget::enableControls);
  connect(d->ui->leName,                &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::nameEdited);
  connect(d->ui->cbType,                static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PrefsRunProgramWidget::typeChanged);
  connect(d->ui->leCommandLine,         &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::commandLineEdited);
  connect(d->ui->pbBrowseExecutable,    &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeExecutable);
  connect(d->ui->pbAddVariable,         &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::selectVariableToAdd);
  connect(d->ui->pbExecuteNow,          &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::executeNow);
  connect(d->ui->leAudioFile,           &QLineEdit::textEdited,                                                 this, &PrefsRunProgramWidget::audioFileEdited);
  connect(d->ui->pbBrowseAudioFile,     &QPushButton::clicked,                                                  this, &PrefsRunProgramWidget::changeAudioFile);
}

void
PrefsRunProgramWidget::selectVariableToAdd() {
  Q_D(PrefsRunProgramWidget);

  d->variableMenu->exec(QCursor::pos());
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
  Q_D(PrefsRunProgramWidget);

  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + Q(" (*.exe *.bat *.cmd)");
#endif
  filters << QY("All files") + Q(" (*)");

  auto realExecutable = Util::replaceMtxVariableWithApplicationDirectory(d->executable);
  auto newExecutable  = Util::getOpenFileName(this, QY("Select executable"), realExecutable, filters.join(Q(";;")));
  newExecutable       = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(newExecutable));

  if (newExecutable.isEmpty() || (newExecutable == d->executable))
    return;

  changeArguments([&newExecutable](QStringList &arguments) {
    if (arguments.isEmpty())
      arguments << newExecutable;
    else
      arguments[0] = newExecutable;
  });

  d->executable = newExecutable;

  enableControls();

  emit titleChanged();
}

void
PrefsRunProgramWidget::commandLineEdited(QString const &commandLine) {
  Q_D(PrefsRunProgramWidget);

  auto arguments     = Util::unescapeSplit(commandLine, Util::EscapeShellUnix);
  auto newExecutable = arguments.value(0);

  if (d->executable == newExecutable)
    return;

  d->executable = newExecutable;

  enableControls();

  emit titleChanged();
}

void
PrefsRunProgramWidget::nameEdited() {
  emit titleChanged();
}

void
PrefsRunProgramWidget::changeArguments(std::function<void(QStringList &)> const &worker) {
  Q_D(PrefsRunProgramWidget);

  auto arguments = Util::unescapeSplit(d->ui->leCommandLine->text(), Util::EscapeShellUnix);

  worker(arguments);

  d->ui->leCommandLine->setText(Util::escape(arguments, Util::EscapeShellUnix).join(Q(" ")));
}

Util::Settings::RunProgramConfigPtr
PrefsRunProgramWidget::config()
  const {
  Q_D(const PrefsRunProgramWidget);

  auto cfg           = std::make_shared<Util::Settings::RunProgramConfig>();
  auto cmdLine       = d->ui->leCommandLine->text().replace(QRegularExpression{"^\\s+"}, Q(""));
  cfg->m_name        = d->ui->leName->text();
  cfg->m_type        = static_cast<Util::Settings::RunProgramType>(d->ui->cbType->currentData().value<int>());
  cfg->m_commandLine = Util::unescapeSplit(cmdLine, Util::EscapeShellUnix);
  cfg->m_active      = d->ui->cbConfigurationActive->isChecked();
  cfg->m_audioFile   = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(d->ui->leAudioFile->text()));
  cfg->m_volume      = d->ui->sbVolume->value();

  for (auto const &checkBox : d->flagsByCheckbox.keys())
    if (checkBox->isChecked())
      cfg->m_forEvents |= d->flagsByCheckbox[checkBox];

  return cfg;
}

void
PrefsRunProgramWidget::changeAudioFile() {
  Q_D(PrefsRunProgramWidget);

  auto filters = QStringList{} << QY("Audio files") + Q(" (*.aac *.flac *.m4a *.mp3 *.ogg *.opus *.wav)")
                               << QY("All files")   + Q(" (*)");

  auto realAudioFile = Util::replaceMtxVariableWithApplicationDirectory(d->ui->leAudioFile->text());
  auto newAudioFile  = Util::getOpenFileName(this, QY("Select audio file"), realAudioFile, filters.join(Q(";;")));
  newAudioFile       = QDir::toNativeSeparators(Util::replaceApplicationDirectoryWithMtxVariable(newAudioFile));

  if (newAudioFile.isEmpty())
    return;

  d->ui->leAudioFile->setText(newAudioFile);

  enableControls();

  emit titleChanged();
}

void
PrefsRunProgramWidget::audioFileEdited() {
  enableControls();

  emit titleChanged();
}

void
PrefsRunProgramWidget::typeChanged(int index) {
  Q_D(PrefsRunProgramWidget);

  auto type = static_cast<Util::Settings::RunProgramType>(d->ui->cbType->itemData(index).value<int>());
  showPageForType(type);

  emit titleChanged();

  enableControls();
}

void
PrefsRunProgramWidget::showPageForType(Util::Settings::RunProgramType type) {
  Q_D(PrefsRunProgramWidget);

  d->ui->typeWidgets->setCurrentWidget(d->pagesByType[type]);
  d->ui->gbTypeSpecificSettings->setVisible(d->pagesByType[type] != d->ui->emptyTypePage);
}

QString
PrefsRunProgramWidget::validate()
  const {
  auto cfg = config();
  return cfg->m_active ? cfg->validate() : QString{};
}

}}
