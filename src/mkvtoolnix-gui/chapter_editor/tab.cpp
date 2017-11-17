#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

#include <matroska/KaxSemantic.h>

#include "common/bitvalue.h"
#include "common/chapters/chapters.h"
#include "common/construct.h"
#include "common/ebml.h"
#include "common/kax_file.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mpls.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/xml/ebml_chapters_converter.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/generate_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tab_p.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/select_character_set_dialog.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

TabPrivate::TabPrivate(Tab &tab,
                       QString const &pFileName)
  : ui{new Ui::Tab}
  , fileName{pFileName}
  , chapterModel{new ChapterModel{&tab}}
  , nameModel{new NameModel{&tab}}
  , expandAllAction{new QAction{&tab}}
  , collapseAllAction{new QAction{&tab}}
  , addEditionBeforeAction{new QAction{&tab}}
  , addEditionAfterAction{new QAction{&tab}}
  , addChapterBeforeAction{new QAction{&tab}}
  , addChapterAfterAction{new QAction{&tab}}
  , addSubChapterAction{new QAction{&tab}}
  , removeElementAction{new QAction{&tab}}
  , duplicateAction{new QAction{&tab}}
  , massModificationAction{new QAction{&tab}}
  , generateSubChaptersAction{new QAction{&tab}}
  , renumberSubChaptersAction{new QAction{&tab}}
{
}

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , d_ptr{new TabPrivate{*this, fileName}}
{
  setup();
}

Tab::Tab(QWidget *parent,
         TabPrivate &d)
  : QWidget{parent}
  , d_ptr{&d}
{
  setup();
}

Tab::~Tab() {
}

void
Tab::setup() {
  Q_D(Tab);

  // Setup UI controls.
  d->ui->setupUi(this);

  setupUi();

  retranslateUi();
}

void
Tab::setupUi() {
  Q_D(Tab);

  Util::Settings::get().handleSplitterSizes(d->ui->chapterEditorSplitter);

  d->ui->elements->setModel(d->chapterModel);
  d->ui->tvChNames->setModel(d->nameModel);

  d->ui->cbChNameCountry->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  d->ui->cbChNameLanguage->setup();
  d->ui->cbChNameCountry->setup(true);

  d->nameWidgets << d->ui->pbChRemoveName
                 << d->ui->lChName         << d->ui->leChName
                 << d->ui->lChNameLanguage << d->ui->cbChNameLanguage
                 << d->ui->lChNameCountry  << d->ui->cbChNameCountry;

  Util::fixScrollAreaBackground(d->ui->scrollArea);
  Util::fixComboBoxViewWidth(*d->ui->cbChNameLanguage);
  Util::fixComboBoxViewWidth(*d->ui->cbChNameCountry);
  Util::HeaderViewManager::create(*d->ui->elements,  "ChapterEditor::Elements");
  Util::HeaderViewManager::create(*d->ui->tvChNames, "ChapterEditor::ChapterNames");

  d->addEditionBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  d->addEditionAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  d->addChapterBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  d->addChapterAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  d->addSubChapterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-under.png")});
  d->generateSubChaptersAction->setIcon(QIcon{Q(":/icons/16x16/.png")});
  d->duplicateAction->setIcon(QIcon{Q(":/icons/16x16/tab-duplicate.png")});
  d->removeElementAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  d->renumberSubChaptersAction->setIcon(QIcon{Q(":/icons/16x16/format-list-ordered.png")});
  d->massModificationAction->setIcon(QIcon{Q(":/icons/16x16/tools-wizard.png")});

  auto mw = MainWindow::get();
  connect(d->ui->elements,                    &Util::BasicTreeView::customContextMenuRequested,                       this,                    &Tab::showChapterContextMenu);
  connect(d->ui->elements,                    &Util::BasicTreeView::deletePressed,                                    this,                    &Tab::removeElement);
  connect(d->ui->elements,                    &Util::BasicTreeView::insertPressed,                                    this,                    &Tab::addEditionOrChapterAfter);
  connect(d->ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this,                    &Tab::chapterSelectionChanged);
  connect(d->ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged,                                 this,                    &Tab::nameSelectionChanged);
  connect(d->ui->leChName,                    &QLineEdit::textEdited,                                                 this,                    &Tab::chapterNameEdited);
  connect(d->ui->cbChNameLanguage,            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                    &Tab::chapterNameLanguageChanged);
  connect(d->ui->cbChNameCountry,             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,                    &Tab::chapterNameCountryChanged);
  connect(d->ui->pbChAddName,                 &QPushButton::clicked,                                                  this,                    &Tab::addChapterName);
  connect(d->ui->pbChRemoveName,              &QPushButton::clicked,                                                  this,                    &Tab::removeChapterName);
  connect(d->ui->pbBrowseSegmentUID,          &QPushButton::clicked,                                                  this,                    &Tab::addSegmentUIDFromFile);

  connect(d->expandAllAction,                 &QAction::triggered,                                                    this,                    &Tab::expandAll);
  connect(d->collapseAllAction,               &QAction::triggered,                                                    this,                    &Tab::collapseAll);
  connect(d->addEditionBeforeAction,          &QAction::triggered,                                                    this,                    &Tab::addEditionBefore);
  connect(d->addEditionAfterAction,           &QAction::triggered,                                                    this,                    &Tab::addEditionAfter);
  connect(d->addChapterBeforeAction,          &QAction::triggered,                                                    this,                    &Tab::addChapterBefore);
  connect(d->addChapterAfterAction,           &QAction::triggered,                                                    this,                    &Tab::addChapterAfter);
  connect(d->addSubChapterAction,             &QAction::triggered,                                                    this,                    &Tab::addSubChapter);
  connect(d->removeElementAction,             &QAction::triggered,                                                    this,                    &Tab::removeElement);
  connect(d->duplicateAction,                 &QAction::triggered,                                                    this,                    &Tab::duplicateElement);
  connect(d->massModificationAction,          &QAction::triggered,                                                    this,                    &Tab::massModify);
  connect(d->generateSubChaptersAction,       &QAction::triggered,                                                    this,                    &Tab::generateSubChapters);
  connect(d->renumberSubChaptersAction,       &QAction::triggered,                                                    this,                    &Tab::renumberSubChapters);

  connect(mw,                                 &MainWindow::preferencesChanged,                                        d->ui->cbChNameLanguage, &Util::ComboBoxBase::reInitialize);
  connect(mw,                                 &MainWindow::preferencesChanged,                                        d->ui->cbChNameCountry,  &Util::ComboBoxBase::reInitialize);

  for (auto &lineEdit : findChildren<Util::BasicLineEdit *>()) {
    lineEdit->acceptDroppedFiles(false).setTextToDroppedFileName(false);
    connect(lineEdit, &Util::BasicLineEdit::returnPressed,      this, &Tab::focusOtherControlInNextChapterElement);
    connect(lineEdit, &Util::BasicLineEdit::shiftReturnPressed, this, &Tab::focusSameControlInNextChapterElement);
  }
}

void
Tab::updateFileNameDisplay() {
  Q_D(Tab);

  if (!d->fileName.isEmpty()) {
    auto info = QFileInfo{d->fileName};
    d->ui->fileName->setText(info.fileName());
    d->ui->directory->setText(QDir::toNativeSeparators(info.path()));

  } else {
    d->ui->fileName->setText(QY("<Unsaved file>"));
    d->ui->directory->setText(Q(""));

  }
}

void
Tab::retranslateUi() {
  Q_D(Tab);

  d->ui->retranslateUi(this);

  updateFileNameDisplay();

  d->expandAllAction->setText(QY("&Expand all"));
  d->collapseAllAction->setText(QY("&Collapse all"));
  d->addEditionBeforeAction->setText(QY("Add new e&dition before"));
  d->addEditionAfterAction->setText(QY("Add new ed&ition after"));
  d->addChapterBeforeAction->setText(QY("Add new c&hapter before"));
  d->addChapterAfterAction->setText(QY("Add new ch&apter after"));
  d->addSubChapterAction->setText(QY("Add new &sub-chapter inside"));
  d->removeElementAction->setText(QY("&Remove selected edition or chapter"));
  d->duplicateAction->setText(QY("D&uplicate selected edition or chapter"));
  d->massModificationAction->setText(QY("Additional &modifications"));
  d->generateSubChaptersAction->setText(QY("&Generate sub-chapters"));
  d->renumberSubChaptersAction->setText(QY("Re&number sub-chapters"));

  setupToolTips();

  d->chapterModel->retranslateUi();
  d->nameModel->retranslateUi();

  resizeChapterColumnsToContents();

  emit titleChanged();
}

