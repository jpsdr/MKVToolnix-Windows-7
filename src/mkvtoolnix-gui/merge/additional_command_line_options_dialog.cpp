#include "common/common_pch.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

#include "common/qt.h"
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

  add(Q("--cluster-length"), true, global,
      { QY("This option needs an additional argument 'n'."),
        QY("Tells mkvmerge to put at most 'n' data blocks into each cluster."),
        QY("If the number is postfixed with 'ms' then put at most 'n' milliseconds of data into each cluster."),
        QY("The maximum length for a cluster that mkvmerge accepts is 60000 blocks and 32000ms; the minimum length is 100ms."),
        QY("Programs will only be able to seek to clusters, so creating larger clusters may lead to imprecise or slow seeking.") });

  add(Q("--no-cues"), false, global,
      { QY("Tells mkvmerge not to create and write the cue data which can be compared to an index in an AVI."),
        QY("Matroska files can be played back without the cue data, but seeking will probably be imprecise and slower."),
        QY("Use this only for testing purposes.") });

  add(Q("--clusters-in-meta-seek"),         false, global, { QY("Tells mkvmerge to create a meta seek element at the end of the file containing all clusters.") });
  add(Q("--no-date"),                       false, global, { QY("Tells mkvmerge not to write the 'date' field in the segment information headers."),
                                                             QY("This field is normally set to the date the file is created.") });
  add(Q("--disable-lacing"),                false, global, { QY("Disables lacing for all tracks."), QY("This will increase the file's size, especially if there are many audio tracks."), QY("Use only for testing.") });
  add(Q("--enable-durations"),              false, global, { QY("Write durations for all blocks."), QY("This will increase file size and does not offer any additional value for players at the moment.") });
  add(Q("--disable-track-statistics-tags"), false, global, { QY("Tells mkvmerge not to write tags with statistics for each track.") });
  add(Q("--timestamp-scale"),               true,  global,
      { QY("Forces the timestamp scale factor to the given value."),
        QY("You have to enter a value between 1000 and 10000000 or the magic value -1."),
        QY("Normally mkvmerge will use a value of 1000000 which means that timestamps and durations will have a precision of 1ms."),
        QY("For files that will not contain a video track but at least one audio track mkvmerge will automatically choose a timestamp scale factor so that all timestamps and durations have a precision of one sample."),
        QY("This causes bigger overhead but allows precise seeking and extraction."),
        QY("If the magical value -1 is used then mkvmerge will use sample precision even if a video track is present.") });

  add(Q("--flush-on-close"), false, global,
      { QY("Tells mkvmerge to flush all data cached in memory to storage when closing files opened for writing."),
        QY("This can be used to prevent data loss on power outages or to circumvent certain problems in the operating system or drivers."),
        QY("The downside is that multiplexing will take longer as mkvmerge will wait until all data has been written to the storage before exiting."),
        QY("See issues #2469 and #2480 on the MKVToolNix bug tracker for in-depth discussions on the pros and cons.") });

  add(Q("--abort-on-warnings"), false, global, { QY("Tells mkvmerge to abort after the first warning is emitted.") });

  add(Q("--deterministic"), true, global, { QY("Enables the creation of byte-identical files if the same source files with the same options and the same seed are used.") });

  auto hacks  = m_ui->gridDevelopmentHacks;

  add(Q("--engage space_after_chapters"),         false, hacks, { QY("Leave additional space (EbmlVoid) in the destination file after the chapters.") });
  add(Q("--engage no_chapters_in_meta_seek"),     false, hacks, { QY("Do not add an entry for the chapters in the meta seek element.") });
  add(Q("--engage no_meta_seek"),                 false, hacks, { QY("Do not write meta seek elements at all.") });
  add(Q("--engage lacing_xiph"),                  false, hacks, { QY("Force Xiph style lacing.") });
  add(Q("--engage lacing_ebml"),                  false, hacks, { QY("Force EBML style lacing.") });
  add(Q("--engage native_mpeg4"),                 false, hacks, { QY("Analyze MPEG4 bitstreams, put each frame into one Matroska block, use proper timestamping (I P B B = 0 120 40 80), use V_MPEG4/ISO/... CodecIDs.") });
  add(Q("--engage no_variable_data"),             false, hacks,
      { QY("Use fixed values for the elements that change with each file otherwise (multiplexing date, segment UID, track UIDs etc.)."),
        QY("Two files multiplexed with the same settings and this switch activated will be identical.") });
  add(Q("--engage force_passthrough_packetizer"), false, hacks, { QY("Forces the Matroska reader to use the generic passthrough packetizer even for known and supported track types.") });
  add(Q("--engage allow_avc_in_vfw_mode"),        false, hacks, { QY("Allows storing AVC/H.264 video in Video-for-Windows compatibility mode, e.g. when it is read from an AVI.") });
  add(Q("--engage no_simpleblocks"),              false, hacks, { QY("Disable the use of SimpleBlocks instead of BlockGroups.") });
  add(Q("--engage use_codec_state_only"),         false, hacks, { QY("Store changes in CodecPrivate data in CodecState elements instead of the frames."),
                                                                  QY("This is used for e.g. MPEG-1/-2 video tracks for storing the sequence headers.") });
  add(Q("--engage remove_bitstream_ar_info"),     false, hacks,
      { QY("Normally mkvmerge keeps aspect ratio information in MPEG4 video bitstreams and puts the information into the container."),
        QY("This option causes mkvmerge to remove the aspect ratio information from the bitstream.") });
  add(Q("--engage vobsub_subpic_stop_cmds"),      false, hacks, { QY("Causes mkvmerge to add 'stop display' commands to VobSub subtitle packets that do not contain a duration field.") });
  add(Q("--engage no_cue_duration"),              false, hacks, { QY("Causes mkvmerge not to write 'CueDuration' elements in the cues.") });
  add(Q("--engage no_cue_relative_position"),     false, hacks, { QY("Causes mkvmerge not to write 'CueRelativePosition' elements in the cues.") });
  add(Q("--engage no_delay_for_garbage_in_avi"),  false, hacks,
      { QY("Garbage at the start of audio tracks in AVI files is normally used for delaying that track."),
        QY("mkvmerge normally calculates the delay implied by its presence and offsets all of the track's timestamps by it."),
        QY("This option prevents that behavior.") });
  add(Q("--engage keep_last_chapter_in_mpls"),    false, hacks,
      { QY("Blu-ray discs often contain a chapter entry very close to the end of the movie."),
        QY("mkvmerge normally removes that last entry if it's timestamp is within five seconds of the total duration."),
        QY("Enabling this option causes mkvmerge to keep that last entry.") });
  add(Q("--engage all_i_slices_are_key_frames"),  false, hacks,
      { QY("Some H.264/AVC tracks contain I slices but lack real key frames."),
        QY("This option forces mkvmerge to treat all of those I slices as key frames.") });
  add(Q("--engage append_and_split_flac"), false, hacks,
      { QY("Enable appending and splitting FLAC tracks."),
        QY("The resulting tracks will be broken: the official FLAC tools will not be able to decode them and seeking will not work as expected.") });
  add(Q("--engage cow"),                          false, hacks, { QY("No help available.") });

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
