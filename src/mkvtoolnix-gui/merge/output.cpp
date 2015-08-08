#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/additional_command_line_options_dialog.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

void
Tab::setupOutputControls() {
  auto &cfg = Util::Settings::get();

  cfg.handleSplitterSizes(ui->mergeOutputSplitter);

  setupOutputFileControls();

  m_splitControls << ui->splitOptions << ui->splitOptionsLabel << ui->splitMaxFilesLabel << ui->splitMaxFiles << ui->linkFiles;

  auto comboBoxControls = QList<QComboBox *>{} << ui->splitMode << ui->chapterLanguage << ui->chapterCharacterSet;
  for (auto const &control : comboBoxControls)
    control->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  onSplitModeChanged(MuxConfig::DoNotSplit);

  connect(MainWindow::get(), &MainWindow::preferencesChanged, this, &Tab::setupOutputFileControls);
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
                   .arg(QY("The maximum number of files that will be created even if the last file might contain more bytes/time than wanted."))
                   .arg(QY("Useful e.g. when you want exactly two files."))
                   .arg(QY("If you leave this empty then there is no limit for the number of files mkvmerge might create.")));
  Util::setToolTip(ui->linkFiles,
                   Q("%1 %2")
                   .arg(QY("Use 'segment linking' for the resulting files."))
                   .arg(QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation.")));
  Util::setToolTip(ui->segmentUIDs,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p>")
                   .arg(QY("Sets the segment UIDs to use."))
                   .arg(QY("This is a comma-separated list of 128bit segment UIDs in the usual UID form: hex numbers with or without the \"0x\" prefix, with or without spaces, exactly 32 digits."))
                   .arg(QY("Each file created contains one segment, and each segment has one segment UID."))
                   .arg(QY("If more segment UIDs are specified than segments are created then the surplus UIDs are ignored."))
                   .arg(QY("If fewer UIDs are specified than segments are created then random UIDs will be created for them.")));
  Util::setToolTip(ui->previousSegmentUID, QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(ui->nextSegmentUID,     QY("For an in-depth explanantion of file/segment linking and this feature please read mkvmerge's documentation."));
  Util::setToolTip(ui->chapters,           QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(ui->browseChapters,     QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."));
  Util::setToolTip(ui->chapterLanguage,
                   Q("%1 %2 %3")
                   .arg(QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                   .arg(QY("This option specifies the language to be associated with chapters if the OGM chapter format is used."))
                   .arg(QY("It is ignored for XML chapter files.")));
  Util::setToolTip(ui->chapterCharacterSet,
                   Q("%1 %2 %3")
                   .arg(QY("mkvmerge supports two chapter formats: The OGM like text format and the full featured XML format."))
                   .arg(QY("If the OGM format is used and the file's character set is not recognized correctly then this option can be used to correct that."))
                   .arg(QY("It is ignored for XML chapter files.")));
  Util::setToolTip(ui->chapterCueNameFormat,
                   Q("<p>%1 %2 %3 %4</p><p>%5</p>")
                   .arg(QY("mkvmerge can read CUE sheets for audio CDs and automatically convert them to chapters."))
                   .arg(QY("This option controls how the chapter names are created."))
                   .arg(QY("The sequence '%p' is replaced by the track's PERFORMER, the sequence '%t' by the track's TITLE, '%n' by the track's number and '%N' by the track's number padded with a leading 0 for track numbers < 10."))
                   .arg(QY("The rest is copied as is."))
                   .arg(QY("If nothing is entered then '%p - %t' will be used.")));
  Util::setToolTip(ui->webmMode,
                   Q("<p>%1 %2</p><p>%3 %4 %5</p><p>%6<p>")
                   .arg(QY("Create a WebM compliant file."))
                   .arg(QY("mkvmerge also turns this on if the output file name's extension is \"webm\"."))
                   .arg(QY("This mode enforces several restrictions."))
                   .arg(QY("The only allowed codecs are VP8/VP9 video and Vorbis/Opus audio tracks."))
                   .arg(QY("Tags are allowed, but chapters are not."))
                   .arg(QY("The DocType header item is changed to \"webm\".")));
  Util::setToolTip(ui->additionalOptions,     QY("Any option given here will be added at the end of the mkvmerge command line."));
  Util::setToolTip(ui->editAdditionalOptions, QY("Any option given here will be added at the end of the mkvmerge command line."));
}

void
Tab::onTitleEdited(QString newValue) {
  m_config.m_title = newValue;
}

void
Tab::setDestination(QString const &newValue) {
  m_config.m_destination = newValue;
  emit titleChanged();
}

void
Tab::clearDestination() {
  ui->output->setText(Q(""));
  setDestination(Q(""));
  m_config.m_destinationAuto.clear();
}

void
Tab::clearDestinationMaybe() {
  if (Util::Settings::DontSetOutputFileName != Util::Settings::get().m_outputFileNamePolicy)
    clearDestination();
}

void
Tab::clearTitle() {
  ui->title->setText(Q(""));
  m_config.m_title.clear();
}

void
Tab::clearTitleMaybe() {
  if (Util::Settings::get().m_autoSetFileTitle)
    clearTitle();
}

void
Tab::onBrowseOutput() {
  auto filter   = m_config.m_webmMode ? QY("WebM files") + Q(" (*.webm)") : QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d)");
  auto fileName = getSaveFileName(QY("Select output file name"), filter, ui->output);
  if (fileName.isEmpty())
    return;

  setDestination(fileName);

  auto &settings           = Util::Settings::get();
  settings.m_lastOutputDir = QFileInfo{ fileName }.absoluteDir();
  settings.save();
}

void
Tab::onGlobalTagsEdited(QString newValue) {
  m_config.m_globalTags = newValue;
}

void
Tab::onBrowseGlobalTags() {
  auto fileName = getOpenFileName(QY("Select tags file"), QY("XML tag files") + Q(" (*.xml)"), ui->globalTags);
  if (!fileName.isEmpty())
    m_config.m_globalTags = fileName;
}

void
Tab::onSegmentInfoEdited(QString newValue) {
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
    tooltip << QY("The size after which a new output file is started.")
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
    tooltip << QY("The duration after which a new output file is started.")
            << (Q("%1 %2 %3")
                .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                .arg(QY("You may omit the number of hours 'HH' and the number of nanoseconds 'nnnnnnnnn'."))
                .arg(QY("If given then you may use up to nine digits after the decimal point.")))
            << QY("Examples: 01:00:00 (after one hour) or 1800s (after 1800 seconds).");

    entries << Q("")
            << Q("01:00:00")
            << Q("1800s");

  } else if (MuxConfig::SplitAfterTimecodes == splitMode) {
    label    = QY("Timecodes:");
    tooltip << (Q("%1 %2")
                .arg(QY("The timecodes after which a new output file is started."))
                .arg(QY("The timecodes refer to the whole stream and not to each individual output file.")))
            << (Q("%1 %2 %3")
                .arg(QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'."))
                .arg(QY("You may omit the number of hours 'HH'."))
                .arg(QY("You can specify up to nine digits for the number of nanoseconds 'nnnnnnnnn' or none at all.")))
            << (Q("%1 %2")
                .arg(QY("If two or more timecodes are used then you have to separate them with commas."))
                .arg(QY("The formats can be mixed, too.")))
            << QY("Examples: 01:00:00,01:30:00 (after one hour and after one hour and thirty minutes) or 180s,300s,00:10:00 (after three, five and ten minutes).");

  } else if (MuxConfig::SplitByParts == splitMode) {
    label    = QY("Parts:");
    tooltip << QY("A comma-separated list of timecode ranges of content to keep.")
            << (Q("%1 %2")
                .arg(QY("Each range consists of a start and end timecode with a '-' in the middle, e.g. '00:01:15-00:03:20'."))
                .arg(QY("If a start timecode is left out then the previous range's end timecode is used, or the start of the file if there was no previous range.")))
            << QY("The format is either the form 'HH:MM:SS.nnnnnnnnn' or a number followed by one of the units 's', 'ms' or 'us'.")
            << QY("If a range's start timecode is prefixed with '+' then its content will be written to the same file as the previous range. Otherwise a new file will be created for this range.");

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
            << QY("Splitting will occur right before the first key frame whose timecode is equal to or bigger than the start timecode for the chapters whose numbers are listed.")
            << (Q("%1 %2")
                .arg(QY("A chapter starting at 0s is never considered for splitting and discarded silently."))
                .arg(QY("This mode only considers the top-most level of chapters across all edition entries.")));

  }

  auto options = ui->splitOptions->currentText();

  ui->splitOptionsLabel->setText(label);
  ui->splitOptions->clear();
  ui->splitOptions->addItems(entries);
  ui->splitOptions->setCurrentText(options);

  Util::setToolTip(ui->splitOptions, Q("<p>%1</p>").arg(tooltip.join(Q("</p><p>"))));
}

void
Tab::onSplitOptionsEdited(QString newValue) {
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
Tab::onSegmentUIDsEdited(QString newValue) {
  m_config.m_segmentUIDs = newValue;
}

void
Tab::onPreviousSegmentUIDEdited(QString newValue) {
  m_config.m_previousSegmentUID = newValue;
}

void
Tab::onNextSegmentUIDEdited(QString newValue) {
  m_config.m_nextSegmentUID = newValue;
}

void
Tab::onChaptersEdited(QString newValue) {
  m_config.m_chapters = newValue;
}

void
Tab::onBrowseChapters() {
  auto fileName = getOpenFileName(QY("Select chapter file"),
                                  QY("Supported file types")           + Q(" (*.txt *.xml);;") +
                                  QY("XML chapter files")              + Q(" (*.xml);;") +
                                  QY("Simple OGM-style chapter files") + Q(" (*.txt)"),
                                  ui->chapters);

  if (!fileName.isEmpty())
    m_config.m_chapters = fileName;
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
  // TODO: change output file extension
}

void
Tab::onAdditionalOptionsEdited(QString newValue) {
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

  ui->chapterLanguage->setCurrentByData(m_config.m_chapterLanguage);
  ui->chapterCharacterSet->setCurrentByData(m_config.m_chapterCharacterSet);
}

}}}
