#include "common/common_pch.h"

#include "common/bitvalue.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

void
Tab::setupOutputControls() {
  auto &cfg = Util::Settings::get();

  cfg.handleSplitterSizes(ui->mergeOutputSplitter);

  for (auto idx = 0; idx < 8; ++idx)
    ui->splitMode->addItem(QString{}, idx);

  setupOutputFileControls();

  ui->chapterCharacterSetPreview->setEnabled(false);

  m_splitControls << ui->splitOptions << ui->splitOptionsLabel << ui->splitMaxFilesLabel << ui->splitMaxFiles << ui->linkFiles;

  auto comboBoxControls = QList<QComboBox *>{} << ui->splitMode << ui->chapterLanguage << ui->chapterCharacterSet << ui->chapterGenerationMode;
  for (auto const &control : comboBoxControls) {
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    Util::fixComboBoxViewWidth(*control);
  }

  ui->splitMaxFiles->setMaximum(std::numeric_limits<int>::max());

  onSplitModeChanged(MuxConfig::DoNotSplit);
  onChapterGenerationModeChanged();

  connect(MainWindow::get(),                 &MainWindow::preferencesChanged,                                                                  this, &Tab::setupOutputFileControls);

  connect(ui->additionalOptions,             &QLineEdit::textChanged,                                                                          this, &Tab::onAdditionalOptionsChanged);
  connect(ui->browseChapters,                &QPushButton::clicked,                                                                            this, &Tab::onBrowseChapters);
  connect(ui->browseGlobalTags,              &QPushButton::clicked,                                                                            this, &Tab::onBrowseGlobalTags);
  connect(ui->browseNextSegmentUID,          &QPushButton::clicked,                                                                            this, &Tab::onBrowseNextSegmentUID);
  connect(ui->browseOutput,                  &QPushButton::clicked,                                                                            this, &Tab::onBrowseOutput);
  connect(ui->browsePreviousSegmentUID,      &QPushButton::clicked,                                                                            this, &Tab::onBrowsePreviousSegmentUID);
  connect(ui->browseSegmentInfo,             &QPushButton::clicked,                                                                            this, &Tab::onBrowseSegmentInfo);
  connect(ui->browseSegmentUID,              &QPushButton::clicked,                                                                            this, &Tab::onBrowseSegmentUID);
  connect(ui->chapterCharacterSet,           static_cast<void (QComboBox::*)(QString const &)>(&QComboBox::currentIndexChanged),               this, &Tab::onChapterCharacterSetChanged);
  connect(ui->chapterCharacterSetPreview,    &QPushButton::clicked,                                                                            this, &Tab::onPreviewChapterCharacterSet);
  connect(ui->chapterCueNameFormat,          &QLineEdit::textChanged,                                                                          this, &Tab::onChapterCueNameFormatChanged);
  connect(ui->chapterLanguage,               static_cast<void (Util::LanguageComboBox::*)(int)>(&Util::LanguageComboBox::currentIndexChanged), this, &Tab::onChapterLanguageChanged);
  connect(ui->chapters,                      &QLineEdit::textChanged,                                                                          this, &Tab::onChaptersChanged);
  connect(ui->globalTags,                    &QLineEdit::textChanged,                                                                          this, &Tab::onGlobalTagsChanged);
  connect(ui->linkFiles,                     &QPushButton::clicked,                                                                            this, &Tab::onLinkFilesClicked);
  connect(ui->nextSegmentUID,                &QLineEdit::textChanged,                                                                          this, &Tab::onNextSegmentUIDChanged);
  connect(ui->output,                        &QLineEdit::textChanged,                                                                          this, &Tab::setDestination);
  connect(ui->previousSegmentUID,            &QLineEdit::textChanged,                                                                          this, &Tab::onPreviousSegmentUIDChanged);
  connect(ui->segmentInfo,                   &QLineEdit::textChanged,                                                                          this, &Tab::onSegmentInfoChanged);
  connect(ui->segmentUIDs,                   &QLineEdit::textChanged,                                                                          this, &Tab::onSegmentUIDsChanged);
  connect(ui->splitMaxFiles,                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),                                    this, &Tab::onSplitMaxFilesChanged);
  connect(ui->splitMode,                     static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this, &Tab::onSplitModeChanged);
  connect(ui->splitOptions,                  &QComboBox::editTextChanged,                                                                      this, &Tab::onSplitOptionsChanged);
  connect(ui->title,                         &QLineEdit::textChanged,                                                                          this, &Tab::onTitleChanged);
  connect(ui->webmMode,                      &QPushButton::clicked,                                                                            this, &Tab::onWebmClicked);
  connect(ui->chapterGenerationMode,         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this, &Tab::onChapterGenerationModeChanged);
  connect(ui->chapterGenerationNameTemplate, &QLineEdit::textChanged,                                                                          this, &Tab::onChapterGenerationNameTemplateChanged);
  connect(ui->chapterGenerationInterval,     &QLineEdit::textChanged,                                                                          this, &Tab::onChapterGenerationIntervalChanged);
}