void
Tab::setupToolTips() {
  Q_D(Tab);

  Util::setToolTip(d->ui->elements, QY("Right-click for actions for editions and chapters"));
  Util::setToolTip(d->ui->pbBrowseSegmentUID, QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
}

void
Tab::resizeChapterColumnsToContents()
  const {
  Q_D(const Tab);

  Util::resizeViewColumnsToContents(d->ui->elements);
}

void
Tab::resizeNameColumnsToContents()
  const {
  Q_D(const Tab);

  Util::resizeViewColumnsToContents(d->ui->tvChNames);
}

QString
Tab::title()
  const {
  Q_D(const Tab);

  if (d->fileName.isEmpty())
    return QY("<Unsaved file>");
  return QFileInfo{d->fileName}.fileName();
}

QString const &
Tab::fileName()
  const {
  Q_D(const Tab);

  return d->fileName;
}

void
Tab::newFile() {
  Q_D(Tab);

  addEdition(false);

  auto selectionModel = d->ui->elements->selectionModel();
  auto selection      = QItemSelection{d->chapterModel->index(0, 0), d->chapterModel->index(0, d->chapterModel->columnCount() - 1)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);

  addSubChapter();

  auto parentIdx = d->chapterModel->index(0, 0);
  selection      = QItemSelection{d->chapterModel->index(0, 0, parentIdx), d->chapterModel->index(0, d->chapterModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

  resizeChapterColumnsToContents();

  d->ui->leChStart->selectAll();
  d->ui->leChStart->setFocus();

  d->savedState = currentState();
}

void
Tab::resetData() {
  Q_D(Tab);

  d->analyzer.reset();
  d->nameModel->reset();
  d->chapterModel->reset();
}

bool
Tab::readFileEndTimestampForMatroska() {
  Q_D(Tab);

  d->fileEndTimestamp.reset();

  auto idx = d->analyzer->find(KaxInfo::ClassInfos.GlobalId);
  if (-1 == idx) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(d->fileName)).exec();
    return false;
  }

  auto info = d->analyzer->read_element(idx);
  if (!info) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(d->fileName)).exec();
    return false;
  }

  auto durationKax = FindChild<KaxDuration>(*info);
  if (!durationKax) {
    qDebug() << "readFileEndTimestampForMatroska: no duration found";
    return true;
  }

  auto timestampScale = FindChildValue<KaxTimecodeScale, uint64_t>(static_cast<KaxInfo &>(*info), TIMESTAMP_SCALE);
  auto duration       = timestamp_c::ns(durationKax->GetValue() * timestampScale);

  qDebug() << "readFileEndTimestampForMatroska: duration is" << Q(format_timestamp(duration));

  auto &fileIo = d->analyzer->get_file();
  fileIo.setFilePointer(d->analyzer->get_segment_data_start_pos());

  kax_file_c fileKax{fileIo};
  fileKax.enable_reporting(false);

  auto cluster = std::shared_ptr<KaxCluster>(fileKax.read_next_cluster());
  if (!cluster) {
    qDebug() << "readFileEndTimestampForMatroska: no cluster found";
    return true;
  }

  cluster->InitTimecode(FindChildValue<KaxClusterTimecode>(*cluster), timestampScale);

  auto minBlockTimestamp = timestamp_c::ns(0);

  for (auto const &child : *cluster) {
    timestamp_c blockTimestamp;

    if (Is<KaxBlockGroup>(child)) {
      auto &group = static_cast<KaxBlockGroup &>(*child);
      auto block  = FindChild<KaxBlock>(group);

      if (block) {
        block->SetParent(*cluster);
        blockTimestamp = timestamp_c::ns(mtx::math::to_signed(block->GlobalTimecode()));
      }

    } else if (Is<KaxSimpleBlock>(child)) {
      auto &block = static_cast<KaxSimpleBlock &>(*child);
      block.SetParent(*cluster);
      blockTimestamp = timestamp_c::ns(mtx::math::to_signed(block.GlobalTimecode()));

    }

    if (   blockTimestamp.valid()
        && (   !minBlockTimestamp.valid()
            || (blockTimestamp < minBlockTimestamp)))
      minBlockTimestamp = blockTimestamp;
  }

  d->fileEndTimestamp = minBlockTimestamp + duration;

  qDebug() << "readFileEndTimestampForMatroska: minBlockTimestamp" << Q(format_timestamp(minBlockTimestamp)) << "result" << Q(format_timestamp(d->fileEndTimestamp));

  return true;
}

Tab::LoadResult
Tab::loadFromMatroskaFile() {
  Q_D(Tab);

  d->analyzer = std::make_unique<QtKaxAnalyzer>(this, d->fileName);

  if (!d->analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).set_open_mode(MODE_READ).process()) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(d->fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
    emit removeThisTab();
    return {};
  }

  auto idx = d->analyzer->find(KaxChapters::ClassInfos.GlobalId);
  if (-1 == idx) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) does not contain any chapters.").arg(d->fileName)).exec();
    emit removeThisTab();
    return {};
  }

  auto chapters = d->analyzer->read_element(idx);
  if (!chapters) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(d->fileName)).exec();
    emit removeThisTab();
    return {};
  }

  if (!readFileEndTimestampForMatroska()) {
    emit removeThisTab();
    return {};
  }

  d->analyzer->close_file();

  return { std::static_pointer_cast<KaxChapters>(chapters), true };
}

Tab::LoadResult
Tab::checkSimpleFormatForBomAndNonAscii(ChaptersPtr const &chapters) {
  Q_D(Tab);

  auto result = Util::checkForBomAndNonAscii(d->fileName);

  if (   (BO_NONE != result.byteOrder)
      || !result.containsNonAscii
      || !Util::Settings::get().m_ceTextFileCharacterSet.isEmpty())
    return { chapters, false };

  Util::enableChildren(this, false);

  d->originalFileName = d->fileName;
  auto dlg            = new SelectCharacterSetDialog{this, d->originalFileName};

  connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::reloadSimpleChaptersWithCharacterSet);
  connect(dlg, &SelectCharacterSetDialog::rejected,             this, &Tab::closeTab);

  dlg->show();

  return {};
}

Tab::LoadResult
Tab::loadFromChapterFile() {
  Q_D(Tab);

  auto format   = mtx::chapters::format_e::xml;
  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    chapters = mtx::chapters::parse(to_utf8(d->fileName), 0, -1, 0, "", to_utf8(Util::Settings::get().m_ceTextFileCharacterSet), true, &format);

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::chapters::parser_x &ex) {
    error = Q(ex.what());
  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(d->fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    emit removeThisTab();

  } else if (format != mtx::chapters::format_e::xml) {
    auto result = checkSimpleFormatForBomAndNonAscii(chapters);

    d->fileName.clear();
    emit titleChanged();

    return result;
  }

  return { chapters, format == mtx::chapters::format_e::xml };
}

void
Tab::reloadSimpleChaptersWithCharacterSet(QString const &characterSet) {
  Q_D(Tab);

  auto error = QString{};

  try {
    auto chapters = mtx::chapters::parse(to_utf8(d->originalFileName), 0, -1, 0, "", to_utf8(characterSet), true);
    chaptersLoaded(chapters, false);

    Util::enableChildren(this, true);

    MainWindow::chapterEditorTool()->enableMenuActions();

    return;

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());
  } catch (mtx::chapters::parser_x &ex) {
    error = Q(ex.what());
  }

  Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("Error message from the parser: %1").arg(error)).exec();

  emit removeThisTab();
}

