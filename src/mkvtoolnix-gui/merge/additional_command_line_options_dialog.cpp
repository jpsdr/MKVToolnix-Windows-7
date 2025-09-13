#include "common/common_pch.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

#include "common/hacks.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

AdditionalCommandLineOptionsDialog::AdditionalCommandLineOptionsDialog(QWidget *parent,
                                                                       QString const &options)
  : QDialog{parent, Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
  , m_ui{new Ui::AdditionalCommandLineOptionsDialog}
  , m_customOptions{options}
{
  // Setup UI controls.
  m_ui->setupUi(this);

  auto global = m_ui->gridGlobalOutputControl;

  add(Q("--abort-on-warnings"), false, global, { QY("Tells mkvmerge to abort after the first warning is emitted.") });
  add(Q("--append-mode"),       true,  global, { QY("Selects how mkvmerge calculates timestamps when appending files."),
                                                 QY("The default is 'file' with 'track' being an alternative mode.") });
  add(Q("--cluster-length"), true, global,
      { QY("This option needs an additional argument 'n'."),
        QY("Tells mkvmerge to put at most 'n' data blocks into each cluster."),
        QY("If the number is postfixed with 'ms' then put at most 'n' milliseconds of data into each cluster."),
        QY("The maximum length for a cluster that mkvmerge accepts is 60000 blocks and 32000ms; the minimum length is 100ms."),
        QY("Programs will only be able to seek to clusters, so creating larger clusters may lead to imprecise or slow seeking.") });

  add(Q("--clusters-in-meta-seek"),         false, global, { QY("Tells mkvmerge to create a meta seek element at the end of the file containing all clusters.") });
  add(Q("--deterministic"),                 true,  global, { QY("Enables the creation of byte-identical files if the same source files with the same options and the same seed are used.") });
  add(Q("--disable-lacing"),                false, global, { QY("Disables lacing for all tracks."), QY("This will increase the file's size, especially if there are many audio tracks."), QY("Use this only for testing purposes.") });
  add(Q("--disable-track-statistics-tags"), false, global, { QY("Tells mkvmerge not to write tags with statistics for each track.") });
  add(Q("--disable-language-ietf"),         false, global, { QY("Tells mkvmerge not to write LanguageIETF track header elements and ChapLanguageIETF chapter elements.") });
  add(Q("--enable-durations"),              false, global, { QY("Write durations for all blocks."), QY("This will increase file size and does not offer any additional value for players at the moment.") });

  add(Q("--flush-on-close"), false, global,
      { QY("Tells mkvmerge to flush all data cached in memory to storage when closing files opened for writing."),
        QY("This can be used to prevent data loss on power outages or to circumvent certain problems in the operating system or drivers."),
        QY("The downside is that multiplexing will take longer as mkvmerge will wait until all data has been written to the storage before exiting."),
        QY("See issues #2469 and #2480 on the MKVToolNix bug tracker for in-depth discussions on the pros and cons.") });

  add(Q("--stop-after-video-ends"), false, global, { QY("Stops processing after the primary video track ends, discarding any remaining packets of other tracks.") });

  add(Q("--no-cues"), false, global,
      { QY("Tells mkvmerge not to create and write the cue data which can be compared to an index in an AVI."),
        QY("Matroska files can be played back without the cue data, but seeking will probably be imprecise and slower."),
        QY("Use this only for testing purposes.") });

  add(Q("--no-date"), false, global, { QY("Tells mkvmerge not to write the 'date' field in the segment information headers."),
                                       QY("This field is normally set to the date the file is created.") });
  add(Q("--date"), true, global, { QY("Sets the 'date' field (UTC or with a time zone offset) in the segment information headers."),
                                   QY("This field is normally set to the date the file is created.") });

  add(Q("--timestamp-scale"), true,  global,
      { QY("Forces the timestamp scale factor to the given value."),
        QY("You have to enter a value between 1000 and 10000000 or the magic value -1."),
        QY("Normally mkvmerge will use a value of 1000000 which means that timestamps and durations will have a precision of 1ms."),
        QY("For files that will not contain a video track but at least one audio track mkvmerge will automatically choose a timestamp scale factor so that all timestamps and durations have a precision of one sample."),
        QY("This causes bigger overhead but allows precise seeking and extraction."),
        QY("If the magical value -1 is used then mkvmerge will use sample precision even if a video track is present.") });

  add(Q("--regenerate-track-uids"), false, global,
      { QY("Generate new random track UIDs instead of keeping existing ones."),
        QY("When given, this option applies to all source files.") });

  auto hacks       = m_ui->gridDevelopmentHacks;
  auto listOfHacks = mtx::hacks::get_list();

  std::sort(listOfHacks.begin(), listOfHacks.end(), [](auto const &a, auto const &b) { return a.name < b.name; });

  for (auto const &hack : listOfHacks)
    add(Q("--engage %1").arg(Q(hack.name)), false, hacks, { Q(mtx::string::join(hack.description, " ")) });

  m_ui->gbGlobalOutputControl->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  m_ui->gbDevelopmentHacks   ->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

  m_customOptions.replace(QRegularExpression{"^\\s+|\\s+$"}, Q(""));

  Util::restoreWidgetGeometry(this);

  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &AdditionalCommandLineOptionsDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &AdditionalCommandLineOptionsDialog::reject);
}

