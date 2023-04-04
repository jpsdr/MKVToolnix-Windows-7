#include "common/common_pch.h"

#include <QFileInfo>
#include <QMenu>
#include <QStringList>

#include "common/bcp47.h"
#include "common/bitvalue.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tab_p.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/language_display_widget.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

void
Tab::setupOutputControls() {
  auto &p   = *p_func();
  auto &cfg = Util::Settings::get();

  cfg.handleSplitterSizes(p.ui->mergeOutputSplitter);

  for (auto idx = 0; idx < 8; ++idx)
    p.ui->splitMode->addItem(QString{}, idx);

  setupOutputFileControls();

  p.ui->chapterLanguage->enableClearingLanguage(true);
  p.ui->chapterCharacterSetPreview->setEnabled(false);

  p.splitControls << p.ui->splitOptions << p.ui->splitOptionsLabel << p.ui->splitMaxFilesLabel << p.ui->splitMaxFiles << p.ui->linkFiles;

  auto comboBoxControls = QList<QComboBox *>{} << p.ui->splitMode << p.ui->chapterCharacterSet << p.ui->chapterGenerationMode;
  for (auto const &control : comboBoxControls) {
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    Util::fixComboBoxViewWidth(*control);
  }

  p.ui->splitOptions->lineEdit()->setClearButtonEnabled(true);
  p.ui->splitMaxFiles->setMaximum(std::numeric_limits<int>::max());

  onSplitModeChanged(MuxConfig::DoNotSplit);
  onChaptersChanged({});
  onChapterGenerationModeChanged();

#if !defined(HAVE_DVDREAD)
  p.ui->chapterTitleNumberLabel->setVisible(false);
  p.ui->chapterTitleNumber     ->setVisible(false);
#endif  // !defined(HAVE_DVDREAD)

  connect(MainWindow::get(),                 &MainWindow::preferencesChanged,                                                                  this, &Tab::setupOutputFileControls);

  connect(p.ui->additionalOptions,             &QLineEdit::textChanged,                                                                          this, &Tab::onAdditionalOptionsChanged);
  connect(p.ui->browseChapters,                &QPushButton::clicked,                                                                            this, &Tab::onBrowseChapters);
  connect(p.ui->browseGlobalTags,              &QPushButton::clicked,                                                                            this, &Tab::onBrowseGlobalTags);
  connect(p.ui->browseNextSegmentUID,          &QPushButton::clicked,                                                                            this, &Tab::onBrowseNextSegmentUID);
  connect(p.ui->browseOutput,                  &QPushButton::clicked,                                                                            this, &Tab::onBrowseOutput);
  connect(p.ui->browsePreviousSegmentUID,      &QPushButton::clicked,                                                                            this, &Tab::onBrowsePreviousSegmentUID);
  connect(p.ui->browseSegmentInfo,             &QPushButton::clicked,                                                                            this, &Tab::onBrowseSegmentInfo);
  connect(p.ui->browseSegmentUID,              &QPushButton::clicked,                                                                            this, &Tab::onBrowseSegmentUID);
  connect(p.ui->chapterCharacterSet,           &QComboBox::currentTextChanged,                                                                   this, &Tab::onChapterCharacterSetChanged);
  connect(p.ui->chapterCharacterSetPreview,    &QPushButton::clicked,                                                                            this, &Tab::onPreviewChapterCharacterSet);
  connect(p.ui->chapterDelay,                  &QLineEdit::textChanged,                                                                          this, &Tab::onChapterDelayChanged);
  connect(p.ui->chapterStretchBy,              &QLineEdit::textChanged,                                                                          this, &Tab::onChapterStretchByChanged);
  connect(p.ui->chapterCueNameFormat,          &QLineEdit::textChanged,                                                                          this, &Tab::onChapterCueNameFormatChanged);
  connect(p.ui->chapterLanguage,               &Util::LanguageDisplayWidget::languageChanged,                                                    this, &Tab::onChapterLanguageChanged);
  connect(p.ui->chapters,                      &QLineEdit::textChanged,                                                                          this, &Tab::onChaptersChanged);
  connect(p.ui->globalTags,                    &QLineEdit::textChanged,                                                                          this, &Tab::onGlobalTagsChanged);
  connect(p.ui->linkFiles,                     &QPushButton::clicked,                                                                            this, &Tab::onLinkFilesClicked);
  connect(p.ui->nextSegmentUID,                &QLineEdit::textChanged,                                                                          this, &Tab::onNextSegmentUIDChanged);
  connect(p.ui->output,                        &QLineEdit::textChanged,                                                                          this, &Tab::setDestination);
  connect(p.ui->outputRecentlyUsed,            &QPushButton::clicked,                                                                            this, &Tab::showRecentlyUsedOutputDirs);
  connect(p.ui->previousSegmentUID,            &QLineEdit::textChanged,                                                                          this, &Tab::onPreviousSegmentUIDChanged);
  connect(p.ui->segmentInfo,                   &QLineEdit::textChanged,                                                                          this, &Tab::onSegmentInfoChanged);
  connect(p.ui->segmentUIDs,                   &QLineEdit::textChanged,                                                                          this, &Tab::onSegmentUIDsChanged);
  connect(p.ui->splitMaxFiles,                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),                                    this, &Tab::onSplitMaxFilesChanged);
  connect(p.ui->splitMode,                     static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this, &Tab::onSplitModeChanged);
  connect(p.ui->splitOptions,                  &QComboBox::editTextChanged,                                                                      this, &Tab::onSplitOptionsChanged);
  connect(p.ui->title,                         &QLineEdit::textChanged,                                                                          this, &Tab::onTitleChanged);
  connect(p.ui->webmMode,                      &QPushButton::clicked,                                                                            this, &Tab::onWebmClicked);
  connect(p.ui->stopAfterVideoEnds,            &QPushButton::clicked,                                                                            this, &Tab::onStopAfterVideoEndsClicked);
  connect(p.ui->chapterGenerationMode,         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                           this, &Tab::onChapterGenerationModeChanged);
  connect(p.ui->chapterGenerationNameTemplate, &QLineEdit::textChanged,                                                                          this, &Tab::onChapterGenerationNameTemplateChanged);
  connect(p.ui->chapterGenerationInterval,     &QLineEdit::textChanged,                                                                          this, &Tab::onChapterGenerationIntervalChanged);
  connect(p.ui->chapterTitleNumber,            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),                                    this, &Tab::onChapterTitleNumberChanged);
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
  auto &p = *p_func();

  if (!p.ui->hlOutput)
    return;

  auto widgets = QList<QWidget *>{} << p.ui->outputLabel << p.ui->output << p.ui->browseOutput << p.ui->outputRecentlyUsed;

  for (auto const &widget : widgets) {
    widget->setParent(p.ui->gbOutputFile);
    widget->show();
  }

  delete p.ui->hlOutput;
  p.ui->hlOutput = nullptr;

  delete p.ui->gbOutputFile->layout();
  auto layout = new QHBoxLayout{p.ui->gbOutputFile};
  layout->setSpacing(6);
  layout->setContentsMargins(6, 6, 6, 6);

  for (auto const &widget : widgets)
    layout->addWidget(widget);

  p.ui->gbOutputFile->show();
}

