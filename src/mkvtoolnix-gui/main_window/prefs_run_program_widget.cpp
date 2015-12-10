#include "common/common_pch.h"

#include <QAction>
#include <QCursor>
#include <QMenu>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/main_window/prefs_run_program_widget.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx { namespace gui {

class PrefsRunProgramWidgetPrivate {
  friend class PrefsRunProgramWidget;

  std::unique_ptr<Ui::PrefsRunProgramWidget> ui;
  std::unique_ptr<QMenu> variableMenu;
  QString executable;
  QMap<QCheckBox *, Util::Settings::RunProgramForEvent> flagsByCheckbox;

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
  setupMenu();
  setupConnections();
}

PrefsRunProgramWidget::~PrefsRunProgramWidget() {
}

void
PrefsRunProgramWidget::setupUi(Util::Settings::RunProgramConfig const &cfg) {
  Q_D(PrefsRunProgramWidget);

  // Setup UI controls.
  d->ui->setupUi(this);

  d->flagsByCheckbox[d->ui->cbAfterJobQueueStopped] = Util::Settings::RunAfterJobQueueFinishes;
  d->flagsByCheckbox[d->ui->cbAfterJobSuccessful]   = Util::Settings::RunAfterJobCompletesSuccessfully;
  d->flagsByCheckbox[d->ui->cbAfterJobError]        = Util::Settings::RunAfterJobCompletesWithErrors;

  d->executable = cfg.m_commandLine.value(0);
  d->ui->leCommandLine->setText(Util::escape(cfg.m_commandLine, Util::EscapeShellUnix).join(" "));

  for (auto const &checkBox : d->flagsByCheckbox.keys())
    if (cfg.m_forEvents & d->flagsByCheckbox[checkBox])
      checkBox->setChecked(true);
}


void
PrefsRunProgramWidget::setupMenu() {
  Q_D(PrefsRunProgramWidget);

  QList<std::pair<QString, QString> > entries{
    { QY("Variables for all job types"),                                 Q("")                      },
    { QY("Job description"),                                             Q("JOB_DESCRIPTION")       },
    { QY("Job start date & time in ISO 8601 format"),                    Q("JOB_START_TIME")        },
    { QY("Job end date & time in ISO 8601 format"),                      Q("JOB_END_TIME")          },
    { QY("Exit code (0: ok, 1: warnings occurred, 2: errors occurred)"), Q("JOB_EXIT_CODE")         },

    { QY("Variables for merge jobs"),                                    Q("")                      },
    { QY("Output file's name"),                                          Q("OUTPUT_FILE_NAME")      },
    { QY("Output file's directory"),                                     Q("OUTPUT_FILE_DIRECTORY") },
    { QY("Source file names"),                                           Q("SOURCE_FILE_NAMES")     },

    { QY("General variables"),                                           Q("")                      },
    { QY("Current date & time in ISO 8601 format"),                      Q("CURRENT_TIME")          },
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

  connect(d->ui->leCommandLine,      &QLineEdit::textEdited, this, &PrefsRunProgramWidget::commandLineEdited);
  connect(d->ui->pbBrowseExecutable, &QPushButton::clicked,  this, &PrefsRunProgramWidget::changeExecutable);
  connect(d->ui->pbAddVariable,      &QPushButton::clicked,  this, &PrefsRunProgramWidget::selectVariableToAdd);
}

void
PrefsRunProgramWidget::selectVariableToAdd() {
  Q_D(PrefsRunProgramWidget);

  d->variableMenu->exec(QCursor::pos());
}

void
PrefsRunProgramWidget::addVariable(QString const &variable) {
  changeArguments([&](QStringList &arguments) {
    arguments << variable;
  });
}

bool
PrefsRunProgramWidget::isValid()
  const {
  Q_D(const PrefsRunProgramWidget);

  auto arguments = Util::unescapeSplit(d->ui->leCommandLine->text(), Util::EscapeShellUnix);
  return !arguments.value(0).isEmpty();
}

void
PrefsRunProgramWidget::changeExecutable() {
  Q_D(PrefsRunProgramWidget);

  auto filters = QStringList{};

#if defined(SYS_WINDOWS)
  filters << QY("Executable files") + Q(" (*.exe *.bat *.cmd)");
#endif
  filters << QY("All files") + Q(" (*)");

  auto newExecutable = Util::getOpenFileName(this, QY("Select executable"), d->executable, filters.join(Q(";;")));
  if (newExecutable.isEmpty() || (newExecutable == d->executable))
    return;

  changeArguments([&](QStringList &arguments) {
    if (arguments.isEmpty())
      arguments << newExecutable;
    else
      arguments[0] = newExecutable;
  });

  d->executable = newExecutable;

  emit executableChanged(newExecutable);
}

void
PrefsRunProgramWidget::commandLineEdited(QString const &commandLine) {
  Q_D(PrefsRunProgramWidget);

  auto arguments     = Util::unescapeSplit(commandLine, Util::EscapeShellUnix);
  auto newExecutable = arguments.value(0);

  if (d->executable == newExecutable)
    return;

  d->executable = newExecutable;

  emit executableChanged(newExecutable);
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
  cfg->m_commandLine = Util::unescapeSplit(d->ui->leCommandLine->text(), Util::EscapeShellUnix);

  for (auto const &checkBox : d->flagsByCheckbox.keys())
    if (checkBox->isChecked())
      cfg->m_forEvents |= d->flagsByCheckbox[checkBox];

  return cfg;
}

}}