bool
Tab::areWidgetsEnabled()
  const {
  Q_D(const Tab);

  return d->ui->elements->isEnabled();
}

ChaptersPtr
Tab::timestampsToChapters(std::vector<timestamp_c> const &timestamps)
  const {
  auto &cfg     = Util::Settings::get();
  auto chapters = ChaptersPtr{ static_cast<KaxChapters *>(mtx::construct::cons<KaxChapters>(mtx::construct::cons<KaxEditionEntry>())) };
  auto &edition = GetChild<KaxEditionEntry>(*chapters);
  auto idx      = 0;

  for (auto const &timestamp : timestamps) {
    auto nameTemplate = QString{ cfg.m_chapterNameTemplate };
    auto name         = formatChapterName(nameTemplate, ++idx, timestamp);
    auto atom         = mtx::construct::cons<KaxChapterAtom>(new KaxChapterTimeStart, timestamp.to_ns(),
                                                             mtx::construct::cons<KaxChapterDisplay>(new KaxChapterString,   name,
                                                                                                     new KaxChapterLanguage, to_utf8(cfg.m_defaultChapterLanguage)));
    if (!cfg.m_defaultChapterCountry.isEmpty())
      GetChild<KaxChapterCountry>(GetChild<KaxChapterDisplay>(atom)).SetValue(to_utf8(cfg.m_defaultChapterCountry));

    edition.PushElement(*atom);
  }

  return chapters;
}

Tab::LoadResult
Tab::loadFromMplsFile() {
  Q_D(Tab);

  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    auto in     = mm_file_io_c{to_utf8(d->fileName)};
    auto parser = ::mtx::bluray::mpls::parser_c{};

    parser.enable_dropping_last_entry_if_at_end(Util::Settings::get().m_dropLastChapterFromBlurayPlaylist);

    if (parser.parse(&in))
      chapters = timestampsToChapters(parser.get_chapters());

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::bluray::mpls::exception &ex) {
    error = Q(ex.what());

  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(d->fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    emit removeThisTab();

  } else {
    d->fileName.clear();
    emit titleChanged();
  }

  return { chapters, false };
}

void
Tab::load() {
  Q_D(Tab);

  resetData();

  d->savedState = currentState();
  auto result   = kax_analyzer_c::probe(to_utf8(d->fileName)) ? loadFromMatroskaFile()
                : d->fileName.toLower().endsWith(Q(".mpls"))  ? loadFromMplsFile()
                :                                               loadFromChapterFile();

  if (result.first)
    chaptersLoaded(result.first, result.second);
}

void
Tab::chaptersLoaded(ChaptersPtr const &chapters,
                    bool canBeWritten) {
  Q_D(Tab);

  mtx::chapters::fix_country_codes(*chapters);

  if (!d->fileName.isEmpty())
    d->fileModificationTime = QFileInfo{d->fileName}.lastModified();

  disconnect(d->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  d->chapterModel->reset();
  d->chapterModel->populate(*chapters);

  if (canBeWritten)
    d->savedState = currentState();

  expandAll();
  resizeChapterColumnsToContents();

  connect(d->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  MainWindow::chapterEditorTool()->enableMenuActions();
}

void
Tab::save() {
  Q_D(Tab);

  if (!d->analyzer)
    saveAsXmlImpl(false);

  else
    saveToMatroskaImpl(false);
}

void
Tab::saveAsImpl(bool requireNewFileName,
                std::function<bool(bool, QString &)> const &worker) {
  Q_D(Tab);

  if (!copyControlsToStorage())
    return;

  d->chapterModel->fixMandatoryElements();
  setControlsFromStorage();

  auto newFileName = d->fileName;
  if (d->fileName.isEmpty())
    requireNewFileName = true;

  if (!worker(requireNewFileName, newFileName))
    return;

  d->savedState = currentState();

  if (newFileName != d->fileName) {
    d->fileName            = newFileName;

    auto &settings         = Util::Settings::get();
    settings.m_lastOpenDir = QFileInfo{newFileName}.path();
    settings.save();

    updateFileNameDisplay();
    emit titleChanged();
  }

  MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
}

void
Tab::saveAsXml() {
  saveAsXmlImpl(true);
}

void
Tab::saveAsXmlImpl(bool requireNewFileName) {
  Q_D(Tab);

  saveAsImpl(requireNewFileName, [this, d](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (doRequireNewFileName) {
      auto defaultFilePath = !d->fileName.isEmpty() ? Util::dirPath(QFileInfo{d->fileName}.path()) : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getSaveFileName(this, QY("Save chapters as XML"), defaultFilePath, QY("XML chapter files") + Q(" (*.xml);;") + QY("All files") + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    try {
      auto chapters = d->chapterModel->allChapters();
      auto out      = mm_file_io_c{to_utf8(newFileName), MODE_CREATE};
      mtx::xml::ebml_chapters_converter_c::write_xml(*chapters, out);

    } catch (mtx::mm_io::exception &) {
      Util::MessageBox::critical(this)->title(QY("Saving failed")).text(QY("Creating the file failed. Check to make sure you have permission to write to that directory and that the drive is not full.")).exec();
      return false;

    } catch (mtx::xml::conversion_x &ex) {
      Util::MessageBox::critical(this)->title(QY("Saving failed")).text(QY("Converting the chapters to XML failed: %1").arg(ex.what())).exec();
      return false;
    }

    return true;
  });
}

void
Tab::saveToMatroska() {
  saveToMatroskaImpl(true);
}

void
Tab::saveToMatroskaImpl(bool requireNewFileName) {
  Q_D(Tab);

  saveAsImpl(requireNewFileName, [this, d](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (!d->analyzer)
      doRequireNewFileName = true;

    if (doRequireNewFileName) {
      auto defaultFilePath = !d->fileName.isEmpty() ? QFileInfo{d->fileName}.path() : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getOpenFileName(this, QY("Save chapters to Matroska or WebM file"), defaultFilePath,
                                                   QY("Supported file types") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") +
                                                   QY("Matroska files")       + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
                                                   QY("WebM files")           + Q(" (*.webm);;") +
                                                   QY("All files")            + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    if (doRequireNewFileName || (QFileInfo{newFileName}.lastModified() != d->fileModificationTime)) {
      d->analyzer = std::make_unique<QtKaxAnalyzer>(this, newFileName);
      if (!d->analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).process()) {
        auto text = Q("%1 %2")
          .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(newFileName))
          .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
        Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
        return false;
      }

      d->fileName = newFileName;
    }

    auto chapters = d->chapterModel->allChapters();
    auto result   = kax_analyzer_c::uer_success;

    if (chapters && (0 != chapters->ListSize())) {
      fix_mandatory_elements(chapters.get());
      if (d->analyzer->is_webm())
        mtx::chapters::remove_elements_unsupported_by_webm(*chapters);

      result = d->analyzer->update_element(chapters, !d->analyzer->is_webm(), false);

    } else
      result = d->analyzer->remove_elements(EBML_ID(KaxChapters));

    d->analyzer->close_file();

    if (kax_analyzer_c::uer_success != result) {
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the chapters failed."));
      return false;
    }

    d->fileModificationTime = QFileInfo{d->fileName}.lastModified();

    return true;
  });
}

void
Tab::selectChapterRow(QModelIndex const &idx,
                      bool ignoreSelectionChanges) {
  Q_D(Tab);

  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), d->chapterModel->columnCount() - 1)};

  d->ignoreChapterSelectionChanges = ignoreSelectionChanges;
  d->ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  d->ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  d->ignoreChapterSelectionChanges = false;
}

bool
Tab::copyControlsToStorage() {
  Q_D(Tab);

  auto idx = Util::selectedRowIdx(d->ui->elements);
  return idx.isValid() ? copyControlsToStorage(idx) : true;
}

bool
Tab::copyControlsToStorage(QModelIndex const &idx) {
  Q_D(Tab);

  auto result = copyControlsToStorageImpl(idx);
  if (result.first) {
    d->chapterModel->updateRow(idx);
    return true;
  }

  selectChapterRow(idx, true);

  Util::MessageBox::critical(this)->title(QY("Validation failed")).text(result.second).exec();

  return false;
}

Tab::ValidationResult
Tab::copyControlsToStorageImpl(QModelIndex const &idx) {
  Q_D(Tab);

  auto stdItem = d->chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return { true, QString{} };

  if (!idx.parent().isValid())
    return copyEditionControlsToStorage(d->chapterModel->editionFromItem(stdItem));
  return copyChapterControlsToStorage(d->chapterModel->chapterFromItem(stdItem));
}

Tab::ValidationResult
Tab::copyChapterControlsToStorage(ChapterPtr const &chapter) {
  Q_D(Tab);

  if (!chapter)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!d->ui->leChUid->text().isEmpty()) {
    auto ok = false;
    uid  = d->ui->leChUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The chapter UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxChapterUID>(*chapter).SetValue(uid);
  else
    DeleteChildren<KaxChapterUID>(*chapter);

  if (!d->ui->cbChFlagEnabled->isChecked())
    GetChild<KaxChapterFlagEnabled>(*chapter).SetValue(0);
  else
    DeleteChildren<KaxChapterFlagEnabled>(*chapter);

  if (d->ui->cbChFlagHidden->isChecked())
    GetChild<KaxChapterFlagHidden>(*chapter).SetValue(1);
  else
    DeleteChildren<KaxChapterFlagHidden>(*chapter);

  auto startTimestamp = int64_t{};
  if (!parse_timestamp(to_utf8(d->ui->leChStart->text()), startTimestamp))
    return { false, QY("The start time could not be parsed: %1").arg(Q(timestamp_parser_error)) };
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTimestamp);

  if (!d->ui->leChEnd->text().isEmpty()) {
    auto endTimestamp = int64_t{};
    if (!parse_timestamp(to_utf8(d->ui->leChEnd->text()), endTimestamp))
      return { false, QY("The end time could not be parsed: %1").arg(Q(timestamp_parser_error)) };

    if (endTimestamp <= startTimestamp)
      return { false, QY("The end time must be greater than the start time.") };

    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(endTimestamp);

  } else
    DeleteChildren<KaxChapterTimeEnd>(*chapter);

  if (!d->ui->leChSegmentUid->text().isEmpty()) {
    try {
      auto value = mtx::bits::value_c{to_utf8(d->ui->leChSegmentUid->text())};
      GetChild<KaxChapterSegmentUID>(*chapter).CopyBuffer(value.data(), value.byte_size());

    } catch (mtx::bits::value_parser_x const &ex) {
      return { false, QY("The segment UID could not be parsed: %1").arg(ex.what()) };
    }

  } else
    DeleteChildren<KaxChapterSegmentUID>(*chapter);

  if (!d->ui->leChSegmentEditionUid->text().isEmpty()) {
    auto ok = false;
    uid     = d->ui->leChSegmentEditionUid->text().toULongLong(&ok);
    if (!ok || !uid)
      return { false, QY("The segment edition UID must be a positive number if given.") };

    GetChild<KaxChapterSegmentEditionUID>(*chapter).SetValue(uid);
  }

  RemoveChildren<KaxChapterDisplay>(*chapter);
  for (auto row = 0, numRows = d->nameModel->rowCount(); row < numRows; ++row)
    chapter->PushElement(*d->nameModel->displayFromIndex(d->nameModel->index(row, 0)));

  return { true, QString{} };
}