void
Tab::setupOutputFileControls() {
  if (Util::Settings::get().m_mergeAlwaysShowOutputFileControls)
    moveOutputFileNameToGlobal();
  else
    moveOutputFileNameToOutputTab();
}

void
Tab::moveOutputFileNameToGlobal() {
  if (!ui->hlOutput)
    return;

  ui->outputLabel->setParent(ui->gbOutputFile);
  ui->output->setParent(ui->gbOutputFile);
  ui->browseOutput->setParent(ui->gbOutputFile);

  ui->outputLabel->show();
  ui->output->show();
  ui->browseOutput->show();

  delete ui->hlOutput;
  ui->hlOutput = nullptr;

  delete ui->gbOutputFile->layout();
  auto layout = new QHBoxLayout{ui->gbOutputFile};
  layout->setSpacing(6);
  layout->setContentsMargins(6, 6, 6, 6);

  layout->addWidget(ui->outputLabel);
  layout->addWidget(ui->output);
  layout->addWidget(ui->browseOutput);

  ui->gbOutputFile->show();
}

void
Tab::moveOutputFileNameToOutputTab() {
    ui->gbOutputFile->hide();

  if (ui->hlOutput)
    return;

  ui->gbOutputFile->hide();

  ui->outputLabel->setParent(ui->generalBox);
  ui->output->setParent(ui->generalBox);
  ui->browseOutput->setParent(ui->generalBox);

  ui->outputLabel->show();
  ui->output->show();
  ui->browseOutput->show();

  ui->hlOutput = new QHBoxLayout{ui->gbOutputFile};
  ui->hlOutput->setSpacing(6);
  ui->hlOutput->addWidget(ui->output);
  ui->hlOutput->addWidget(ui->browseOutput);

  ui->generalGridLayout->addWidget(ui->outputLabel, 1, 0, 1, 1);
  ui->generalGridLayout->addLayout(ui->hlOutput,    1, 1, 1, 1);
}

void
Tab::retranslateOutputUI() {
  Util::setComboBoxTexts(ui->splitMode,
                         QStringList{} << QY("Do not split")                 << QY("After output size")                     << QY("After output duration")     << QY("After specific timestamps")
                                       << QY("By parts based on timestamps") << QY("By parts based on frame/field numbers") << QY("After frame/field numbers") << QY("Before chapters"));

  setupOutputToolTips();
}