void
Tab::moveOutputFileNameToOutputTab() {
  auto &p = *p_func();

  p.ui->gbOutputFile->hide();

  if (p.ui->hlOutput)
    return;

  p.ui->gbOutputFile->hide();

  auto widgets = QList<QWidget *>{} << p.ui->outputLabel << p.ui->output << p.ui->browseOutput << p.ui->outputRecentlyUsed;

  for (auto const &widget : widgets) {
    widget->setParent(p.ui->generalBox);
    widget->show();
  }

  p.ui->hlOutput = new QHBoxLayout{p.ui->gbOutputFile};
  p.ui->hlOutput->setSpacing(6);
  p.ui->hlOutput->addWidget(p.ui->output);
  p.ui->hlOutput->addWidget(p.ui->browseOutput);
  p.ui->hlOutput->addWidget(p.ui->outputRecentlyUsed);

  p.ui->generalGridLayout->addWidget(p.ui->outputLabel, 1, 0, 1, 1);
  p.ui->generalGridLayout->addLayout(p.ui->hlOutput,    1, 1, 1, 1);
}

void
Tab::retranslateOutputUI() {
  auto &p = *p_func();

  Util::setComboBoxTexts(p.ui->splitMode,
                         QStringList{} << QY("Do not split")                 << QY("After output size")                     << QY("After output duration")     << QY("After specific timestamps")
                                       << QY("By parts based on timestamps") << QY("By parts based on frame/field numbers") << QY("After frame/field numbers") << QY("Before chapters"));

  p.ui->chapterLanguage->retranslateUi();

  setupOutputToolTips();
  setupSplitModeLabelAndToolTips();
}