Tab::ValidationResult
Tab::copyEditionControlsToStorage(EditionPtr const &edition) {
  Q_D(Tab);

  if (!edition)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!d->ui->leEdUid->text().isEmpty()) {
    auto ok = false;
    uid     = d->ui->leEdUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The edition UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxEditionUID>(*edition).SetValue(uid);
  else
    DeleteChildren<KaxEditionUID>(*edition);

  if (d->ui->cbEdFlagDefault->isChecked())
    GetChild<KaxEditionFlagDefault>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagDefault>(*edition);

  if (d->ui->cbEdFlagHidden->isChecked())
    GetChild<KaxEditionFlagHidden>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagHidden>(*edition);

  if (d->ui->cbEdFlagOrdered->isChecked())
    GetChild<KaxEditionFlagOrdered>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagOrdered>(*edition);

  return { true, QString{} };
}

bool
Tab::setControlsFromStorage() {
  Q_D(Tab);

  auto idx = Util::selectedRowIdx(d->ui->elements);
  return idx.isValid() ? setControlsFromStorage(idx) : true;
}

bool
Tab::setControlsFromStorage(QModelIndex const &idx) {
  Q_D(Tab);

  auto stdItem = d->chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return false;

  if (!idx.parent().isValid())
    return setEditionControlsFromStorage(d->chapterModel->editionFromItem(stdItem));
  return setChapterControlsFromStorage(d->chapterModel->chapterFromItem(stdItem));
}

bool
Tab::setChapterControlsFromStorage(ChapterPtr const &chapter) {
  Q_D(Tab);

  if (!chapter)
    return true;

  auto uid               = FindChildValue<KaxChapterUID>(*chapter);
  auto end               = FindChild<KaxChapterTimeEnd>(*chapter);
  auto segmentEditionUid = FindChild<KaxChapterSegmentEditionUID>(*chapter);

  d->ui->lChapter->setText(d->chapterModel->chapterDisplayName(*chapter));
  d->ui->leChStart->setText(Q(format_timestamp(FindChildValue<KaxChapterTimeStart>(*chapter))));
  d->ui->leChEnd->setText(end ? Q(format_timestamp(end->GetValue())) : Q(""));
  d->ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter, 1));
  d->ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  d->ui->leChUid->setText(uid ? QString::number(uid) : Q(""));
  d->ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  d->ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  auto nameSelectionModel        = d->ui->tvChNames->selectionModel();
  auto previouslySelectedNameIdx = nameSelectionModel->currentIndex();
  d->nameModel->populate(*chapter);
  enableNameWidgets(false);

  if (d->nameModel->rowCount()) {
    auto oldBlocked  = nameSelectionModel->blockSignals(true);
    auto rowToSelect = std::min(previouslySelectedNameIdx.isValid() ? previouslySelectedNameIdx.row() : 0, d->nameModel->rowCount());
    auto selection   = QItemSelection{ d->nameModel->index(rowToSelect, 0), d->nameModel->index(rowToSelect, d->nameModel->columnCount() - 1) };

    nameSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

    setNameControlsFromStorage(d->nameModel->index(rowToSelect, 0));
    enableNameWidgets(true);

    nameSelectionModel->blockSignals(oldBlocked);
  }

  d->ui->pageContainer->setCurrentWidget(d->ui->chapterPage);

  return true;
}

bool
Tab::setEditionControlsFromStorage(EditionPtr const &edition) {
  Q_D(Tab);

  if (!edition)
    return true;

  auto uid = FindChildValue<KaxEditionUID>(*edition);

  d->ui->leEdUid->setText(uid ? QString::number(uid) : Q(""));
  d->ui->cbEdFlagDefault->setChecked(!!FindChildValue<KaxEditionFlagDefault>(*edition));
  d->ui->cbEdFlagHidden->setChecked(!!FindChildValue<KaxEditionFlagHidden>(*edition));
  d->ui->cbEdFlagOrdered->setChecked(!!FindChildValue<KaxEditionFlagOrdered>(*edition));

  d->ui->pageContainer->setCurrentWidget(d->ui->editionPage);

  return true;
}

bool
Tab::handleChapterDeselection(QItemSelection const &deselected) {
  if (deselected.isEmpty())
    return true;

  auto indexes = deselected.at(0).indexes();
  return indexes.isEmpty() ? true : copyControlsToStorage(indexes.at(0));
}