AdditionalCommandLineOptionsDialog::~AdditionalCommandLineOptionsDialog() {
  Util::saveWidgetGeometry(this);
}

void
AdditionalCommandLineOptionsDialog::hideSaveAsDefaultCheckbox() {
  m_ui->cbSaveAsDefault->setVisible(false);
}

void
AdditionalCommandLineOptionsDialog::add(QString const &title,
                                        bool requiresValue,
                                        QGridLayout *layout,
                                        QStringList const &description) {
  auto checkBox = new QCheckBox{this};
  auto option   = std::make_shared<Option>(title, checkBox);

  checkBox->setText(title);

  m_options << option;

  layout->addWidget(checkBox, layout->rowCount(), 0, Qt::AlignTop);

  auto lDescription = new QLabel{this};
  lDescription->setText(description.join(Q(" ")));
  lDescription->setWordWrap(true);
  layout->addWidget(lDescription, layout->rowCount() - 1, layout->columnCount() - 1, Qt::AlignTop);


  if (requiresValue) {
    option->value = new QLineEdit{this};
    option->value->setMinimumWidth(80);
    option->value->setEnabled(false);

    layout->addWidget(option->value, layout->rowCount() - 1, 1, Qt::AlignTop);

    connect(checkBox,      &QCheckBox::toggled,     this, &AdditionalCommandLineOptionsDialog::enableOkButton);
    connect(option->value, &QLineEdit::textChanged, this, &AdditionalCommandLineOptionsDialog::enableOkButton);
  }

  auto re    = QRegularExpression{ requiresValue ? Q("\\s*%1\\s+([^\\s]+)\\s*").arg(QRegularExpression::escape(title)) : Q("\\s*%1\\s*").arg(QRegularExpression::escape(title)) };
  auto match = re.match(m_customOptions);

  if (!match.hasMatch())
    return;

  checkBox->setChecked(true);
  if (requiresValue) {
    option->value->setText(match.captured(1));
    option->value->setEnabled(true);
  }

  m_customOptions.replace(match.capturedStart(0), match.capturedLength(0), Q(" "));
}

void
AdditionalCommandLineOptionsDialog::enableOkButton() {
  for (auto const &option : m_options)
    if (option->control == QObject::sender()) {
      if (option->value) {
        option->value->setEnabled(option->control->isChecked());
        option->value->setFocus();
      }

      break;
    }

  for (auto const &option : m_options)
    if (option->value && option->control->isChecked() && option->value->text().isEmpty()) {
      Util::buttonForRole(m_ui->buttonBox, QDialogButtonBox::AcceptRole)->setEnabled(false);
      return;
    }

  Util::buttonForRole(m_ui->buttonBox, QDialogButtonBox::AcceptRole)->setEnabled(true);
}

QString
AdditionalCommandLineOptionsDialog::additionalOptions()
  const {
  auto options = QStringList{};

  if (!m_customOptions.isEmpty())
    options << m_customOptions;

  for (auto const &option : m_options)
    if (option->control->isChecked())
      options << (option->value ? Q("%1 %2").arg(option->title).arg(option->value->text()) : option->title);

  return options.join(Q(" "));
}

bool
AdditionalCommandLineOptionsDialog::saveAsDefault()
  const {
  return m_ui->cbSaveAsDefault->isChecked();
}

}