void
Tab::setupOutputToolTips() {
  auto &p = *p_func();

  Util::setToolTip(p.ui->title, QY("This is the title that players may show as the 'main title' for this movie."));
  Util::setToolTip(p.ui->splitMode,
                   Q("%1 %2")
                   .arg(QY("Enables splitting of the output into more than one file."))
                   .arg(QY("You can split based on the amount of time passed, based on timestamps, on frame/field numbers or on chapter numbers.")));
  Util::setToolTip(p.ui->splitMaxFiles,
                   Q("<p>%1 %2</p><p>%3</p>")
                   .arg(QYH("The maximum number of files that will be created even if the last file might contain more bytes/time than wanted."))
                   .arg(QYH("Useful e.g. when you want exactly two files."))
                   .arg(QYH("If you leave this empty then there is no limit for the number of files mkvmerge might create.")));
  Util::setToolTip(p.ui->linkFiles,
                   Q("%1 %2")
                   .arg(QY("Use 'segment linking' for the resulting files."))
                   .arg(QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation.")));
  Util::setToolTip(p.ui->segmentUIDs,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p>")
                   .arg(QYH("Sets the segment UIDs to use."))
                   .arg(QYH("This is a comma-separated list of 128-bit segment UIDs in the usual UID form: hex numbers with or without the \"0x\" prefix, with or without spaces, exactly 32 digits."))
                   .arg(QYH("Each file created contains one segment, and each segment has one segment UID."))
                   .arg(QYH("If more segment UIDs are specified than segments are created then the surplus UIDs are ignored."))
                   .arg(QYH("If fewer UIDs are specified than segments are created then random UIDs will be created for them.")));
  Util::setToolTip(p.ui->previousSegmentUID, QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(p.ui->nextSegmentUID,     QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(p.ui->chapters,           QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(p.ui->browseChapters,     QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(p.ui->browseSegmentUID,         QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(p.ui->browseNextSegmentUID,     QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(p.ui->browsePreviousSegmentUID, QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
  Util::setToolTip(p.ui->chapterLanguage,          Q("<p>%1 %2 %3</p><p>%4</p>")
                                                   .arg(QYH("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                                                   .arg(QYH("This option specifies the language to be associated with chapters if the OGM chapter format is used."))
                                                   .arg(QYH("It is ignored for XML chapter files."))
                                                   .arg(QYH("The language set here is also used when chapters are generated.")));
  Util::setToolTip(p.ui->chapterCharacterSet,
                   Q("%1 %2 %3")
                   .arg(QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                   .arg(QY("If the OGM format is used and the file's character set is not recognized correctly then this option can be used to correct that."))
                   .arg(QY("It is ignored for XML chapter files.")));
  Util::setToolTip(p.ui->chapterCueNameFormat,
                   Q("<p>%1 %2 %3 %4</p><p>%5</p>")
                   .arg(QYH("mkvmerge can read cue sheets for audio CDs and automatically convert them to chapters."))
                   .arg(QYH("This option controls how the chapter names are created."))
                   .arg(QYH("The sequence '%p' is replaced by the track's PERFORMER, the sequence '%t' by the track's TITLE, '%n' by the track's number and '%N' by the track's number padded with a leading 0 for track numbers < 10."))
                   .arg(QYH("The rest is copied as is."))
                   .arg(QYH("If nothing is entered then '%p - %t' will be used.")));
  Util::setToolTip(p.ui->chapterDelay, QY("Delay the chapters' timestamps by a couple of ms."));
  Util::setToolTip(p.ui->chapterStretchBy,
                   Q("%1 %2")
                   .arg(QYH("Multiply the chapters' timestamps with a factor."))
                   .arg(QYH("The value can be given either as a floating point number (e.g. 12.345) or a fraction of numbers (e.g. 123/456.78).")));
  Util::setToolTip(p.ui->chapterGenerationMode,
                   Q("<p>%1 %2</p><ol><li>%3</li><li>%4</li></ol><p>%5</p>")
                   .arg(QYH("mkvmerge can generate chapters automatically."))
                   .arg(QYH("The following modes are supported:"))
                   .arg(QYH("When appending: one chapter is created at the start and one whenever a file is appended."))
                   .arg(QYH("In fixed intervals: chapters are created in fixed intervals, e.g. every 30 seconds."))
                   .arg(QYH("The language for the newly created chapters is set via the chapter language control above.")));
  Util::setToolTip(p.ui->chapterGenerationNameTemplate, ChapterEditor::Tool::chapterNameTemplateToolTip());
  Util::setToolTip(p.ui->chapterGenerationInterval, QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."));
  Util::setToolTip(p.ui->webmMode,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p><p>%6<p>")
                   .arg(QYH("Create a WebM compliant file."))
                   .arg(QYH("mkvmerge also turns this on if the destination file name's extension is \"webm\"."))
                   .arg(QYH("This mode enforces several restrictions."))
                   .arg(QYH("The only allowed codecs are VP8/VP9 video and Vorbis/Opus audio tracks."))
                   .arg(QYH("Tags are allowed, but chapters are not."))
                   .arg(QYH("The DocType header item is changed to \"webm\".")));
  Util::setToolTip(p.ui->stopAfterVideoEnds,
                   Q("<p>%1 %2</p>")
                   .arg(QYH("Stops processing after the primary video track ends."))
                   .arg(QYH("Any later packets of other tracks will be discarded.")));
  Util::setToolTip(p.ui->additionalOptions,     QY("Any option given here will be added at the end of the mkvmerge command line."));
  Util::setToolTip(p.ui->editAdditionalOptions, QY("Any option given here will be added at the end of the mkvmerge command line."));
}

void
Tab::setupSplitModeLabelAndToolTips() {
  auto &p = *p_func();

  auto tooltip = QStringList{};
  auto entries = QStringList{};
  auto label   = QString{};

  if (MuxConfig::SplitAfterSize == p.config.m_splitMode) {
    label    = QY("Size:");
    tooltip << QY("The size after which a new destination file is started.")
            << QY("The letters 'G', 'M' and 'K' can be used to indicate giga/mega/kilo bytes respectively.")
            << QY("All units are based on 1024 (G = 1024^3, M = 1024^2, K = 1024).");
    entries << Q("");
    entries += Util::Settings::get().m_mergePredefinedSplitSizes;

  } else if (MuxConfig::SplitAfterDuration == p.config.m_splitMode) {
    label    = QY("Duration:");
    tooltip << QY("The duration after which a new destination file is started.")
            << (Q("%1 %2 %3")
                .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                .arg(QY("You may omit the number of hours 'HH' and the number of nanoseconds 'nnnnnnnnn'."))
                .arg(QY("If given then you may use up to nine digits after the decimal point.")))
            << QY("Examples: 01:00:00 (after one hour) or 1800s (after 1800 seconds).");
    entries << Q("");
    entries += Util::Settings::get().m_mergePredefinedSplitDurations;

  } else if (MuxConfig::SplitAfterTimestamps == p.config.m_splitMode) {
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

  } else if (MuxConfig::SplitByParts == p.config.m_splitMode) {
    label    = QY("Parts:");
    tooltip << QY("A comma-separated list of timestamp ranges of content to keep.")
            << (Q("%1 %2")
                .arg(QY("Each range consists of a start and end timestamp with a '-' in the middle, e.g. '00:01:15-00:03:20'."))
                .arg(QY("If a start timestamp is left out then the previous range's end timestamp is used, or the start of the file if there was no previous range.")))
            << QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'.")
            << QY("If a range's start timestamp is prefixed with '+' then its content will be written to the same file as the previous range. Otherwise a new file will be created for this range.");

  } else if (MuxConfig::SplitByPartsFrames == p.config.m_splitMode) {
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

  } else if (MuxConfig::SplitByFrames == p.config.m_splitMode) {
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

  } else if (MuxConfig::SplitAfterChapters == p.config.m_splitMode) {
    label    = QY("Chapter numbers:");
    tooltip << (Q("%1 %2")
                .arg(QY("Either the word 'all' which selects all chapters or a comma-separated list of chapter numbers before which to split."))
                .arg(QY("The numbering starts at 1.")))
            << QY("Splitting will occur right before the first key frame whose timestamp is equal to or bigger than the start timestamp for the chapters whose numbers are listed.")
            << (Q("%1 %2")
                .arg(QY("A chapter starting at 0s is never considered for splitting and discarded silently."))
                .arg(QY("This mode only considers the top-most level of chapters across all edition entries.")));

  } else
    label    = QY("Options:");

  p.ui->splitOptionsLabel->setText(label);

  if (MuxConfig::DoNotSplit == p.config.m_splitMode) {
    p.ui->splitOptions->setToolTip({});
    return;
  }

  auto options = p.ui->splitOptions->currentText();

  p.ui->splitOptions->clear();
  p.ui->splitOptions->addItems(entries);
  p.ui->splitOptions->setCurrentText(options);

  for (auto &oneTooltip : tooltip)
    oneTooltip = oneTooltip.toHtmlEscaped();

  Util::setToolTip(p.ui->splitOptions, Q("<p>%1</p>").arg(tooltip.join(Q("</p><p>"))));
}

void
Tab::onTitleChanged(QString newValue) {
  p_func()->config.m_title = newValue;
}

void
Tab::setDestination(QString const &newValue) {
  auto &p = *p_func();

  if (newValue.isEmpty()) {
    p.config.m_destination.clear();
    Q_EMIT titleChanged();
    return;
  }

#if defined(SYS_WINDOWS)
  if (!newValue.contains(QRegularExpression{Q(R"(^[a-zA-Z]:[\\/]|^(?:\\\\|//).+[\\/].+)")}))
    return;
#endif

  p.config.m_destination = QDir::toNativeSeparators(Util::removeInvalidPathCharacters(newValue));
  if (!p.config.m_destination.isEmpty()) {
    auto &settings           = Util::Settings::get();
    settings.m_lastOutputDir = QFileInfo{ newValue }.absoluteDir();
  }

  Q_EMIT titleChanged();

  if (p.config.m_destination == newValue)
    return;

  auto numRemovedChars = newValue.size()                - std::min<int>(p.config.m_destination.size(), newValue.size());
  auto newPosition     = p.ui->output->cursorPosition() - std::min<int>(numRemovedChars, p.ui->output->cursorPosition());

  p.ui->output->setText(p.config.m_destination);
  p.ui->output->setCursorPosition(newPosition);
}

void
Tab::clearDestination() {
  auto &p = *p_func();

  p.ui->output->setText(Q(""));
  setDestination(Q(""));
  p.config.m_destinationAuto.clear();
  p.config.m_destinationUniquenessSuffix.clear();
}

void
Tab::clearDestinationMaybe() {
  if (Util::Settings::get().m_autoClearOutputFileName)
    clearDestination();
}

void
Tab::clearTitle() {
  auto &p = *p_func();

  p.ui->title->setText(Q(""));
  p.config.m_title.clear();
}

void
Tab::clearTitleMaybe() {
  if (Util::Settings::get().m_autoClearFileTitle)
    clearTitle();
}

void
Tab::onBrowseOutput() {
  auto &p       = *p_func();
  auto filter   = p.config.m_webmMode ? QY("WebM files") + Q(" (*.webm)") : QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d)");
  auto ext      = !p.config.m_destination.isEmpty() ? QFileInfo{p.config.m_destination}.suffix()
                : p.config.m_webmMode               ? Q("webm")
                :                                     Q("mkv");
  auto mkvName  = defaultFileNameForSaving(!ext.isEmpty() ? Q(".%1").arg(ext) : ext);
  auto fileName = getSaveFileName(QY("Select destination file name"), mkvName, filter, p.ui->output, ext);
  if (fileName.isEmpty())
    return;

  setDestination(fileName);

  auto &settings           = Util::Settings::get();
  settings.m_lastOutputDir = QFileInfo{ fileName }.absoluteDir();
  settings.save();
}

void
Tab::onGlobalTagsChanged(QString newValue) {
  p_func()->config.m_globalTags = newValue;
}

void
Tab::onBrowseGlobalTags() {
  auto &p       = *p_func();
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), p.ui->globalTags);
  if (!fileName.isEmpty())
    p.config.m_globalTags = fileName;
}

void
Tab::onSegmentInfoChanged(QString newValue) {
  p_func()->config.m_segmentInfo = newValue;
}

void
Tab::onBrowseSegmentInfo() {
  auto &p       = *p_func();
  auto fileName = getOpenFileName(QY("Select segment info file"), QY("XML segment info files") + Q(" (*.xml)"), p.ui->segmentInfo);
  if (!fileName.isEmpty())
    p.config.m_segmentInfo = fileName;
}

void
Tab::onSplitModeChanged(int newMode) {
  auto &p              = *p_func();
  auto splitMode       = static_cast<MuxConfig::SplitMode>(newMode);
  p.config.m_splitMode = splitMode;

  Util::enableWidgets(p.splitControls, MuxConfig::DoNotSplit != splitMode);
  setupSplitModeLabelAndToolTips();
}

void
Tab::onSplitOptionsChanged(QString newValue) {
  p_func()->config.m_splitOptions = newValue;
}

void
Tab::onLinkFilesClicked(bool newValue) {
  p_func()->config.m_linkFiles = newValue;
}

void
Tab::onSplitMaxFilesChanged(int newValue) {
  p_func()->config.m_splitMaxFiles = newValue;
}

void
Tab::onSegmentUIDsChanged(QString newValue) {
  p_func()->config.m_segmentUIDs = newValue;
}

void
Tab::onPreviousSegmentUIDChanged(QString newValue) {
  p_func()->config.m_previousSegmentUID = newValue;
}

void
Tab::onNextSegmentUIDChanged(QString newValue) {
  p_func()->config.m_nextSegmentUID = newValue;
}

void
Tab::onChaptersChanged(QString newValue) {
  auto &p             = *p_func();
  p.config.m_chapters = newValue;
  auto enablePreview  = !newValue.isEmpty();

#if defined(HAVE_DVDREAD)
  auto isDVD = newValue.toLower().endsWith(".ifo");

  if (enablePreview && isDVD)
    enablePreview = false;

  p.ui->chapterTitleNumberLabel->setEnabled(isDVD);
  p.ui->chapterTitleNumber     ->setEnabled(isDVD);
#endif  // HAVE_DVDREAD

  p.ui->chapterCharacterSetPreview->setEnabled(enablePreview);
}

void
Tab::onBrowseChapters() {
  QString ifo;
  QStringList dvds;

#if defined(HAVE_DVDREAD)
  dvds << Q("%1 (*.ifo *.IFO)").arg(QY("DVDs"));
  ifo = Q("*.ifo *.IFO ");
#endif  // HAVE_DVDREAD

  auto fileTypes = QStringList{} << Q("%1 (%2*.txt *.xml)").arg(QY("Supported file types")).arg(ifo)
                                 << Q("%1 (*.xml)").arg(QY("XML chapter files"))
                                 << Q("%1 (*.txt)").arg(QY("Simple OGM-style chapter files"));

  fileTypes += dvds;

  auto fileName = getOpenFileName(QY("Select chapter file"), fileTypes.join(Q(";;")), p_func()->ui->chapters, InitialDirMode::ContentFirstInputFileLastOpenDir);

  if (!fileName.isEmpty())
    onChaptersChanged(fileName);
}

void
Tab::onChapterTitleNumberChanged(int newValue) {
  p_func()->config.m_chapterTitleNumber = newValue;
}

void
Tab::onChapterLanguageChanged(mtx::bcp47::language_c const &newLanguage) {
  if (newLanguage.is_valid())
    p_func()->config.m_chapterLanguage = newLanguage;
}

void
Tab::onChapterCharacterSetChanged(QString newValue) {
  p_func()->config.m_chapterCharacterSet = newValue;
}

void
Tab::onChapterDelayChanged(QString newValue) {
  p_func()->config.m_chapterDelay = newValue;
}

void
Tab::onChapterStretchByChanged(QString newValue) {
  p_func()->config.m_chapterStretchBy = newValue;
}

void
Tab::onChapterCueNameFormatChanged(QString newValue) {
  p_func()->config.m_chapterCueNameFormat = newValue;
}

void
Tab::onWebmClicked(bool newValue) {
  p_func()->config.m_webmMode = newValue;
  setOutputFileNameMaybe();
}

void
Tab::onStopAfterVideoEndsClicked(bool newValue) {
  p_func()->config.m_stopAfterVideoEnds = newValue;
}

void
Tab::onAdditionalOptionsChanged(QString newValue) {
  p_func()->config.m_additionalOptions = newValue;
}

void
Tab::onEditAdditionalOptions() {
  auto &p = *p_func();

  AdditionalCommandLineOptionsDialog dlg{this, p.config.m_additionalOptions};
  if (!dlg.exec())
    return;

  p.config.m_additionalOptions = dlg.additionalOptions();
  p.ui->additionalOptions->setText(p.config.m_additionalOptions);

  if (dlg.saveAsDefault()) {
    auto &settings = Util::Settings::get();
    settings.m_defaultAdditionalMergeOptions = p.config.m_additionalOptions;
    settings.save();
  }
}

void
Tab::setOutputControlValues() {
  auto &p = *p_func();

  p.config.m_destination = Util::removeInvalidPathCharacters(p.config.m_destination);

  p.ui->title->setText(p.config.m_title);
  p.ui->output->setText(p.config.m_destination);
  p.ui->globalTags->setText(p.config.m_globalTags);
  p.ui->segmentInfo->setText(p.config.m_segmentInfo);
  p.ui->splitMode->setCurrentIndex(p.config.m_splitMode);
  p.ui->splitOptions->setEditText(p.config.m_splitOptions);
  p.ui->splitMaxFiles->setValue(p.config.m_splitMaxFiles);
  p.ui->linkFiles->setChecked(p.config.m_linkFiles);
  p.ui->segmentUIDs->setText(p.config.m_segmentUIDs);
  p.ui->previousSegmentUID->setText(p.config.m_previousSegmentUID);
  p.ui->nextSegmentUID->setText(p.config.m_nextSegmentUID);
  p.ui->chapters->setText(p.config.m_chapters);
  p.ui->chapterTitleNumber->setValue(p.config.m_chapterTitleNumber);
  p.ui->chapterDelay->setText(p.config.m_chapterDelay);
  p.ui->chapterStretchBy->setText(p.config.m_chapterStretchBy);
  p.ui->chapterCueNameFormat->setText(p.config.m_chapterCueNameFormat);
  p.ui->additionalOptions->setText(p.config.m_additionalOptions);
  p.ui->webmMode->setChecked(p.config.m_webmMode);
  p.ui->stopAfterVideoEnds->setChecked(p.config.m_stopAfterVideoEnds);

  p.ui->chapterLanguage->setLanguage(p.config.m_chapterLanguage);
  p.ui->chapterCharacterSet->setAdditionalItems(p.config.m_chapterCharacterSet)
    .reInitializeIfNecessary()
    .setCurrentByData(p.config.m_chapterCharacterSet);
  p.ui->chapterGenerationMode->setCurrentIndex(static_cast<int>(p.config.m_chapterGenerationMode));
  p.ui->chapterGenerationNameTemplate->setText(p.config.m_chapterGenerationNameTemplate);
  p.ui->chapterGenerationInterval->setText(p.config.m_chapterGenerationInterval);
}

void
Tab::onBrowseSegmentUID() {
  addSegmentUIDFromFile(*p_func()->ui->segmentUIDs, true);
}

void
Tab::onBrowsePreviousSegmentUID() {
  addSegmentUIDFromFile(*p_func()->ui->previousSegmentUID, false);
}

void
Tab::onBrowseNextSegmentUID() {
  addSegmentUIDFromFile(*p_func()->ui->nextSegmentUID, false);
}

void
Tab::addSegmentUIDFromFile(QLineEdit &lineEdit,
                           bool append) {
  Util::addSegmentUIDFromFileToLineEdit(*this, lineEdit, append);
}

void
Tab::onPreviewChapterCharacterSet() {
  auto &p = *p_func();

  if (p.config.m_chapters.isEmpty())
    return;

  auto dlg = new SelectCharacterSetDialog{this, p.config.m_chapters, p.ui->chapterCharacterSet->currentData().toString()};
  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::setChapterCharacterSet);

  dlg->show();
}

void
Tab::setChapterCharacterSet(QString const &characterSet) {
  Util::setComboBoxTextByData(p_func()->ui->chapterCharacterSet, characterSet);
  onChapterCharacterSetChanged(characterSet);
}

void
Tab::setChaptersFileName(QString const &fileName) {
  auto &p             = *p_func();
  p.config.m_chapters = fileName;
  p.ui->chapters->setText(fileName);
}

void
Tab::setTagsFileName(QString const &fileName) {
  auto &p               = *p_func();
  p.config.m_globalTags = fileName;
  p.ui->globalTags->setText(fileName);
}

void
Tab::setSegmentInfoFileName(QString const &fileName) {
  auto &p                = *p_func();
  p.config.m_segmentInfo = fileName;
  p.ui->segmentInfo->setText(fileName);
}

void
Tab::onCopyFirstFileNameToTitle() {
  auto &p = *p_func();

  if (hasSourceFiles())
    p.ui->title->setText(QFileInfo{ p.config.m_files[0]->m_fileName }.completeBaseName());
}

void
Tab::onCopyOutputFileNameToTitle() {
  auto &p = *p_func();

  if (hasDestinationFileName())
    p.ui->title->setText(QFileInfo{ p.config.m_destination }.completeBaseName());
}

void
Tab::onCopyTitleToOutputFileName() {
  auto &p = *p_func();

  if (!hasTitle())
    return;

  p.config.m_destinationUniquenessSuffix.clear();

  auto info  = QFileInfo{ p.config.m_destination };
  auto path  = info.path();
  auto title = Util::replaceInvalidFileNameCharacters(p.config.m_title);

  QString newFileName;

  if (Util::Settings::get().m_uniqueOutputFileNames)
    newFileName = generateUniqueOutputFileName(title, QDir{path});

  else {
    auto suffix = info.suffix();
    newFileName = Q("%1.%2")
      .arg(path.isEmpty()   ? title    : Q("%1/%2").arg(path).arg(title))
      .arg(suffix.isEmpty() ? Q("mkv") : suffix);
  }

  p.ui->output->setText(QDir::toNativeSeparators(newFileName));
}

bool
Tab::hasTitle()
  const {
  return !p_func()->config.m_title.isEmpty();
}

bool
Tab::hasDestinationFileName()
  const {
  return !p_func()->config.m_destination.isEmpty();
}

void
Tab::onChapterGenerationModeChanged() {
  auto &p = *p_func();

  p.config.m_chapterGenerationMode = static_cast<MuxConfig::ChapterGenerationMode>(p.ui->chapterGenerationMode->currentIndex());
  auto isInterval                  = MuxConfig::ChapterGenerationMode::Intervals == p.config.m_chapterGenerationMode;

  p.ui->chapterGenerationInterval->setEnabled(isInterval);
  p.ui->chapterGenerationIntervalLabel->setEnabled(isInterval);
}

void
Tab::onChapterGenerationNameTemplateChanged() {
  auto &p = *p_func();

  p.config.m_chapterGenerationNameTemplate = p.ui->chapterGenerationNameTemplate->text();
}

void
Tab::onChapterGenerationIntervalChanged() {
  auto &p = *p_func();

  p.config.m_chapterGenerationInterval = p.ui->chapterGenerationInterval->text();
}

void
Tab::showRecentlyUsedOutputDirs() {
  auto &reg   = Util::Settings::get();
  auto &items = reg.m_mergeLastOutputDirs;
  auto path   = QFileInfo{ p_func()->config.m_destination }.path();

  if (!path.isEmpty())
    items.add(QDir::toNativeSeparators(path));

  if (items.isEmpty())
    return;

  QMenu menu{this};

  for (auto const &dir : Util::Settings::get().m_mergeLastOutputDirs.items()) {
    auto action = new QAction{&menu};
    action->setText(dir);

    connect(action, &QAction::triggered, [this, dir]() { changeOutputDirectoryTo(dir); });

    menu.addAction(action);
  }

  menu.exec(QCursor::pos());

}

void
Tab::changeOutputDirectoryTo(QString const &directory) {
  auto &p = *p_func();

  auto makeUnique  = Util::Settings::get().m_uniqueOutputFileNames;
  auto oldFileName = QFileInfo{ p.config.m_destination }.fileName();
  auto newFileName = !oldFileName.isEmpty() ? oldFileName : Q("%1.%2").arg(QY("unnamed")).arg(suggestOutputFileNameExtension());
  auto newFilePath = makeUnique ? generateUniqueOutputFileName(QFileInfo{newFileName}.completeBaseName(), QDir{directory}, true) : Q("%1/%2").arg(directory).arg(newFileName);

  p.ui->output->setText(QDir::toNativeSeparators(newFilePath));
}

}