void
Tab::chapterSelectionChanged(QItemSelection const &selected,
                             QItemSelection const &deselected) {
  Q_D(Tab);

  auto selectedIdx = QModelIndex{};
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty())
      selectedIdx = indexes.at(0);
  }

  d->chapterModel->setSelectedIdx(selectedIdx);

  if (d->ignoreChapterSelectionChanges)
    return;

  if (!handleChapterDeselection(deselected))
    return;

  if (selectedIdx.isValid() && setControlsFromStorage(selectedIdx.sibling(selectedIdx.row(), 0)))
    return;

  d->ui->pageContainer->setCurrentWidget(d->ui->emptyPage);
}

bool
Tab::setNameControlsFromStorage(QModelIndex const &idx) {
  Q_D(Tab);

  auto display = d->nameModel->displayFromIndex(idx);
  if (!display)
    return false;

  auto language = Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"}));

  d->ui->leChName->setText(Q(GetChildValue<KaxChapterString>(display)));
  d->ui->cbChNameLanguage->setAdditionalItems(usedNameLanguages())
    .reInitializeIfNecessary()
    .setCurrentByData(language);
  d->ui->cbChNameCountry->setAdditionalItems(usedNameCountryCodes())
    .reInitializeIfNecessary()
    .setCurrentByData(Q(FindChildValue<KaxChapterCountry>(display)));

  resizeNameColumnsToContents();

  return true;
}

void
Tab::nameSelectionChanged(QItemSelection const &selected,
                          QItemSelection const &) {
  Q_D(Tab);

  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setNameControlsFromStorage(indexes.at(0))) {
      enableNameWidgets(true);

      d->ui->leChName->selectAll();
      QTimer::singleShot(0, d->ui->leChName, SLOT(setFocus()));

      return;
    }
  }

  enableNameWidgets(false);
}

void
Tab::withSelectedName(std::function<void(QModelIndex const &, KaxChapterDisplay &)> const &worker) {
  Q_D(Tab);

  auto selectedRows = d->ui->tvChNames->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return;

  auto idx     = selectedRows.at(0);
  auto display = d->nameModel->displayFromIndex(idx);
  if (display)
    worker(idx, *display);
}

void
Tab::chapterNameEdited(QString const &text) {
  Q_D(Tab);

  withSelectedName([d, &text](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterString>(display).SetValueUTF8(to_utf8(text));
    d->nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameLanguageChanged(int index) {
  Q_D(Tab);

  if (0 > index)
    return;

  withSelectedName([d, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(d->ui->cbChNameLanguage->itemData(index).toString()));
    d->nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameCountryChanged(int index) {
  Q_D(Tab);

  if (0 > index)
    return;

  withSelectedName([d, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    if (0 == index)
      DeleteChildren<KaxChapterCountry>(display);
    else
      GetChild<KaxChapterCountry>(display).SetValue(to_utf8(d->ui->cbChNameCountry->currentData().toString()));
    d->nameModel->updateRow(idx.row());
  });
}

void
Tab::addChapterName() {
  Q_D(Tab);

  d->nameModel->addNew();
}

void
Tab::removeChapterName() {
  Q_D(Tab);

  auto idx = Util::selectedRowIdx(d->ui->tvChNames);
  if (idx.isValid())
    d->nameModel->remove(idx);
}

void
Tab::enableNameWidgets(bool enable) {
  Q_D(Tab);

  for (auto const &widget : d->nameWidgets)
    widget->setEnabled(enable);
}

void
Tab::expandAll() {
  expandCollapseAll(true);
}

void
Tab::collapseAll() {
  expandCollapseAll(false);
}

void
Tab::addEditionBefore() {
  addEdition(true);
}

void
Tab::addEditionAfter() {
  addEdition(false);
}

QModelIndex
Tab::addEdition(bool before) {
  Q_D(Tab);

  auto edition     = std::make_shared<KaxEditionEntry>();
  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  auto row         = 0;

  if (selectedIdx.isValid()) {
    while (selectedIdx.parent().isValid())
      selectedIdx = selectedIdx.parent();

    row = selectedIdx.row() + (before ? 0 : 1);
  }

  GetChild<KaxEditionUID>(*edition).SetValue(0);

  d->chapterModel->insertEdition(row, edition);

  emit numberOfEntriesChanged();

  return d->chapterModel->index(row, 0);
}

void
Tab::addEditionOrChapterAfter() {
  Q_D(Tab);

  auto selectedIdx     = Util::selectedRowIdx(d->ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();

  if (!hasSelection)
    return;

  auto newEntryIdx = chapterSelected ? addChapter(false) : addEdition(false);
  selectChapterRow(newEntryIdx, false);
}

ChapterPtr
Tab::createEmptyChapter(int64_t startTime,
                        int chapterNumber,
                        boost::optional<QString> const &nameTemplate,
                        boost::optional<QString> const &language,
                        boost::optional<QString> const &country) {
  auto &cfg     = Util::Settings::get();
  auto chapter  = std::make_shared<KaxChapterAtom>();
  auto &display = GetChild<KaxChapterDisplay>(*chapter);
  auto name     = formatChapterName(nameTemplate ? *nameTemplate : cfg.m_chapterNameTemplate, chapterNumber, timestamp_c::ns(startTime));

  GetChild<KaxChapterUID>(*chapter).SetValue(0);
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTime);
  GetChild<KaxChapterString>(display).SetValue(to_wide(name));
  GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(language ? *language : cfg.m_defaultChapterLanguage));
  if ((country && !country->isEmpty()) || !cfg.m_defaultChapterCountry.isEmpty())
    GetChild<KaxChapterCountry>(display).SetValue(to_utf8((country && !country->isEmpty()) ? *country : cfg.m_defaultChapterCountry));

  return chapter;
}

void
Tab::addChapterBefore() {
  addChapter(true);
}

void
Tab::addChapterAfter() {
  addChapter(false);
}

QModelIndex
Tab::addChapter(bool before) {
  Q_D(Tab);

  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  if (!selectedIdx.isValid() || !selectedIdx.parent().isValid())
    return {};

  // TODO: Tab::addChapter: start time
  auto row     = selectedIdx.row() + (before ? 0 : 1);
  auto chapter = createEmptyChapter(0, row + 1);

  d->chapterModel->insertChapter(row, chapter, selectedIdx.parent());

  emit numberOfEntriesChanged();

  return d->chapterModel->index(row, 0, selectedIdx.parent());
}

void
Tab::addSubChapter() {
  Q_D(Tab);

  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  if (!selectedIdx.isValid())
    return;

  // TODO: Tab::addSubChapter: start time
  auto selectedItem = d->chapterModel->itemFromIndex(selectedIdx);
  auto chapter      = createEmptyChapter(0, (selectedItem ? selectedItem->rowCount() : 0) + 1);

  d->chapterModel->appendChapter(chapter, selectedIdx);
  expandCollapseAll(true, selectedIdx);

  emit numberOfEntriesChanged();
}

void
Tab::removeElement() {
  Q_D(Tab);

  d->chapterModel->removeTree(Util::selectedRowIdx(d->ui->elements));
  emit numberOfEntriesChanged();
}

void
Tab::applyModificationToTimestamps(QStandardItem *item,
                                   std::function<int64_t(int64_t)> const &unaryOp) {
  Q_D(Tab);

  if (!item)
    return;

  if (item->parent()) {
    auto chapter = d->chapterModel->chapterFromItem(item);
    if (chapter) {
      auto kStart = FindChild<KaxChapterTimeStart>(*chapter);
      auto kEnd   = FindChild<KaxChapterTimeEnd>(*chapter);

      if (kStart)
        kStart->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kStart->GetValue())), 0));
      if (kEnd)
        kEnd->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kEnd->GetValue())), 0));

      if (kStart || kEnd)
        d->chapterModel->updateRow(item->index());
    }
  }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    applyModificationToTimestamps(item->child(row), unaryOp);
}

void
Tab::multiplyTimestamps(QStandardItem *item,
                        double factor) {
  applyModificationToTimestamps(item, [=](int64_t timestamp) { return static_cast<int64_t>((timestamp * factor) * 10.0 + 5) / 10ll; });
}

void
Tab::shiftTimestamps(QStandardItem *item,
                     int64_t delta) {
  applyModificationToTimestamps(item, [=](int64_t timestamp) { return timestamp + delta; });
}