void
Tab::setupOutputToolTips() {
  Util::setToolTip(ui->title, QY("This is the title that players may show as the 'main title' for this movie."));
  Util::setToolTip(ui->splitMode,
                   Q("%1 %2")
                   .arg(QY("Enables splitting of the output into more than one file."))
                   .arg(QY("You can split based on the amount of time passed, based on timestamps, on frame/field numbers or on chapter numbers.")));
  Util::setToolTip(ui->splitMaxFiles,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The maximum number of files that will be created even if the last file might contain more bytes/time than wanted."))
                   .arg(QYH("Useful e.g. when you want exactly two files."))
                   .arg(QYH("If you leave this empty then there is no limit for the number of files mkvmerge might create.")));
  Util::setToolTip(ui->linkFiles,
                   Q("%1 %2")
                   .arg(QY("Use 'segment linking' for the resulting files."))
                   .arg(QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation.")));
  Util::setToolTip(ui->segmentUIDs,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p>")
                   .arg(QYH("Sets the segment UIDs to use."))
                   .arg(QYH("This is a comma-separated list of 128bit segment UIDs in the usual UID form: hex numbers with or without the \"0x\" prefix, with or without spaces, exactly 32 digits."))
                   .arg(QYH("Each file created contains one segment, and each segment has one segment UID."))
                   .arg(QYH("If more segment UIDs are specified than segments are created then the surplus UIDs are ignored."))
                   .arg(QYH("If fewer UIDs are specified than segments are created then random UIDs will be created for them.")));
  Util::setToolTip(ui->previousSegmentUID, QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(ui->nextSegmentUID,     QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(ui->chapters,           QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(ui->browseChapters,     QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(ui->browseSegmentUID,         QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(ui->browseNextSegmentUID,     QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(ui->browsePreviousSegmentUID, QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(ui->chapterLanguage,
                   Q("<p>%1 %2 %3</p><p>%4</p>")
                   .arg(QYH("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                   .arg(QYH("This option specifies the language to be associated with chapters if the OGM chapter format is used."))
                   .arg(QYH("It is ignored for XML chapter files."))
                   .arg(QYH("The language set here is also used when chapters are generated.")));
  Util::setToolTip(ui->chapterCharacterSet,
                   Q("%1 %2 %3")
                   .arg(QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                   .arg(QY("If the OGM format is used and the file's character set is not recognized correctly then this option can be used to correct that."))
                   .arg(QY("It is ignored for XML chapter files.")));
  Util::setToolTip(ui->chapterCueNameFormat,
                   Q("<p>%1 %2 %3 %4</p><p>%5</p>")
                   .arg(QYH("mkvmerge can read cue sheets for audio CDs and automatically convert them to chapters."))
                   .arg(QYH("This option controls how the chapter names are created."))
                   .arg(QYH("The sequence '%p' is replaced by the track's PERFORMER, the sequence '%t' by the track's TITLE, '%n' by the track's number and '%N' by the track's number padded with a leading 0 for track numbers < 10."))
                   .arg(QYH("The rest is copied as is."))
                   .arg(QYH("If nothing is entered then '%p - %t' will be used.")));
  Util::setToolTip(ui->chapterGenerationMode,
                   Q("<p>%1 %2</p><ol><li>%3</li><li>%4</li></ol><p>%5</p>")
                   .arg(QYH("mkvmerge can generate chapters automatically."))
                   .arg(QYH("The following modes are supported:"))
                   .arg(QYH("When appending: one chapter is created at the start and one whenever a file is appended."))
                   .arg(QYH("In fixed intervals: chapters are created in fixed intervals, e.g. every 30 seconds."))
                   .arg(QYH("The language for the newly created chapters is set via the chapter language control above.")));
  Util::setToolTip(ui->chapterGenerationNameTemplate,
                   Q("%1<p>%2</p>")
                   .arg(ChapterEditor::Tool::chapterNameTemplateToolTip())
                   .arg(QYH("If nothing is entered the default 'Chapter <NUM:2>' will be used.")));
  Util::setToolTip(ui->chapterGenerationInterval, QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."));
  Util::setToolTip(ui->webmMode,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p><p>%6<p>")
                   .arg(QYH("Create a WebM compliant file."))
                   .arg(QYH("mkvmerge also turns this on if the destination file name's extension is \"webm\"."))
                   .arg(QYH("This mode enforces several restrictions."))
                   .arg(QYH("The only allowed codecs are VP8/VP9 video and Vorbis/Opus audio tracks."))
                   .arg(QYH("Tags are allowed, but chapters are not."))
                   .arg(QYH("The DocType header item is changed to \"webm\".")));
  Util::setToolTip(ui->additionalOptions,     QY("Any option given here will be added at the end of the mkvmerge command line."));
  Util::setToolTip(ui->editAdditionalOptions, QY("Any option given here will be added at the end of the mkvmerge command line."));
}

void
Tab::onTitleChanged(QString newValue) {
  m_config.m_title = newValue;
}

void
Tab::setDestination(QString const &newValue) {
  QString prefix;
#if defined(SYS_WINDOWS)
  if (newValue.startsWith(Q(":")))
    prefix = Q(":");
#endif

  m_config.m_destination = QDir::toNativeSeparators(prefix + Util::removeInvalidPathCharacters(newValue));
  if (!m_config.m_destination.isEmpty()) {
    auto &settings           = Util::Settings::get();
    settings.m_lastOutputDir = QFileInfo{ newValue }.absoluteDir();
  }

  emit titleChanged();

  if (m_config.m_destination == newValue)
    return;

  auto newPosition = std::max(ui->output->cursorPosition(), 1) - 1;
  ui->output->setText(m_config.m_destination);
  ui->output->setCursorPosition(newPosition);
}

void
Tab::clearDestination() {
  ui->output->setText(Q(""));
  setDestination(Q(""));
  m_config.m_destinationAuto.clear();
}

void
Tab::clearDestinationMaybe() {
  if (Util::Settings::get().m_autoClearOutputFileName)
    clearDestination();
}

void
Tab::clearTitle() {
  ui->title->setText(Q(""));
  m_config.m_title.clear();
}

void
Tab::clearTitleMaybe() {
  if (Util::Settings::get().m_autoClearFileTitle)
    clearTitle();
}

void
Tab::onBrowseOutput() {
  auto filter   = m_config.m_webmMode ? QY("WebM files") + Q(" (*.webm)") : QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d)");
  auto fileName = getSaveFileName(QY("Select destination file name"), filter, ui->output);
  if (fileName.isEmpty())
    return;

  setDestination(fileName);

  auto &settings           = Util::Settings::get();
  settings.m_lastOutputDir = QFileInfo{ fileName }.absoluteDir();
  settings.save();
}

void
Tab::onGlobalTagsChanged(QString newValue) {
  m_config.m_globalTags = newValue;
}

void
Tab::onBrowseGlobalTags() {
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), ui->globalTags);
  if (!fileName.isEmpty())
    m_config.m_globalTags = fileName;
}

void
Tab::onSegmentInfoChanged(QString newValue) {
  m_config.m_segmentInfo = newValue;
}

void
Tab::onBrowseSegmentInfo() {
  auto fileName = getOpenFileName(QY("Select segment info file"), QY("XML segment info files") + Q(" (*.xml)"), ui->segmentInfo);
  if (!fileName.isEmpty())
    m_config.m_segmentInfo = fileName;
}

void
Tab::onSplitModeChanged(int newMode) {
  auto splitMode       = static_cast<MuxConfig::SplitMode>(newMode);
  m_config.m_splitMode = splitMode;

  if (MuxConfig::DoNotSplit == splitMode) {
    Util::enableWidgets(m_splitControls, false);
    return;
  }

  Util::enableWidgets(m_splitControls, true);

  auto tooltip = QStringList{};
  auto entries = QStringList{};
  auto label   = QString{};

  if (MuxConfig::SplitAfterSize == splitMode) {
    label    = QY("Size:");
    tooltip << QY("The size after which a new destination file is started.")
            << QY("The letters 'G', 'M' and 'K' can be used to indicate giga/mega/kilo bytes respectively.")
            << QY("All units are based on 1024 (G = 1024^3, M = 1024^2, K = 1024).");
    entries << Q("")
            << Q("350M")
            << Q("650M")
            << Q("700M")
            << Q("703M")
            << Q("800M")
            << Q("1000M")
            << Q("4483M")
            << Q("8142M");

  } else if (MuxConfig::SplitAfterDuration == splitMode) {
    label    = QY("Duration:");
    tooltip << QY("The duration after which a new destination file is started.")
            << (Q("%1 %2 %3")
                .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                .arg(QY("You may omit the number of hours 'HH' and the number of nanoseconds 'nnnnnnnnn'."))
                .arg(QY("If given then you may use up to nine digits after the decimal point.")))
            << QY("Examples: 01:00:00 (after one hour) or 1800s (after 1800 seconds).");

    entries << Q("")
            << Q("01:00:00")
            << Q("1800s");

  } else if (MuxConfig::SplitAfterTimestamps == splitMode) {
    label    = QY("Timestamps:");
    tooltip << (Q("%1 %2")
                .arg(QY("The timestamps after which a new destination file is started."))
                .arg(QY("The timestamps refer to the whole stream and not to each individual destination file.")))
            << (Q("%1 %2 %3")
                .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                .arg(QY("You may omit the number of hours 'HH'."))
                .arg(QY("You can specify up to nine digits for the number of nanoseconds 'nnnnnnnnn' or none at all.")))
            << (Q("%1 %2")
                .arg(QY("If two or more timestamps are used then you have to separate them with commas."))
                .arg(QY("The formats can be mixed, too.")))
            << QY("Examples: 01:00:00,01:30:00 (after one hour and after one hour and thirty minutes) or 180s,300s,00:10:00 (after three, five and ten minutes).");

  } else if (MuxConfig::SplitByParts == splitMode) {
    label    = QY("Parts:");
    tooltip << QY("A comma-separated list of timestamp ranges of content to keep.")
            << (Q("%1 %2")
                .arg(QY("Each range consists of a start and end timestamp with a '-' in the middle, e.g. '00:01:15-00:03:20'."))
                .arg(QY("If a start timestamp is left out then the previous range's end timestamp is used, or the start of the file if there was no previous range.")))
            << QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'.")
            << QY("If a range's start timestamp is prefixed with '+' then its content will be written to the same file as the previous range. Otherwise a new file will be created for this range.");

  } else if (MuxConfig::SplitByPartsFrames == splitMode) {
    label    = QY("Parts:");
    tooltip << (Q("%1 %2 %3")
                .arg(QY("A comma-separated list of frame/field number ranges of content to keep."))
                .arg(QY("Each range consists of a start and end frame/field number with a '-' in the middle, e.g. '157-238'."))
                .arg(QY("The numbering starts at 1.")))
            << (Q("%1 %2")
                .arg(QY("This mode considers only the first video track that is output."))
                .arg(QY("If no video track is output no splitting will occur.")))
            << (Q("%1 %2 %3")
                .arg(QY("The numbers given with this argument are interpreted based on the number of Matroska blocks that are output."))
                .arg(QY("A single Matroska block contains either a full frame (for progressive content) or a single field (for interlaced content)."))
                .arg(QY("mkvmerge does not distinguish between those two and simply counts the number of blocks.")))
            << (Q("%1 %2")
                .arg(QY("If a start number is left out then the previous range's end number is used, or the start of the file if there was no previous range."))
                .arg(QY("If a range's start number is prefixed with '+' then its content will be written to the same file as the previous range. Otherwise a new file will be created for this range.")));

  } else if (MuxConfig::SplitByFrames == splitMode) {
    label    = QY("Frames/fields:");
    tooltip << (Q("%1 %2")
                .arg(QY("A comma-separated list of frame/field numbers after which to split."))
                .arg(QY("The numbering starts at 1.")))
            << (Q("%1 %2")
                .arg(QY("This mode considers only the first video track that is output."))
                .arg(QY("If no video track is output no splitting will occur.")))
            << (Q("%1 %2 %3")
                .arg(QY("The numbers given with this argument are interpreted based on the number of Matroska blocks that are output."))
                .arg(QY("A single Matroska block contains either a full frame (for progressive content) or a single field (for interlaced content)."))
                .arg(QY("mkvmerge does not distinguish between those two and simply counts the number of blocks.")));

  } else if (MuxConfig::SplitAfterChapters == splitMode) {
    label    = QY("Chapter numbers:");
    tooltip << (Q("%1 %2")
                .arg(QY("Either the word 'all' which selects all chapters or a comma-separated list of chapter numbers before which to split."))
                .arg(QY("The numbering starts at 1.")))
            << QY("Splitting will occur right before the first key frame whose timestamp is equal to or bigger than the start timestamp for the chapters whose numbers are listed.")
            << (Q("%1 %2")
                .arg(QY("A chapter starting at 0s is never considered for splitting and discarded silently."))
                .arg(QY("This mode only considers the top-most level of chapters across all edition entries.")));

  }

  auto options = ui->splitOptions->currentText();

  ui->splitOptionsLabel->setText(label);
  ui->splitOptions->clear();
  ui->splitOptions->addItems(entries);
  ui->splitOptions->setCurrentText(options);

  for (auto &oneTooltip : tooltip)
    oneTooltip = oneTooltip.toHtmlEscaped();

  Util::setToolTip(ui->splitOptions, Q("<p>%1</p>").arg(tooltip.join(Q("</p><p>"))));
}

void
Tab::onSplitOptionsChanged(QString newValue) {
  m_config.m_splitOptions = newValue;
}

void
Tab::onLinkFilesClicked(bool newValue) {
  m_config.m_linkFiles = newValue;
}

void
Tab::onSplitMaxFilesChanged(int newValue) {
  m_config.m_splitMaxFiles = newValue;
}

void
Tab::onSegmentUIDsChanged(QString newValue) {
  m_config.m_segmentUIDs = newValue;
}

void
Tab::onPreviousSegmentUIDChanged(QString newValue) {
  m_config.m_previousSegmentUID = newValue;
}

void
Tab::onNextSegmentUIDChanged(QString newValue) {
  m_config.m_nextSegmentUID = newValue;
}

void
Tab::onChaptersChanged(QString newValue) {
  m_config.m_chapters = newValue;
  ui->chapterCharacterSetPreview->setEnabled(!newValue.isEmpty());
}

void
Tab::onBrowseChapters() {
  auto fileName = getOpenFileName(QY("Select chapter file"),
                                  QY("Supported file types")           + Q(" (*.txt *.xml);;") +
                                  QY("XML chapter files")              + Q(" (*.xml);;") +
                                  QY("Simple OGM-style chapter files") + Q(" (*.txt)"),
                                  ui->chapters,
                                  InitialDirMode::ContentFirstInputFileLastOpenDir);

  if (!fileName.isEmpty())
    onChaptersChanged(fileName);
}

void
Tab::onChapterLanguageChanged(int newValue) {
  auto data = ui->chapterLanguage->itemData(newValue);
  if (data.isValid())
    m_config.m_chapterLanguage = data.toString();
}

void
Tab::onChapterCharacterSetChanged(QString newValue) {
  m_config.m_chapterCharacterSet = newValue;
}

void
Tab::onChapterCueNameFormatChanged(QString newValue) {
  m_config.m_chapterCueNameFormat = newValue;
}

void
Tab::onWebmClicked(bool newValue) {
  m_config.m_webmMode = newValue;
  setOutputFileNameMaybe();
}

void
Tab::onAdditionalOptionsChanged(QString newValue) {
  m_config.m_additionalOptions = newValue;
}

void
Tab::onEditAdditionalOptions() {
  AdditionalCommandLineOptionsDialog dlg{this, m_config.m_additionalOptions};
  if (!dlg.exec())
    return;

  m_config.m_additionalOptions = dlg.additionalOptions();
  ui->additionalOptions->setText(m_config.m_additionalOptions);

  if (dlg.saveAsDefault()) {
    auto &settings = Util::Settings::get();
    settings.m_defaultAdditionalMergeOptions = m_config.m_additionalOptions;
    settings.save();
  }
}

void
Tab::setOutputControlValues() {
  m_config.m_destination = Util::removeInvalidPathCharacters(m_config.m_destination);

  ui->title->setText(m_config.m_title);
  ui->output->setText(m_config.m_destination);
  ui->globalTags->setText(m_config.m_globalTags);
  ui->segmentInfo->setText(m_config.m_segmentInfo);
  ui->splitMode->setCurrentIndex(m_config.m_splitMode);
  ui->splitOptions->setEditText(m_config.m_splitOptions);
  ui->splitMaxFiles->setValue(m_config.m_splitMaxFiles);
  ui->linkFiles->setChecked(m_config.m_linkFiles);
  ui->segmentUIDs->setText(m_config.m_segmentUIDs);
  ui->previousSegmentUID->setText(m_config.m_previousSegmentUID);
  ui->nextSegmentUID->setText(m_config.m_nextSegmentUID);
  ui->chapters->setText(m_config.m_chapters);
  ui->chapterCueNameFormat->setText(m_config.m_chapterCueNameFormat);
  ui->additionalOptions->setText(m_config.m_additionalOptions);
  ui->webmMode->setChecked(m_config.m_webmMode);

  ui->chapterLanguage->setAdditionalItems(m_config.m_chapterLanguage)
    .reInitializeIfNecessary()
    .setCurrentByData(m_config.m_chapterLanguage);
  ui->chapterCharacterSet->setAdditionalItems(m_config.m_chapterCharacterSet)
    .reInitializeIfNecessary()
    .setCurrentByData(m_config.m_chapterCharacterSet);
  ui->chapterGenerationMode->setCurrentIndex(static_cast<int>(m_config.m_chapterGenerationMode));
  ui->chapterGenerationNameTemplate->setText(m_config.m_chapterGenerationNameTemplate);
  ui->chapterGenerationInterval->setText(m_config.m_chapterGenerationInterval);
}

void
Tab::onBrowseSegmentUID() {
  addSegmentUIDFromFile(*ui->segmentUIDs, true);
}

void
Tab::onBrowsePreviousSegmentUID() {
  addSegmentUIDFromFile(*ui->previousSegmentUID, false);
}

void
Tab::onBrowseNextSegmentUID() {
  addSegmentUIDFromFile(*ui->nextSegmentUID, false);
}

void
Tab::addSegmentUIDFromFile(QLineEdit &lineEdit,
                           bool append) {
  Util::addSegmentUIDFromFileToLineEdit(*this, lineEdit, append);
}

void
Tab::onPreviewChapterCharacterSet() {
  if (m_config.m_chapters.isEmpty())
    return;

  auto dlg = new SelectCharacterSetDialog{this, m_config.m_chapters, ui->chapterCharacterSet->currentData().toString()};
  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::setChapterCharacterSet);

  dlg->show();
}

void
Tab::setChapterCharacterSet(QString const &characterSet) {
  Util::setComboBoxTextByData(ui->chapterCharacterSet, characterSet);
  onChapterCharacterSetChanged(characterSet);
}

void
Tab::onCopyFirstFileNameToTitle() {
  if (hasSourceFiles())
    ui->title->setText(QFileInfo{ m_config.m_files[0]->m_fileName }.completeBaseName());
}

void
Tab::onCopyOutputFileNameToTitle() {
  if (hasDestinationFileName())
    ui->title->setText(QFileInfo{ m_config.m_destination }.completeBaseName());
}

void
Tab::onCopyTitleToOutputFileName() {
  if (!hasTitle())
    return;

  auto info        = QFileInfo{ m_config.m_destination };
  auto suffix      = info.completeSuffix();
  auto path        = info.path();
  auto newFileName = m_config.m_title;

  if (!path.isEmpty())
    newFileName = Q("%1/%2").arg(path).arg(newFileName);

  if (suffix.isEmpty())
    suffix = "mkv";

  ui->output->setText(QDir::toNativeSeparators(Q("%1.%2").arg(newFileName).arg(suffix)));
}

bool
Tab::hasTitle()
  const {
  return !m_config.m_title.isEmpty();
}

bool
Tab::hasDestinationFileName()
  const {
  return !m_config.m_destination.isEmpty();
}

void
Tab::onChapterGenerationModeChanged() {
  m_config.m_chapterGenerationMode = static_cast<MuxConfig::ChapterGenerationMode>(ui->chapterGenerationMode->currentIndex());
  auto isEnabled                   = MuxConfig::ChapterGenerationMode::None      != m_config.m_chapterGenerationMode;
  auto isInterval                  = MuxConfig::ChapterGenerationMode::Intervals == m_config.m_chapterGenerationMode;

  ui->chapterGenerationNameTemplate->setEnabled(isEnabled);
  ui->chapterGenerationNameTemplateLabel->setEnabled(isEnabled);
  ui->chapterGenerationInterval->setEnabled(isInterval);
  ui->chapterGenerationIntervalLabel->setEnabled(isInterval);
}

void
Tab::onChapterGenerationNameTemplateChanged() {
  m_config.m_chapterGenerationNameTemplate = ui->chapterGenerationNameTemplate->text();
}

void
Tab::onChapterGenerationIntervalChanged() {
  m_config.m_chapterGenerationInterval = ui->chapterGenerationInterval->text();
}

}}}