void
Tab::constrictTimestamps(QStandardItem *item,
                         boost::optional<uint64_t> const &constrictStart,
                         boost::optional<uint64_t> const &constrictEnd) {
  Q_D(Tab);

  if (!item)
    return;

  auto chapter = item->parent() ? d->chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      constrictTimestamps(item->child(row), {}, {});
    return;
  }

  auto kStart   = &GetChild<KaxChapterTimeStart>(*chapter);
  auto kEnd     = FindChild<KaxChapterTimeEnd>(*chapter);
  auto newStart = !constrictStart ? kStart->GetValue()
                : !constrictEnd   ? std::max(*constrictStart, kStart->GetValue())
                :                   std::min(*constrictEnd, std::max(*constrictStart, kStart->GetValue()));
  auto newEnd   = !kEnd           ? boost::optional<uint64_t>{}
                : !constrictEnd   ? std::max(newStart, kEnd->GetValue())
                :                   std::max(newStart, std::min(*constrictEnd, kEnd->GetValue()));

  kStart->SetValue(newStart);
  if (newEnd)
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);

  d->chapterModel->updateRow(item->index());

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    constrictTimestamps(item->child(row), newStart, newEnd);
}

std::pair<boost::optional<uint64_t>, boost::optional<uint64_t>>
Tab::expandTimestamps(QStandardItem *item) {
  Q_D(Tab);

  if (!item)
    return {};

  auto chapter = item->parent() ? d->chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      expandTimestamps(item->child(row));
    return {};
  }

  auto kStart   = chapter ? FindChild<KaxChapterTimeStart>(*chapter)      : nullptr;
  auto kEnd     = chapter ? FindChild<KaxChapterTimeEnd>(*chapter)        : nullptr;
  auto newStart = kStart  ? boost::optional<uint64_t>{kStart->GetValue()} : boost::optional<uint64_t>{};
  auto newEnd   = kEnd    ? boost::optional<uint64_t>{kEnd->GetValue()}   : boost::optional<uint64_t>{};
  auto modified = false;

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row) {
    auto startAndEnd = expandTimestamps(item->child(row));

    if (!newStart || (startAndEnd.first && (*startAndEnd.first < *newStart)))
      newStart = startAndEnd.first;

    if (!newEnd || (startAndEnd.second && (*startAndEnd.second > *newEnd)))
      newEnd = startAndEnd.second;
  }

  if (newStart && (!kStart || (kStart->GetValue() > *newStart))) {
    GetChild<KaxChapterTimeStart>(*chapter).SetValue(*newStart);
    modified = true;
  }

  if (newEnd && (!kEnd || (kEnd->GetValue() < *newEnd))) {
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);
    modified = true;
  }

  if (modified)
    d->chapterModel->updateRow(item->index());

  return std::make_pair(newStart, newEnd);
}

void
Tab::setLanguages(QStandardItem *item,
                  QString const &language) {
  Q_D(Tab);

  if (!item)
    return;

  auto chapter = d->chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (kDisplay)
        GetChild<KaxChapterLanguage>(*kDisplay).SetValue(to_utf8(language));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setLanguages(item->child(row), language);
}

void
Tab::setCountries(QStandardItem *item,
                  QString const &country) {
  Q_D(Tab);

  if (!item)
    return;

  auto chapter = d->chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (!kDisplay)
        continue;

      if (country.isEmpty())
        DeleteChildren<KaxChapterCountry>(*kDisplay);
      else
        GetChild<KaxChapterCountry>(*kDisplay).SetValue(to_utf8(country));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setCountries(item->child(row), country);
}

void
Tab::setEndTimestamps(QStandardItem *startItem) {
  Q_D(Tab);

  if (!startItem)
    return;

  auto allAtomData = collectChapterAtomDataForEdition(startItem);

  std::function<void(QStandardItem *)> setter = [d, &allAtomData, &setter](QStandardItem *item) {
    if (item->parent()) {
      auto chapter = d->chapterModel->chapterFromItem(item);
      if (chapter) {
        auto data = allAtomData[chapter.get()];
        if (data && data->calculatedEnd.valid()) {
          GetChild<KaxChapterTimeEnd>(*chapter).SetValue(data->calculatedEnd.to_ns());
          d->chapterModel->updateRow(item->index());
        }
      }
    }

    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      setter(item->child(row));
  };

  setter(startItem);

  setControlsFromStorage();
}

void
Tab::massModify() {
  Q_D(Tab);

  if (!copyControlsToStorage())
    return;

  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  auto item        = selectedIdx.isValid() ? d->chapterModel->itemFromIndex(selectedIdx) : d->chapterModel->invisibleRootItem();

  MassModificationDialog dlg{this, selectedIdx.isValid(), usedNameLanguages(), usedNameCountryCodes()};
  if (!dlg.exec())
    return;

  auto actions = dlg.actions();

  if (actions & MassModificationDialog::Shift)
    shiftTimestamps(item, dlg.shiftBy());

  if (actions & MassModificationDialog::Multiply)
    multiplyTimestamps(item, dlg.multiplyBy());

  if (actions & MassModificationDialog::Constrict)
    constrictTimestamps(item, {}, {});

  if (actions & MassModificationDialog::Expand)
    expandTimestamps(item);

  if (actions & MassModificationDialog::SetLanguage)
    setLanguages(item, dlg.language());

  if (actions & MassModificationDialog::SetCountry)
    setCountries(item, dlg.country());

  if (actions & MassModificationDialog::Sort)
    item->sortChildren(1);

  if (actions & MassModificationDialog::SetEndTimestamps)
    setEndTimestamps(item);

  setControlsFromStorage();
}

void
Tab::duplicateElement() {
  Q_D(Tab);

  auto selectedIdx   = Util::selectedRowIdx(d->ui->elements);
  auto newElementIdx = d->chapterModel->duplicateTree(selectedIdx);

  if (newElementIdx.isValid())
    expandCollapseAll(true, newElementIdx);

  emit numberOfEntriesChanged();
}

QString
Tab::formatChapterName(QString const &nameTemplate,
                       int chapterNumber,
                       timestamp_c const &startTimestamp)
  const {
  return Q(mtx::chapters::format_name_template(to_utf8(nameTemplate), chapterNumber, startTimestamp));
}

void
Tab::generateSubChapters() {
  Q_D(Tab);

  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = d->chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = d->chapterModel->chapterFromItem(selectedItem);
  auto maxEndTimestamp = selectedChapter ? FindChildValue<KaxChapterTimeStart>(*selectedChapter, 0ull) : 0ull;
  auto numRows         = selectedItem->rowCount();

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = d->chapterModel->chapterFromItem(selectedItem->child(row));
    if (chapter)
      maxEndTimestamp = std::max(maxEndTimestamp, std::max(FindChildValue<KaxChapterTimeStart>(*chapter, 0ull), FindChildValue<KaxChapterTimeEnd>(*chapter, 0ull)));
  }

  GenerateSubChaptersParametersDialog dlg{this, numRows + 1, maxEndTimestamp, usedNameLanguages(), usedNameCountryCodes()};
  if (!dlg.exec())
    return;

  auto toCreate      = dlg.numberOfEntries();
  auto chapterNumber = dlg.firstChapterNumber();
  auto timestamp     = dlg.startTimestamp();
  auto duration      = dlg.durationInNs();
  auto nameTemplate  = dlg.nameTemplate();
  auto language      = dlg.language();
  auto country       = dlg.country();

  while (toCreate > 0) {
    auto chapter = createEmptyChapter(timestamp, chapterNumber, nameTemplate, language, country);
    timestamp   += duration;

    ++chapterNumber;
    --toCreate;

    d->chapterModel->appendChapter(chapter, selectedIdx);
  }

  expandCollapseAll(true, selectedIdx);
  resizeNameColumnsToContents();

  emit numberOfEntriesChanged();
}

bool
Tab::changeChapterName(QModelIndex const &parentIdx,
                       int row,
                       int chapterNumber,
                       QString const &nameTemplate,
                       RenumberSubChaptersParametersDialog::NameMatch nameMatchingMode,
                       QString const &languageOfNamesToReplace,
                       bool skipHidden) {
  Q_D(Tab);

  auto idx     = d->chapterModel->index(row, 0, parentIdx);
  auto item    = d->chapterModel->itemFromIndex(idx);
  auto chapter = d->chapterModel->chapterFromItem(item);

  if (!chapter)
    return false;

  if (skipHidden) {
    auto flagHidden = FindChild<KaxChapterFlagHidden>(*chapter);
    if (flagHidden && flagHidden->GetValue())
      return false;
  }

  auto startTimestamp = FindChildValue<KaxChapterTimeStart>(*chapter);
  auto name           = to_wide(formatChapterName(nameTemplate, chapterNumber, timestamp_c::ns(startTimestamp)));

  if (RenumberSubChaptersParametersDialog::NameMatch::First == nameMatchingMode) {
    GetChild<KaxChapterString>(GetChild<KaxChapterDisplay>(*chapter)).SetValue(name);
    d->chapterModel->updateRow(idx);

    return true;
  }

  for (auto const &element : *chapter) {
    auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
    if (!kDisplay)
      continue;

    auto language = FindChildValue<KaxChapterLanguage>(kDisplay, std::string{"eng"});
    if (   (RenumberSubChaptersParametersDialog::NameMatch::All == nameMatchingMode)
        || (Q(language)                                         == languageOfNamesToReplace))
      GetChild<KaxChapterString>(*kDisplay).SetValue(name);
  }

  d->chapterModel->updateRow(idx);

  return true;
}

void
Tab::renumberSubChapters() {
  Q_D(Tab);

  auto selectedIdx = Util::selectedRowIdx(d->ui->elements);
  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = d->chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = d->chapterModel->chapterFromItem(selectedItem);
  auto numRows         = selectedItem->rowCount();
  auto chapterTitles   = QStringList{};
  auto firstName       = QString{};

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = d->chapterModel->chapterFromItem(selectedItem->child(row));
    if (!chapter)
      continue;

    auto start = GetChild<KaxChapterTimeStart>(*chapter).GetValue();
    auto end   = FindChild<KaxChapterTimeEnd>(*chapter);
    auto name  = ChapterModel::chapterDisplayName(*chapter);

    if (firstName.isEmpty())
      firstName = name;

    if (end)
      chapterTitles << Q("%1 (%2 – %3)").arg(name).arg(Q(format_timestamp(start))).arg(Q(format_timestamp(end->GetValue())));
    else
      chapterTitles << Q("%1 (%2)").arg(name).arg(Q(format_timestamp(start)));
  }

  auto matches       = QRegularExpression{Q("(\\d+)$")}.match(firstName);
  auto firstNumber   = matches.hasMatch() ? matches.captured(0).toInt() : 1;
  auto usedLanguages = usedNameLanguages(selectedItem);

  RenumberSubChaptersParametersDialog dlg{this, firstNumber, chapterTitles, usedLanguages};
  if (!dlg.exec())
    return;

  auto row               = dlg.firstEntryToRenumber();
  auto toRenumber        = dlg.numberOfEntries() ? dlg.numberOfEntries() : numRows;
  auto chapterNumber     = dlg.firstChapterNumber();
  auto nameTemplate      = dlg.nameTemplate();
  auto nameMatchingMode  = dlg.nameMatchingMode();
  auto languageToReplace = dlg.languageOfNamesToReplace();
  auto skipHidden        = dlg.skipHidden();

  while ((row < numRows) && (0 < toRenumber)) {
    auto renumbered = changeChapterName(selectedIdx, row, chapterNumber, nameTemplate, nameMatchingMode, languageToReplace, skipHidden);

    if (renumbered)
      ++chapterNumber;
    ++row;
  }

  resizeNameColumnsToContents();
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  Q_D(Tab);

  if (parentIdx.isValid())
    d->ui->elements->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = d->chapterModel->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(expand, d->chapterModel->index(row, 0, parentIdx));
}

void
Tab::expandInsertedElements(QModelIndex const &parentIdx,
                            int,
                            int) {
  expandCollapseAll(true, parentIdx);
}

QString
Tab::formatEbmlBinary(EbmlBinary *binary) {
  auto value = std::string{};
  auto data  = static_cast<unsigned char const *>(binary ? binary->GetBuffer() : nullptr);

  if (data)
    for (auto end = data + binary->GetSize(); data < end; ++data)
      value += (boost::format("%|1$02x|") % static_cast<unsigned int>(*data)).str();

  return Q(value);
}

void
Tab::showChapterContextMenu(QPoint const &pos) {
  Q_D(Tab);

  auto selectedIdx     = Util::selectedRowIdx(d->ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();
  auto hasEntries      = !!d->chapterModel->rowCount();
  auto hasSubEntries   = selectedIdx.isValid() ? !!d->chapterModel->rowCount(selectedIdx) : false;

  d->addChapterBeforeAction->setEnabled(chapterSelected);
  d->addChapterAfterAction->setEnabled(chapterSelected);
  d->addSubChapterAction->setEnabled(hasSelection);
  d->generateSubChaptersAction->setEnabled(hasSelection);
  d->renumberSubChaptersAction->setEnabled(hasSelection && hasSubEntries);
  d->removeElementAction->setEnabled(hasSelection);
  d->duplicateAction->setEnabled(hasSelection);
  d->expandAllAction->setEnabled(hasEntries);
  d->collapseAllAction->setEnabled(hasEntries);

  QMenu menu{this};

  menu.addAction(d->addEditionBeforeAction);
  menu.addAction(d->addEditionAfterAction);
  menu.addSeparator();
  menu.addAction(d->addChapterBeforeAction);
  menu.addAction(d->addChapterAfterAction);
  menu.addAction(d->addSubChapterAction);
  menu.addAction(d->generateSubChaptersAction);
  menu.addSeparator();
  menu.addAction(d->duplicateAction);
  menu.addSeparator();
  menu.addAction(d->removeElementAction);
  menu.addSeparator();
  menu.addAction(d->renumberSubChaptersAction);
  menu.addAction(d->massModificationAction);
  menu.addSeparator();
  menu.addAction(d->expandAllAction);
  menu.addAction(d->collapseAllAction);

  menu.exec(d->ui->elements->viewport()->mapToGlobal(pos));
}

bool
Tab::isSourceMatroska()
  const {
  Q_D(const Tab);

  return !!d->analyzer;
}

bool
Tab::hasChapters()
  const {
  Q_D(const Tab);

  for (auto idx = 0, numEditions = d->chapterModel->rowCount(); idx < numEditions; ++idx)
    if (d->chapterModel->item(idx)->rowCount())
      return true;
  return false;
}

QString
Tab::currentState()
  const {
  Q_D(const Tab);

  auto chapters = d->chapterModel->allChapters();
  return chapters ? Q(ebml_dumper_c::dump_to_string(chapters.get(), static_cast<ebml_dumper_c::dump_style_e>(ebml_dumper_c::style_with_values | ebml_dumper_c::style_with_indexes))) : QString{};
}

bool
Tab::hasBeenModified()
  const {
  Q_D(const Tab);

  return currentState() != d->savedState;
}

bool
Tab::focusNextChapterName() {
  Q_D(Tab);

  auto selectedRows = d->ui->tvChNames->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return false;

  auto nextRow = selectedRows.at(0).row() + 1;
  if (nextRow >= d->nameModel->rowCount())
    return false;

  Util::selectRow(d->ui->tvChNames, nextRow);

  return true;
}

bool
Tab::focusNextChapterAtom(FocusElementType toFocus) {
  Q_D(Tab);

  auto doSelect = [d, toFocus](QModelIndex const &idx) -> bool {
    Util::selectRow(d->ui->elements, idx.row(), idx.parent());

    auto lineEdit = FocusChapterStartTime == toFocus ? d->ui->leChStart : d->ui->leChName;
    lineEdit->selectAll();
    lineEdit->setFocus();

    return true;
  };

  auto selectedRows = d->ui->elements->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return false;

  auto selectedIdx  = selectedRows.at(0);
  selectedIdx       = selectedIdx.sibling(selectedIdx.row(), 0);
  auto selectedItem = d->chapterModel->itemFromIndex(selectedIdx);

  if (selectedItem->rowCount())
    return doSelect(selectedIdx.child(0, 0));

  auto parentIdx  = selectedIdx.parent();
  auto parentItem = d->chapterModel->itemFromIndex(parentIdx);
  auto nextRow    = selectedIdx.row() + 1;

  if (nextRow < parentItem->rowCount())
    return doSelect(parentIdx.child(nextRow, 0));

  while (parentIdx.parent().isValid()) {
    nextRow    = parentIdx.row() + 1;
    parentIdx  = parentIdx.parent();
    parentItem = d->chapterModel->itemFromIndex(parentIdx);

    if (nextRow < parentItem->rowCount())
      return doSelect(parentIdx.child(nextRow, 0));
  }

  auto numEditions = d->chapterModel->rowCount();
  auto editionIdx  = parentIdx.sibling((parentIdx.row() + 1) % numEditions, 0);

  while (numEditions) {
    if (d->chapterModel->itemFromIndex(editionIdx)->rowCount())
      return doSelect(editionIdx.child(0, 0));

    editionIdx = editionIdx.sibling((editionIdx.row() + 1) % numEditions, 0);
  }

  return false;
}

void
Tab::focusNextChapterElement(bool keepSameElement) {
  Q_D(Tab);

  if (!copyControlsToStorage())
    return;

  if (QObject::sender() == d->ui->leChName) {
    if (focusNextChapterName())
      return;

    focusNextChapterAtom(keepSameElement ? FocusChapterName : FocusChapterStartTime);
    return;
  }

  if (!keepSameElement) {
    d->ui->leChName->selectAll();
    d->ui->leChName->setFocus();
    return;
  }

  focusNextChapterAtom(FocusChapterStartTime);
}

void
Tab::focusOtherControlInNextChapterElement() {
  focusNextChapterElement(false);
}

void
Tab::focusSameControlInNextChapterElement() {
  focusNextChapterElement(true);
}

void
Tab::closeTab() {
  emit removeThisTab();
}

void
Tab::addSegmentUIDFromFile() {
  Q_D(Tab);

  Util::addSegmentUIDFromFileToLineEdit(*this, *d->ui->leChSegmentUid, false);
}

QStringList
Tab::usedNameLanguages(QStandardItem *rootItem) {
  Q_D(Tab);

  if (!rootItem)
    rootItem = d->chapterModel->invisibleRootItem();

  auto names = QSet<QString>{};

  std::function<void(QStandardItem *)> collector = [d, &collector, &names](auto *currentItem) {
    if (!currentItem)
      return;

    auto chapter = d->chapterModel->chapterFromItem(currentItem);
    if (chapter)
      for (auto const &element : *chapter) {
        auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
        if (!kDisplay)
          continue;

        auto kLanguage = FindChild<KaxChapterLanguage>(*kDisplay);
        names << (kLanguage ? Q(*kLanguage) : Q("eng"));
      }

    for (int row = 0, numRows = currentItem->rowCount(); row < numRows; ++row)
      collector(currentItem->child(row));
  };

  collector(rootItem);

  return names.toList();
}

QStringList
Tab::usedNameCountryCodes(QStandardItem *rootItem) {
  Q_D(Tab);

  if (!rootItem)
    rootItem = d->chapterModel->invisibleRootItem();

  auto countryCodes = QSet<QString>{};

  std::function<void(QStandardItem *)> collector = [d, &collector, &countryCodes](auto *currentItem) {
    if (!currentItem)
      return;

    auto chapter = d->chapterModel->chapterFromItem(currentItem);
    if (chapter)
      for (auto const &element : *chapter) {
        auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
        if (!kDisplay)
          continue;

        auto kCountry = FindChild<KaxChapterCountry>(*kDisplay);
        if (kCountry)
          countryCodes << Q(*kCountry);
      }

    for (int row = 0, numRows = currentItem->rowCount(); row < numRows; ++row)
      collector(currentItem->child(row));
  };

  collector(rootItem);

  return countryCodes.toList();
}

QHash<KaxChapterAtom *, ChapterAtomDataPtr>
Tab::collectChapterAtomDataForEdition(QStandardItem *item) {
  Q_D(Tab);

  if (!item)
    return {};

  QHash<KaxChapterAtom *, ChapterAtomDataPtr> allAtoms;
  QHash<KaxChapterAtom *, QVector<ChapterAtomDataPtr>> atomsByParent;
  QVector<ChapterAtomDataPtr> atomList;

  // Collect all existing start and end timestamps.
  std::function<void(QStandardItem *, KaxChapterAtom *, int)> collector = [d, &collector, &allAtoms, &atomsByParent, &atomList](auto *currentItem, auto *parentAtom, int level) {
    if (!currentItem)
      return;

    QVector<ChapterAtomDataPtr> currentAtoms;

    auto chapter = d->chapterModel->chapterFromItem(currentItem);
    if (chapter && currentItem->parent()) {
      auto timeEndKax     = FindChild<KaxChapterTimeEnd>(*chapter);
      auto displayKax     = FindChild<KaxChapterDisplay>(*chapter);

      auto data           = std::make_shared<ChapterAtomData>();
      data->atom          = chapter.get();
      data->parentAtom    = parentAtom;
      data->level         = level - 1;
      data->start         = timestamp_c::ns(GetChildValue<KaxChapterTimeStart>(*chapter));

      if (timeEndKax)
        data->end         = timestamp_c::ns(timeEndKax->GetValue());

      if (displayKax)
        data->primaryName = Q(FindChildValue<KaxChapterString>(*displayKax));

      allAtoms.insert(chapter.get(), data);
      atomsByParent[parentAtom].append(data);
      atomList.append(data);
    }

    for (int row = 0, numRows = currentItem->rowCount(); row < numRows; ++row)
      collector(currentItem->child(row), chapter.get(), level + 1);
  };

  std::function<void(QStandardItem *)> calculator = [d, &calculator, &allAtoms, &atomsByParent](auto *parentItem) {
    if (!parentItem || !parentItem->rowCount())
      return;

    auto parentAtom         = d->chapterModel->chapterFromItem(parentItem);
    auto parentData         = allAtoms[parentAtom.get()];
    auto &sortedData        = atomsByParent[parentAtom.get()];
    auto parentEndTimestamp = parentData ? parentData->calculatedEnd : d->fileEndTimestamp;

    for (int row = 0, numRows = parentItem->rowCount(); row < numRows; ++row) {
      auto atom = d->chapterModel->chapterFromItem(parentItem->child(row));
      auto data = allAtoms[atom.get()];

      if (!data)
        continue;

      auto itr            = brng::lower_bound(sortedData, data, [](auto const &a, auto const &b) { return a->start <= b->start; });
      data->calculatedEnd = itr == sortedData.end() ? parentEndTimestamp : (*itr)->start;
    }

    for (int row = 0, numRows = parentItem->rowCount(); row < numRows; ++row)
      calculator(parentItem->child(row));
  };

  // Determine edition we're working in.
  while (item->parent())
    item = item->parent();

  // Collect existing tree structure & basic data for each atom.
  collector(item, nullptr, 0);

  // Sort all levels by their start times.
  for (auto const &key : atomsByParent.keys())
    brng::sort(atomsByParent[key], [](ChapterAtomDataPtr const &a, ChapterAtomDataPtr const &b) { return a->start < b->start; });

  // Calculate end timestamps.
  calculator(item);

  // Output debug info.
  for (auto const &data : atomList)
    qDebug() <<
      Q((boost::format("collectChapterAtomData: data %1%%2% start %3% end %4% [%5%] atom %6% parent %7%")
         % std::string(data->level * 2, ' ')
         % to_utf8(data->primaryName)
         % data->start
         % data->end
         % data->calculatedEnd
         % data->atom
         % data->parentAtom));

  return allAtoms;
}

}}}
