#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

#include <matroska/KaxSemantic.h>

#include "common/bitvalue.h"
#include "common/bluray/mpls.h"
#include "common/chapters/bluray.h"
#include "common/chapters/chapters.h"
#include "common/chapters/dvd.h"
#include "common/ebml.h"
#include "common/kax_file.h"
#include "common/math.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/xml/ebml_chapters_converter.h"
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
#include "mkvtoolnix-gui/util/tree.h"
#include "mkvtoolnix-gui/util/widget.h"

using namespace libmatroska;
using namespace mtx::gui;

namespace mtx::bcp47 {

uint
qHash(language_c const &language) {
  return qHash(Q(language.format()));
}

}

namespace mtx::gui::ChapterEditor {

namespace {

bool
removeLanguageControlRow(QVector<std::tuple<QBoxLayout *, QWidget *, QPushButton *>> &controls,
                         QObject *sender) {
  for (int idx = 0, numControls = controls.size(); idx < numControls; ++idx) {
    auto &[layout, widget, button] = controls[idx];

    if (button != sender)
      continue;

    delete layout;
    delete widget;
    delete button;

    controls.removeAt(idx);

    return true;
  }

  return false;
}

}

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
  , copyToOtherTabMenu{new QMenu{&tab}}
{
}

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , p_ptr{new TabPrivate{*this, fileName}}
{
  setup();
}

Tab::Tab(QWidget *parent,
         TabPrivate &p)
  : QWidget{parent}
  , p_ptr{&p}
{
  setup();
}

Tab::~Tab() {
}

void
Tab::setup() {
  auto p = p_func();

  // Setup UI controls.
  p->ui->setupUi(this);

  setupUi();

  retranslateUi();
}

void
Tab::setupUi() {
  auto p = p_func();

  Util::Settings::get().handleSplitterSizes(p->ui->chapterEditorSplitter);

  p->ui->elements->setModel(p->chapterModel);
  p->ui->tvChNames->setModel(p->nameModel);

  p->ui->elements->acceptDroppedFiles(true);

  p->languageControls.push_back({ nullptr, p->ui->ldwChNameLanguage1, p->ui->pbChNameLanguageAdd });

  p->nameWidgets << p->ui->pbChRemoveName << p->ui->lChName << p->ui->leChName;

  Util::fixScrollAreaBackground(p->ui->scrollArea);
  Util::HeaderViewManager::create(*p->ui->elements,  "ChapterEditor::Elements")    .setDefaultSizes({ { Q("editionChapter"), 200 }, { Q("start"),    130 }, { Q("end"), 130 } });
  Util::HeaderViewManager::create(*p->ui->tvChNames, "ChapterEditor::ChapterNames").setDefaultSizes({ { Q("name"),           200 }, { Q("language"), 150 } });

  p->addEditionBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  p->addEditionAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  p->addChapterBeforeAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-above.png")});
  p->addChapterAfterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-below.png")});
  p->addSubChapterAction->setIcon(QIcon{Q(":/icons/16x16/edit-table-insert-row-under.png")});
  p->duplicateAction->setIcon(QIcon{Q(":/icons/16x16/tab-duplicate.png")});
  p->removeElementAction->setIcon(QIcon{Q(":/icons/16x16/list-remove.png")});
  p->renumberSubChaptersAction->setIcon(QIcon{Q(":/icons/16x16/format-list-ordered.png")});
  p->massModificationAction->setIcon(QIcon{Q(":/icons/16x16/tools-wizard.png")});
  p->copyToOtherTabMenu->setIcon(QIcon{Q(":/icons/16x16/edit-copy.png")});

  auto tool = MainWindow::chapterEditorTool();
  connect(p->ui->elements,                    &Util::BasicTreeView::customContextMenuRequested,                       this,                    &Tab::showChapterContextMenu);
  connect(p->ui->elements,                    &Util::BasicTreeView::deletePressed,                                    this,                    &Tab::removeElement);
  connect(p->ui->elements,                    &Util::BasicTreeView::insertPressed,                                    this,                    &Tab::addEditionOrChapterAfter);
  connect(p->ui->elements,                    &Util::BasicTreeView::filesDropped,                                     tool,                    &Tool::openFiles);
  connect(p->ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this,                    &Tab::chapterSelectionChanged);
  connect(p->ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged,                                 this,                    &Tab::nameSelectionChanged);
  connect(p->ui->leChName,                    &QLineEdit::textEdited,                                                 this,                    &Tab::chapterNameEdited);
  connect(p->ui->ldwChNameLanguage1,          &Util::LanguageDisplayWidget::languageChanged,                          this,                    &Tab::chapterNameLanguageChanged);
  connect(p->ui->pbChAddName,                 &QPushButton::clicked,                                                  this,                    &Tab::addChapterName);
  connect(p->ui->pbChRemoveName,              &QPushButton::clicked,                                                  this,                    &Tab::removeChapterName);
  connect(p->ui->pbBrowseSegmentUID,          &QPushButton::clicked,                                                  this,                    &Tab::addSegmentUIDFromFile);
  connect(p->ui->pbChNameLanguageAdd,         &QPushButton::clicked,                                                  this,                    &Tab::addChapterNameLanguage);

  connect(p->expandAllAction,                 &QAction::triggered,                                                    this,                    &Tab::expandAll);
  connect(p->collapseAllAction,               &QAction::triggered,                                                    this,                    &Tab::collapseAll);
  connect(p->addEditionBeforeAction,          &QAction::triggered,                                                    this,                    &Tab::addEditionBefore);
  connect(p->addEditionAfterAction,           &QAction::triggered,                                                    this,                    &Tab::addEditionAfter);
  connect(p->addChapterBeforeAction,          &QAction::triggered,                                                    this,                    &Tab::addChapterBefore);
  connect(p->addChapterAfterAction,           &QAction::triggered,                                                    this,                    &Tab::addChapterAfter);
  connect(p->addSubChapterAction,             &QAction::triggered,                                                    this,                    &Tab::addSubChapter);
  connect(p->removeElementAction,             &QAction::triggered,                                                    this,                    &Tab::removeElement);
  connect(p->duplicateAction,                 &QAction::triggered,                                                    this,                    &Tab::duplicateElement);
  connect(p->massModificationAction,          &QAction::triggered,                                                    this,                    &Tab::massModify);
  connect(p->generateSubChaptersAction,       &QAction::triggered,                                                    this,                    &Tab::generateSubChapters);
  connect(p->renumberSubChaptersAction,       &QAction::triggered,                                                    this,                    &Tab::renumberSubChapters);

  for (auto &lineEdit : findChildren<Util::BasicLineEdit *>()) {
    lineEdit->acceptDroppedFiles(false).setTextToDroppedFileName(false);
    connect(lineEdit, &Util::BasicLineEdit::returnPressed,      this, &Tab::focusOtherControlInNextChapterElement);
    connect(lineEdit, &Util::BasicLineEdit::shiftReturnPressed, this, &Tab::focusSameControlInNextChapterElement);
  }
}

void
Tab::updateFileNameDisplay() {
  auto p = p_func();

  if (!p->fileName.isEmpty()) {
    auto info = QFileInfo{p->fileName};
    p->ui->fileName->setText(info.fileName());
    p->ui->directory->setText(QDir::toNativeSeparators(info.path()));

  } else {
    p->ui->fileName->setText(QY("<Unsaved file>"));
    p->ui->directory->setText(Q(""));

  }
}

void
Tab::retranslateUi() {
  auto p = p_func();

  p->ui->retranslateUi(this);

  updateFileNameDisplay();

  p->expandAllAction->setText(QY("&Expand all"));
  p->collapseAllAction->setText(QY("&Collapse all"));
  p->addEditionBeforeAction->setText(QY("Add new e&dition before"));
  p->addEditionAfterAction->setText(QY("Add new ed&ition after"));
  p->addChapterBeforeAction->setText(QY("Add new c&hapter before"));
  p->addChapterAfterAction->setText(QY("Add new ch&apter after"));
  p->addSubChapterAction->setText(QY("Add new &sub-chapter inside"));
  p->removeElementAction->setText(QY("&Remove selected edition or chapter"));
  p->duplicateAction->setText(QY("D&uplicate selected edition or chapter"));
  p->massModificationAction->setText(QY("Additional &modifications"));
  p->generateSubChaptersAction->setText(QY("&Generate sub-chapters"));
  p->renumberSubChaptersAction->setText(QY("Re&number sub-chapters"));
  p->copyToOtherTabMenu->setTitle(QY("Cop&y to other tab"));

  setupToolTips();

  p->chapterModel->retranslateUi();
  p->nameModel->retranslateUi();

  Q_EMIT titleChanged();
}

void
Tab::setupToolTips() {
  auto p = p_func();

  Util::setToolTip(p->ui->elements, QY("Right-click for actions for editions and chapters"));
  Util::setToolTip(p->ui->pbBrowseSegmentUID, QY("Select an existing Matroska or WebM file and the GUI will add its segment UID to the input field on the left."));
}

QString
Tab::title()
  const {
  auto p = p_func();

  if (p->fileName.isEmpty())
    return QY("<Unsaved file>");
  return QFileInfo{p->fileName}.fileName();
}

QString const &
Tab::fileName()
  const {
  auto p = p_func();

  return p->fileName;
}

void
Tab::newFile() {
  auto p = p_func();

  addEdition(false);

  auto selectionModel = p->ui->elements->selectionModel();
  auto selection      = QItemSelection{p->chapterModel->index(0, 0), p->chapterModel->index(0, p->chapterModel->columnCount() - 1)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);

  addSubChapter();

  auto parentIdx = p->chapterModel->index(0, 0);
  selection      = QItemSelection{p->chapterModel->index(0, 0, parentIdx), p->chapterModel->index(0, p->chapterModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

  p->ui->leChStart->selectAll();
  p->ui->leChStart->setFocus();

  p->savedState = currentState();
}

void
Tab::resetData() {
  auto p = p_func();

  p->analyzer.reset();
  p->nameModel->reset();
  p->chapterModel->reset();
}

bool
Tab::readFileEndTimestampForMatroska(kax_analyzer_c &analyzer) {
  auto p = p_func();

  p->fileEndTimestamp.reset();

  auto idx = analyzer.find(KaxInfo::ClassInfos.GlobalId);
  if (-1 == idx) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(p->fileName)).exec();
    return false;
  }

  auto info = analyzer.read_element(idx);
  if (!info) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(p->fileName)).exec();
    return false;
  }

  auto durationKax = FindChild<KaxDuration>(*info);
  if (!durationKax) {
    qDebug() << "readFileEndTimestampForMatroska: no duration found";
    return true;
  }

  auto timestampScale = FindChildValue<KaxTimecodeScale, uint64_t>(static_cast<KaxInfo &>(*info), TIMESTAMP_SCALE);
  auto duration       = timestamp_c::ns(durationKax->GetValue() * timestampScale);

  qDebug() << "readFileEndTimestampForMatroska: duration is" << Q(mtx::string::format_timestamp(duration));

  auto &fileIo = analyzer.get_file();
  fileIo.setFilePointer(analyzer.get_segment_data_start_pos());

  kax_file_c fileKax{fileIo};
  fileKax.enable_reporting(false);

  auto cluster = fileKax.read_next_cluster();
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

  p->fileEndTimestamp = minBlockTimestamp + duration;

  qDebug() << "readFileEndTimestampForMatroska: minBlockTimestamp" << Q(mtx::string::format_timestamp(minBlockTimestamp)) << "result" << Q(mtx::string::format_timestamp(p->fileEndTimestamp));

  return true;
}

Tab::LoadResult
Tab::loadFromMatroskaFile(QString const &fileName,
                          bool append) {
  auto p        = p_func();
  auto analyzer = std::make_unique<Util::KaxAnalyzer>(this, fileName);

  if (!analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).set_open_mode(MODE_READ).process()) {
    auto text = Q("%1 %2")
      .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName))
      .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();

    if (!append)
      Q_EMIT removeThisTab();
    return {};
  }

  if (!append && !readFileEndTimestampForMatroska(*analyzer)) {
    Q_EMIT removeThisTab();
    return {};
  }

  auto idx = analyzer->find(KaxChapters::ClassInfos.GlobalId);
  if (-1 == idx) {
    analyzer->close_file();

    if (!append)
      p->analyzer = std::move(analyzer);

    return { std::make_shared<KaxChapters>(), true };
  }

  auto chapters = analyzer->read_element(idx);
  if (!chapters) {
    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("The file you tried to open (%1) could not be read successfully.").arg(fileName)).exec();
    if (!append)
      Q_EMIT removeThisTab();
    return {};
  }

  analyzer->close_file();

  if (!append)
    p->analyzer = std::move(analyzer);

  return { std::static_pointer_cast<KaxChapters>(chapters), true };
}

Tab::LoadResult
Tab::checkSimpleFormatForBomAndNonAscii(ChaptersPtr const &chapters,
                                        QString const &fileName,
                                        bool append) {
  auto p      = p_func();
  auto result = Util::checkForBomAndNonAscii(fileName);

  if (   (byte_order_mark_e::none != result.byteOrderMark)
      || !result.containsNonAscii
      || !Util::Settings::get().m_ceTextFileCharacterSet.isEmpty())
    return { chapters, false };

  Util::enableChildren(this, false);

  p->originalFileName = fileName;
  auto dlg            = new SelectCharacterSetDialog{this, fileName};

  if (append)
    connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::appendSimpleChaptersWithCharacterSet);

  else {
    connect(dlg, &SelectCharacterSetDialog::characterSetSelected, this, &Tab::reloadSimpleChaptersWithCharacterSet);
    connect(dlg, &SelectCharacterSetDialog::rejected,             this, &Tab::closeTab);
  }

  dlg->show();

  return {};
}

Tab::LoadResult
Tab::loadFromChapterFile(QString const &fileName,
                         bool append) {
  auto p        = p_func();
  auto format   = mtx::chapters::format_e::xml;
  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    chapters = mtx::chapters::parse(to_utf8(fileName), 0, -1, 0, {}, to_utf8(Util::Settings::get().m_ceTextFileCharacterSet), true, &format);
    // log_it(ebml_dumper_c::dump_to_string(chapters.get()));

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::chapters::parser_x &ex) {
    error = Q(ex.what());
  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();

    if (!append)
      Q_EMIT removeThisTab();

  } else if (format != mtx::chapters::format_e::xml) {
    auto result = checkSimpleFormatForBomAndNonAscii(chapters, fileName, append);

    if (!append) {
      p->fileName.clear();
      Q_EMIT titleChanged();
    }

    return result;
  }

  return { chapters, format == mtx::chapters::format_e::xml };
}

void
Tab::appendSimpleChaptersWithCharacterSet(QString const &characterSet) {
  reloadOrAppendSimpleChaptersWithCharacterSet(characterSet, true);
}

void
Tab::reloadSimpleChaptersWithCharacterSet(QString const &characterSet) {
  reloadOrAppendSimpleChaptersWithCharacterSet(characterSet, false);
}

void
Tab::reloadOrAppendSimpleChaptersWithCharacterSet(QString const &characterSet,
                                                  bool append) {
  auto p     = p_func();
  auto error = QString{};

  try {
    auto chapters = mtx::chapters::parse(to_utf8(p->originalFileName), 0, -1, 0, {}, to_utf8(characterSet), true);

    if (!append)
      chaptersLoaded(chapters, false);
    else
      appendTheseChapters(chapters);

    Util::enableChildren(this, true);

    MainWindow::chapterEditorTool()->enableMenuActions();

    return;

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());
  } catch (mtx::chapters::parser_x &ex) {
    error = Q(ex.what());
  }

  Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(QY("Error message from the parser: %1").arg(error)).exec();

  Q_EMIT removeThisTab();
}

bool
Tab::areWidgetsEnabled()
  const {
  auto p = p_func();

  return p->ui->elements->isEnabled();
}

ChaptersPtr
Tab::mplsChaptersToMatroskaChapters(std::vector<mtx::bluray::mpls::chapter_t> const &mplsChapters)
  const {
  auto &cfg = Util::Settings::get();
  return mtx::chapters::convert_mpls_chapters_kax_chapters(mplsChapters, cfg.m_defaultChapterLanguage, to_utf8(cfg.m_chapterNameTemplate));
}

Tab::LoadResult
Tab::loadFromMplsFile(QString const &fileName,
                      bool append) {
  auto p        = p_func();
  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    mm_file_io_c in{to_utf8(fileName)};
    auto parser = ::mtx::bluray::mpls::parser_c{};

    parser.enable_dropping_last_entry_if_at_end(Util::Settings::get().m_dropLastChapterFromBlurayPlaylist);

    if (parser.parse(in))
      chapters = mplsChaptersToMatroskaChapters(parser.get_chapters());

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::bluray::mpls::exception &ex) {
    error = Q(ex.what());

  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    if (!append)
      Q_EMIT removeThisTab();

  } else if (!append) {
    p->fileName.clear();
    Q_EMIT titleChanged();
  }

  return { chapters, false };
}

Tab::LoadResult
Tab::loadFromDVD([[maybe_unused]] QString const &fileName,
                 [[maybe_unused]] bool append) {
#if defined(HAVE_DVDREAD)
  auto p            = p_func();
  auto &cfg         = Util::Settings::get();
  auto chapters     = ChaptersPtr{};
  auto error        = QString{};

  auto dvdDirectory = mtx::fs::to_path(fileName).parent_path();

  try {
    auto titlesAndTimestamps = mtx::chapters::parse_dvd(dvdDirectory.u8string());
    chapters                 = mtx::chapters::create_editions_and_chapters(titlesAndTimestamps, cfg.m_defaultChapterLanguage, to_utf8(cfg.m_chapterNameTemplate));

  } catch (mtx::chapters::parser_x const &ex) {
    error = Q(ex.what());
  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(message).exec();
    if (!append)
      Q_EMIT removeThisTab();

  } else if (!append) {
    p->fileName.clear();
    Q_EMIT titleChanged();
  }

  return { chapters, false };

#else  // HAVE_DVDREAD
  if (!append)
    Q_EMIT removeThisTab();

  return { {}, false };
#endif  // HAVE_DVDREAD
}

void
Tab::load() {
  auto p = p_func();

  resetData();

  p->savedState = currentState();
  auto result   = kax_analyzer_c::probe(to_utf8(p->fileName)) ? loadFromMatroskaFile(p->fileName, false)
                : p->fileName.toLower().endsWith(Q(".mpls"))  ? loadFromMplsFile(p->fileName, false)
                : p->fileName.toLower().endsWith(Q(".ifo"))   ? loadFromDVD(p->fileName, false)
                :                                               loadFromChapterFile(p->fileName, false);

  if (result.first)
    chaptersLoaded(result.first, result.second);
}

void
Tab::append(QString const &fileName) {
  auto result   = kax_analyzer_c::probe(to_utf8(fileName)) ? loadFromMatroskaFile(fileName, true)
                : fileName.toLower().endsWith(Q(".mpls"))  ? loadFromMplsFile(fileName, true)
                : fileName.toLower().endsWith(Q(".ifo"))   ? loadFromDVD(fileName, true)
                :                                            loadFromChapterFile(fileName, true);

  if (result.first)
    appendTheseChapters(result.first);
}

void
Tab::appendTheseChapters(ChaptersPtr const &chapters) {
  auto p = p_func();

  disconnect(p->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  p->chapterModel->populate(*chapters, true);

  expandAll();

  connect(p->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  MainWindow::chapterEditorTool()->enableMenuActions();
}

void
Tab::chaptersLoaded(ChaptersPtr const &chapters,
                    bool canBeWritten) {
  auto p = p_func();

  if (!p->fileName.isEmpty())
    p->fileModificationTime = QFileInfo{p->fileName}.lastModified();

  disconnect(p->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  p->chapterModel->reset();
  p->chapterModel->populate(*chapters, false);

  if (canBeWritten)
    p->savedState = currentState();

  expandAll();

  connect(p->chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  MainWindow::chapterEditorTool()->enableMenuActions();
}

void
Tab::save() {
  auto p = p_func();

  if (!p->analyzer)
    saveAsXmlImpl(false);

  else
    saveToMatroskaImpl(false);
}

void
Tab::saveAsImpl(bool requireNewFileName,
                std::function<bool(bool, QString &)> const &worker) {
  auto p = p_func();

  if (!copyControlsToStorage())
    return;

  p->chapterModel->fixMandatoryElements();
  setControlsFromStorage();

  auto newFileName = p->fileName;
  if (p->fileName.isEmpty())
    requireNewFileName = true;

  if (!worker(requireNewFileName, newFileName))
    return;

  p->savedState = currentState();

  if (newFileName != p->fileName) {
    p->fileName    = newFileName;

    auto &settings = Util::Settings::get();
    settings.m_lastOpenDir.setPath(QFileInfo{newFileName}.path());
    settings.save();

    updateFileNameDisplay();
    Q_EMIT titleChanged();
  }

  MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
}

void
Tab::saveAsXml() {
  saveAsXmlImpl(true);
}

void
Tab::saveAsXmlImpl(bool requireNewFileName) {
  auto p = p_func();

  saveAsImpl(requireNewFileName, [this, p](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (doRequireNewFileName) {
      auto defaultFilePath = !p->fileName.isEmpty() ? Util::dirPath(QFileInfo{p->fileName}.path()) : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getSaveFileName(this, QY("Save chapters as XML"), defaultFilePath, {}, QY("XML chapter files") + Q(" (*.xml);;") + QY("All files") + Q(" (*)"), Q("xml"));

      if (newFileName.isEmpty())
        return false;
    }

    try {
      auto chapters = p->chapterModel->allChapters();
      mm_file_io_c out{to_utf8(newFileName), MODE_CREATE};
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
  auto p = p_func();

  saveAsImpl(requireNewFileName, [this, p](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (!p->analyzer)
      doRequireNewFileName = true;

    if (doRequireNewFileName) {
      auto defaultFilePath = !p->fileName.isEmpty() ? QFileInfo{p->fileName}.path() : Util::Settings::get().lastOpenDirPath();
      newFileName          = Util::getSaveFileName(this, QY("Save chapters to Matroska or WebM file"), defaultFilePath, {},
                                                   QY("Supported file types") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;") +
                                                   QY("Matroska files")       + Q(" (*.mkv *.mka *.mks *.mk3d);;") +
                                                   QY("WebM files")           + Q(" (*.webm);;") +
                                                   QY("All files")            + Q(" (*)"),
                                                   {}, {}, QFileDialog::DontConfirmOverwrite, QFileDialog::ExistingFile);

      if (newFileName.isEmpty())
        return false;
    }

    if (doRequireNewFileName || (QFileInfo{newFileName}.lastModified() != p->fileModificationTime)) {
      p->analyzer = std::make_unique<Util::KaxAnalyzer>(this, newFileName);
      if (!p->analyzer->set_parse_mode(kax_analyzer_c::parse_mode_fast).process()) {
        auto text = Q("%1 %2")
          .arg(QY("The file you tried to open (%1) could not be read successfully.").arg(newFileName))
          .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file."));
        Util::MessageBox::critical(this)->title(QY("File parsing failed")).text(text).exec();
        return false;
      }

      p->fileName = newFileName;
    }

    auto chapters = p->chapterModel->allChapters();
    auto result   = kax_analyzer_c::uer_success;

    if (chapters && (0 != chapters->ListSize())) {
      fix_mandatory_elements(chapters.get());
      if (p->analyzer->is_webm())
        mtx::chapters::remove_elements_unsupported_by_webm(*chapters);

      remove_mandatory_elements_set_to_their_default(*chapters);

      result = p->analyzer->update_element(chapters, false, false);

    } else
      result = p->analyzer->remove_elements(EBML_ID(KaxChapters));

    p->analyzer->close_file();

    if (kax_analyzer_c::uer_success != result) {
      Util::KaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the chapters failed."));
      return false;
    }

    p->fileModificationTime = QFileInfo{p->fileName}.lastModified();

    return true;
  });
}

void
Tab::selectChapterRow(QModelIndex const &idx,
                      bool ignoreSelectionChanges) {
  auto p         = p_func();
  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), p->chapterModel->columnCount() - 1)};

  p->ignoreChapterSelectionChanges = ignoreSelectionChanges;
  p->ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  p->ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  p->ignoreChapterSelectionChanges = false;
}

bool
Tab::copyControlsToStorage() {
  auto p   = p_func();
  auto idx = Util::selectedRowIdx(p->ui->elements);
  return idx.isValid() ? copyControlsToStorage(idx) : true;
}

bool
Tab::copyControlsToStorage(QModelIndex const &idx) {
  auto p      = p_func();
  auto result = copyControlsToStorageImpl(idx);

  if (result.first) {
    p->chapterModel->updateRow(idx);
    return true;
  }

  selectChapterRow(idx, true);

  Util::MessageBox::critical(this)->title(QY("Validation failed")).text(result.second).exec();

  return false;
}

Tab::ValidationResult
Tab::copyControlsToStorageImpl(QModelIndex const &idx) {
  auto p       = p_func();
  auto stdItem = p->chapterModel->itemFromIndex(idx);

  if (!stdItem)
    return { true, QString{} };

  if (!idx.parent().isValid())
    return copyEditionControlsToStorage(p->chapterModel->editionFromItem(stdItem));

  return copyChapterControlsToStorage(p->chapterModel->chapterFromItem(stdItem));
}

QString
Tab::fixAndGetTimestampString(QLineEdit &lineEdit) {
  auto timestampText = lineEdit.text();

  if (timestampText.contains(Q(','))) {
    timestampText.replace(Q(','), Q('.'));
    lineEdit.setText(timestampText);
  }

  return timestampText;
}

Tab::ValidationResult
Tab::copyChapterControlsToStorage(ChapterPtr const &chapter) {
  auto p = p_func();

  if (!chapter)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!p->ui->leChUid->text().isEmpty()) {
    auto ok = false;
    uid  = p->ui->leChUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The chapter UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxChapterUID>(*chapter).SetValue(uid);
  else
    DeleteChildren<KaxChapterUID>(*chapter);

  if (!p->ui->cbChFlagEnabled->isChecked())
    GetChild<KaxChapterFlagEnabled>(*chapter).SetValue(0);
  else
    DeleteChildren<KaxChapterFlagEnabled>(*chapter);

  if (p->ui->cbChFlagHidden->isChecked())
    GetChild<KaxChapterFlagHidden>(*chapter).SetValue(1);
  else
    DeleteChildren<KaxChapterFlagHidden>(*chapter);

  auto startTimestamp = int64_t{};
  if (!mtx::string::parse_timestamp(to_utf8(fixAndGetTimestampString(*p->ui->leChStart)), startTimestamp))
    return { false, QY("The start time could not be parsed: %1").arg(Q(mtx::string::timestamp_parser_error)) };
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTimestamp);

  if (!p->ui->leChEnd->text().isEmpty()) {
    auto endTimestamp = int64_t{};
    if (!mtx::string::parse_timestamp(to_utf8(fixAndGetTimestampString(*p->ui->leChEnd)), endTimestamp))
      return { false, QY("The end time could not be parsed: %1").arg(Q(mtx::string::timestamp_parser_error)) };

    if (endTimestamp <= startTimestamp)
      return { false, QY("The end time must be greater than the start time.") };

    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(endTimestamp);

  } else
    DeleteChildren<KaxChapterTimeEnd>(*chapter);

  if (!p->ui->leChSegmentUid->text().isEmpty()) {
    try {
      auto value = mtx::bits::value_c{to_utf8(p->ui->leChSegmentUid->text())};
      GetChild<KaxChapterSegmentUID>(*chapter).CopyBuffer(value.data(), value.byte_size());

    } catch (mtx::bits::value_parser_x const &ex) {
      return { false, QY("The segment UID could not be parsed: %1").arg(ex.what()) };
    }

  } else
    DeleteChildren<KaxChapterSegmentUID>(*chapter);

  if (!p->ui->leChSegmentEditionUid->text().isEmpty()) {
    auto ok = false;
    uid     = p->ui->leChSegmentEditionUid->text().toULongLong(&ok);
    if (!ok || !uid)
      return { false, QY("The segment edition UID must be a positive number if given.") };

    GetChild<KaxChapterSegmentEditionUID>(*chapter).SetValue(uid);
  }

  RemoveChildren<KaxChapterDisplay>(*chapter);
  for (auto row = 0, numRows = p->nameModel->rowCount(); row < numRows; ++row)
    chapter->PushElement(*p->nameModel->displayFromIndex(p->nameModel->index(row, 0)));

  return { true, QString{} };
}

Tab::ValidationResult
Tab::copyEditionControlsToStorage(EditionPtr const &edition) {
  auto p = p_func();

  if (!edition)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!p->ui->leEdUid->text().isEmpty()) {
    auto ok = false;
    uid     = p->ui->leEdUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The edition UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxEditionUID>(*edition).SetValue(uid);
  else
    DeleteChildren<KaxEditionUID>(*edition);

  if (p->ui->cbEdFlagDefault->isChecked())
    GetChild<KaxEditionFlagDefault>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagDefault>(*edition);

  if (p->ui->cbEdFlagHidden->isChecked())
    GetChild<KaxEditionFlagHidden>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagHidden>(*edition);

  if (p->ui->cbEdFlagOrdered->isChecked())
    GetChild<KaxEditionFlagOrdered>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagOrdered>(*edition);

  return { true, QString{} };
}

bool
Tab::setControlsFromStorage() {
  auto p   = p_func();
  auto idx = Util::selectedRowIdx(p->ui->elements);

  return idx.isValid() ? setControlsFromStorage(idx) : true;
}

bool
Tab::setControlsFromStorage(QModelIndex const &idx) {
  auto p       = p_func();
  auto stdItem = p->chapterModel->itemFromIndex(idx);

  if (!stdItem)
    return false;

  if (!idx.parent().isValid())
    return setEditionControlsFromStorage(p->chapterModel->editionFromItem(stdItem));
  return setChapterControlsFromStorage(p->chapterModel->chapterFromItem(stdItem));
}

bool
Tab::setChapterControlsFromStorage(ChapterPtr const &chapter) {
  auto p = p_func();

  if (!chapter)
    return true;

  auto uid               = FindChildValue<KaxChapterUID>(*chapter);
  auto end               = FindChild<KaxChapterTimeEnd>(*chapter);
  auto segmentEditionUid = FindChild<KaxChapterSegmentEditionUID>(*chapter);

  p->ui->lChapter->setText(p->chapterModel->chapterDisplayName(*chapter));
  p->ui->leChStart->setText(Q(mtx::string::format_timestamp(FindChildValue<KaxChapterTimeStart>(*chapter))));
  p->ui->leChEnd->setText(end ? Q(mtx::string::format_timestamp(end->GetValue())) : Q(""));
  p->ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter, 1));
  p->ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  p->ui->leChUid->setText(uid ? QString::number(uid) : Q(""));
  p->ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  p->ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  auto nameSelectionModel        = p->ui->tvChNames->selectionModel();
  auto previouslySelectedNameIdx = nameSelectionModel->currentIndex();
  p->nameModel->populate(*chapter);
  enableNameWidgets(false);

  if (p->nameModel->rowCount()) {
    auto oldBlocked  = nameSelectionModel->blockSignals(true);
    auto rowToSelect = std::min(previouslySelectedNameIdx.isValid() ? previouslySelectedNameIdx.row() : 0, p->nameModel->rowCount());
    auto selection   = QItemSelection{ p->nameModel->index(rowToSelect, 0), p->nameModel->index(rowToSelect, p->nameModel->columnCount() - 1) };

    nameSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

    setNameControlsFromStorage(p->nameModel->index(rowToSelect, 0));
    enableNameWidgets(true);

    nameSelectionModel->blockSignals(oldBlocked);
  }

  p->ui->pageContainer->setCurrentWidget(p->ui->chapterPage);

  return true;
}

bool
Tab::setEditionControlsFromStorage(EditionPtr const &edition) {
  auto p = p_func();

  if (!edition)
    return true;

  auto uid = FindChildValue<KaxEditionUID>(*edition);

  p->ui->leEdUid->setText(uid ? QString::number(uid) : Q(""));
  p->ui->cbEdFlagDefault->setChecked(!!FindChildValue<KaxEditionFlagDefault>(*edition));
  p->ui->cbEdFlagHidden->setChecked(!!FindChildValue<KaxEditionFlagHidden>(*edition));
  p->ui->cbEdFlagOrdered->setChecked(!!FindChildValue<KaxEditionFlagOrdered>(*edition));

  p->ui->pageContainer->setCurrentWidget(p->ui->editionPage);

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
  auto p           = p_func();
  auto selectedIdx = QModelIndex{};

  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty())
      selectedIdx = indexes.at(0);
  }

  p->chapterModel->setSelectedIdx(selectedIdx);

  if (p->ignoreChapterSelectionChanges)
    return;

  if (!handleChapterDeselection(deselected))
    return;

  if (selectedIdx.isValid() && setControlsFromStorage(selectedIdx.sibling(selectedIdx.row(), 0)))
    return;

  p->ui->pageContainer->setCurrentWidget(p->ui->emptyPage);
}

bool
Tab::setNameControlsFromStorage(QModelIndex const &idx) {
  auto &p      = *p_func();
  auto display = p.nameModel->displayFromIndex(idx);

  if (!display)
    return false;

  // qDebug() << "setNameControlsFromStorage start";

  // log_it(ebml_dumper_c::dump_to_string(display));

  p.ignoreChapterNameChanges = true;

  p.ui->leChName->setText(Q(GetChildValue<KaxChapterString>(display)));

  auto lists             = NameModel::effectiveLanguagesForDisplay(*display);
  auto usedLanguageCodes = usedNameLanguages();

  std::sort(lists.languageCodes.begin(), lists.languageCodes.end(), [](auto const &a, auto const &b) { return a.format_long() < b.format_long(); });

  for (int languageIdx = 1, numLanguages = p.languageControls.size(); languageIdx < numLanguages; ++languageIdx) {
    auto &[layout, language, button] = p.languageControls[languageIdx];
    delete layout;
    delete language;
    delete button;
  }

  p.languageControls.remove(1, p.languageControls.size() - 1);

  p.ui->ldwChNameLanguage1->setAdditionalLanguages(usedLanguageCodes);
  p.ui->ldwChNameLanguage1->setLanguage(lists.languageCodes.isEmpty() ? mtx::bcp47::language_c{} : lists.languageCodes[0]);

  for (int languageIdx = 1, numLanguages = lists.languageCodes.size(); languageIdx < numLanguages; ++languageIdx)
    addOneChapterNameLanguage(lists.languageCodes[languageIdx], usedLanguageCodes);

  p.ignoreChapterNameChanges = false;

  // qDebug() << "setNameControlsFromStorage end";

  return true;
}

void
Tab::nameSelectionChanged(QItemSelection const &selected,
                          QItemSelection const &) {
  auto p = p_func();

  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setNameControlsFromStorage(indexes.at(0))) {
      enableNameWidgets(true);

      p->ui->leChName->selectAll();
      QTimer::singleShot(0, p->ui->leChName, [p]() { p->ui->leChName->setFocus(); });

      return;
    }
  }

  enableNameWidgets(false);
}

void
Tab::withSelectedName(std::function<void(QModelIndex const &, KaxChapterDisplay &)> const &worker) {
  auto p            = p_func();
  auto selectedRows = p->ui->tvChNames->selectionModel()->selectedRows();

  if (selectedRows.isEmpty())
    return;

  auto idx     = selectedRows.at(0);
  auto display = p->nameModel->displayFromIndex(idx);
  if (display)
    worker(idx, *display);
}

void
Tab::chapterNameEdited(QString const &text) {
  auto p = p_func();

  withSelectedName([p, &text](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterString>(display).SetValueUTF8(to_utf8(text));
    p->nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameLanguageChanged() {
  auto &p = *p_func();

  // qDebug() << "chapterNameLanguageChanged start";

  if (p.ignoreChapterNameChanges) {
    // qDebug() << "chapterNameLanguageChanged ignoring";
    return;
  }

  std::vector<mtx::bcp47::language_c> languageCodes;

  for (auto const &control : p.languageControls) {
    auto languageCode = static_cast<mtx::gui::Util::LanguageDisplayWidget *>(std::get<1>(control))->language();
    if (languageCode.is_valid())
      languageCodes.emplace_back(languageCode);
  }

  std::sort(languageCodes.begin(), languageCodes.end());

  withSelectedName([&p, &languageCodes](QModelIndex const &idx, KaxChapterDisplay &display) {
    mtx::chapters::set_languages_in_display(display, languageCodes);

    p.nameModel->updateRow(idx.row());
  });

  // qDebug() << "chapterNameLanguageChanged end";
}

void
Tab::addChapterName() {
  auto p = p_func();

  p->nameModel->addNew();
}

void
Tab::removeChapterName() {
  auto p   = p_func();
  auto idx = Util::selectedRowIdx(p->ui->tvChNames);

  if (idx.isValid())
    p->nameModel->remove(idx);
}

void
Tab::addChapterNameLanguage() {
  addOneChapterNameLanguage({}, usedNameLanguages());
}

void
Tab::addOneChapterNameLanguage(mtx::bcp47::language_c const &languageCode,
                               QStringList const &usedLanguageCodes) {
  auto &p           = *p_func();

  auto language     = new mtx::gui::Util::LanguageDisplayWidget{p.ui->wChNameLanguages};
  auto removeButton = new QPushButton{p.ui->wChNameLanguages};
  auto layout       = new QHBoxLayout;

  language->setAdditionalLanguages(usedLanguageCodes);
  language->setLanguage(languageCode);

  QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  language->setSizePolicy(sizePolicy);

  QIcon removeIcon;
  removeIcon.addFile(QString::fromUtf8(":/icons/16x16/list-remove.png"), QSize(), QIcon::Normal, QIcon::Off);

  removeButton->setIcon(removeIcon);

  layout->addWidget(language);
  layout->addWidget(removeButton);

  static_cast<QBoxLayout *>(p.ui->wChNameLanguages->layout())->addLayout(layout);

  p.languageControls.push_back({ layout, language, removeButton });

  connect(language,     &Util::LanguageDisplayWidget::languageChanged, this, &Tab::chapterNameLanguageChanged);
}

void
Tab::removeChapterNameLanguage() {
  auto &p = *p_func();

  if (removeLanguageControlRow(p.languageControls, sender()))
    chapterNameLanguageChanged();
}

void
Tab::enableNameWidgets(bool enable) {
  auto p = p_func();

  for (auto const &widget : p->nameWidgets)
    widget->setEnabled(enable);

  for (auto const &widget : p->ui->wChNameLanguages->findChildren<QWidget *>())
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
  auto p           = p_func();
  auto edition     = std::make_shared<KaxEditionEntry>();
  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);
  auto row         = 0;

  if (selectedIdx.isValid()) {
    while (selectedIdx.parent().isValid())
      selectedIdx = selectedIdx.parent();

    row = selectedIdx.row() + (before ? 0 : 1);
  }

  GetChild<KaxEditionUID>(*edition).SetValue(0);

  p->chapterModel->insertEdition(row, edition);

  Q_EMIT numberOfEntriesChanged();

  return p->chapterModel->index(row, 0);
}

void
Tab::addEditionOrChapterAfter() {
  auto p               = p_func();
  auto selectedIdx     = Util::selectedRowIdx(p->ui->elements);
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
                        std::optional<QString> const &nameTemplate,
                        mtx::bcp47::language_c const &language) {
  auto &cfg    = Util::Settings::get();
  auto chapter = std::make_shared<KaxChapterAtom>();
  auto name    = formatChapterName(nameTemplate ? *nameTemplate : cfg.m_chapterNameTemplate, chapterNumber, timestamp_c::ns(startTime));

  GetChild<KaxChapterUID>(*chapter).SetValue(0);
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTime);
  if (!name.isEmpty()) {
    auto &display        = GetChild<KaxChapterDisplay>(*chapter);
    auto actual_language = language.is_valid() ? language : cfg.m_defaultChapterLanguage;

    GetChild<KaxChapterString>(display).SetValue(to_wide(name));
    mtx::chapters::set_languages_in_display(display, actual_language);
  }

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
  auto p           = p_func();
  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);

  if (!selectedIdx.isValid() || !selectedIdx.parent().isValid())
    return {};

  // TODO: Tab::addChapter: start time
  auto row     = selectedIdx.row() + (before ? 0 : 1);
  auto chapter = createEmptyChapter(0, row + 1);

  p->chapterModel->insertChapter(row, chapter, selectedIdx.parent());

  Q_EMIT numberOfEntriesChanged();

  return p->chapterModel->index(row, 0, selectedIdx.parent());
}

void
Tab::addSubChapter() {
  auto p           = p_func();
  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);

  if (!selectedIdx.isValid())
    return;

  // TODO: Tab::addSubChapter: start time
  auto selectedItem = p->chapterModel->itemFromIndex(selectedIdx);
  auto chapter      = createEmptyChapter(0, (selectedItem ? selectedItem->rowCount() : 0) + 1);

  p->chapterModel->appendChapter(chapter, selectedIdx);
  expandCollapseAll(true, selectedIdx);

  Q_EMIT numberOfEntriesChanged();
}

void
Tab::removeElement() {
  auto p = p_func();

  p->chapterModel->removeTree(Util::selectedRowIdx(p->ui->elements));
  Q_EMIT numberOfEntriesChanged();
}

void
Tab::applyModificationToTimestamps(QStandardItem *item,
                                   std::function<int64_t(int64_t)> const &unaryOp) {
  auto p = p_func();

  if (!item)
    return;

  if (item->parent()) {
    auto chapter = p->chapterModel->chapterFromItem(item);
    if (chapter) {
      auto kStart = FindChild<KaxChapterTimeStart>(*chapter);
      auto kEnd   = FindChild<KaxChapterTimeEnd>(*chapter);

      if (kStart)
        kStart->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kStart->GetValue())), 0));
      if (kEnd)
        kEnd->SetValue(std::max<int64_t>(unaryOp(static_cast<int64_t>(kEnd->GetValue())), 0));

      if (kStart || kEnd)
        p->chapterModel->updateRow(item->index());
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
                         std::optional<uint64_t> const &constrictStart,
                         std::optional<uint64_t> const &constrictEnd) {
  auto p = p_func();

  if (!item)
    return;

  auto chapter = item->parent() ? p->chapterModel->chapterFromItem(item) : ChapterPtr{};
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
  auto newEnd   = !kEnd           ? std::optional<uint64_t>{}
                : !constrictEnd   ? std::max(newStart, kEnd->GetValue())
                :                   std::max(newStart, std::min(*constrictEnd, kEnd->GetValue()));

  kStart->SetValue(newStart);
  if (newEnd)
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);

  p->chapterModel->updateRow(item->index());

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    constrictTimestamps(item->child(row), newStart, newEnd);
}

std::pair<std::optional<uint64_t>, std::optional<uint64_t>>
Tab::expandTimestamps(QStandardItem *item) {
  auto p = p_func();

  if (!item)
    return {};

  auto chapter = item->parent() ? p->chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      expandTimestamps(item->child(row));
    return {};
  }

  auto kStart   = chapter ? FindChild<KaxChapterTimeStart>(*chapter)    : nullptr;
  auto kEnd     = chapter ? FindChild<KaxChapterTimeEnd>(*chapter)      : nullptr;
  auto newStart = kStart  ? std::optional<uint64_t>{kStart->GetValue()} : std::optional<uint64_t>{};
  auto newEnd   = kEnd    ? std::optional<uint64_t>{kEnd->GetValue()}   : std::optional<uint64_t>{};
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
    p->chapterModel->updateRow(item->index());

  return std::make_pair(newStart, newEnd);
}

void
Tab::setLanguages(QStandardItem *item,
                  mtx::bcp47::language_c const &language) {
  auto p = p_func();

  if (!item)
    return;

  auto chapter = p->chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (kDisplay)
        mtx::chapters::set_languages_in_display(*kDisplay, language);
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setLanguages(item->child(row), language);
}

void
Tab::setEndTimestamps(QStandardItem *startItem) {
  auto p = p_func();

  if (!startItem)
    return;

  auto allAtomData = collectChapterAtomDataForEdition(startItem);

  std::function<void(QStandardItem *)> setter = [p, &allAtomData, &setter](QStandardItem *item) {
    if (item->parent()) {
      auto chapter = p->chapterModel->chapterFromItem(item);
      if (chapter) {
        auto data = allAtomData[chapter.get()];
        if (data && data->calculatedEnd.valid()) {
          GetChild<KaxChapterTimeEnd>(*chapter).SetValue(data->calculatedEnd.to_ns());
          p->chapterModel->updateRow(item->index());
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
Tab::removeEndTimestamps(QStandardItem *startItem) {
  auto chapterModel = p_func()->chapterModel;

  if (!startItem)
    return;

  std::function<void(QStandardItem *)> setter = [chapterModel, &setter](QStandardItem *item) {
    auto chapter = chapterModel->chapterFromItem(item);
    if (chapter) {
      DeleteChildren<KaxChapterTimeEnd>(*chapter);
      chapterModel->updateRow(item->index());
    }

    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      setter(item->child(row));
  };

  setter(startItem);

  setControlsFromStorage();
}

void
Tab::removeNames(QStandardItem *startItem) {
  auto chapterModel = p_func()->chapterModel;

  if (!startItem)
    return;

  std::function<void(QStandardItem *)> worker = [chapterModel, &worker](QStandardItem *item) {
    auto chapter = chapterModel->chapterFromItem(item);
    if (chapter) {
      DeleteChildren<KaxChapterDisplay>(*chapter);
      chapterModel->updateRow(item->index());
    }

    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      worker(item->child(row));
  };

  worker(startItem);

  setControlsFromStorage();
}

void
Tab::massModify() {
  auto p = p_func();

  if (!copyControlsToStorage())
    return;

  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);
  auto item        = selectedIdx.isValid() ? p->chapterModel->itemFromIndex(selectedIdx) : p->chapterModel->invisibleRootItem();

  MassModificationDialog dlg{this, selectedIdx.isValid(), usedNameLanguages()};
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

  if (actions & MassModificationDialog::Sort)
    item->sortChildren(1);

  if (actions & MassModificationDialog::SetEndTimestamps)
    setEndTimestamps(item);

  if (actions & MassModificationDialog::RemoveEndTimestamps)
    removeEndTimestamps(item);

  if (actions & MassModificationDialog::RemoveNames)
    removeNames(item);

  setControlsFromStorage();
}

void
Tab::duplicateElement() {
  auto p             = p_func();
  auto selectedIdx   = Util::selectedRowIdx(p->ui->elements);
  auto newElementIdx = p->chapterModel->duplicateTree(selectedIdx);

  if (newElementIdx.isValid())
    expandCollapseAll(true, newElementIdx);

  Q_EMIT numberOfEntriesChanged();
}

void
Tab::copyElementToOtherTab() {
  auto p             = p_func();
  auto selectedIdx   = Util::selectedRowIdx(p->ui->elements);
  auto tabIdx        = static_cast<QAction *>(sender())->data().toInt();
  auto otherTabs     = MainWindow::chapterEditorTool()->tabs();

  otherTabs.removeOne(this);

  if (   !selectedIdx.isValid()
      || (tabIdx < 0)
      || (tabIdx > otherTabs.size()))
    return;

  auto newChapters  = p->chapterModel->cloneSubtreeForRetrieval(selectedIdx);

  otherTabs[tabIdx]->appendTheseChapters(newChapters);
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
  auto p           = p_func();
  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);

  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = p->chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = p->chapterModel->chapterFromItem(selectedItem);
  auto maxEndTimestamp = selectedChapter ? FindChildValue<KaxChapterTimeStart>(*selectedChapter, 0ull) : 0ull;
  auto numRows         = selectedItem->rowCount();

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = p->chapterModel->chapterFromItem(selectedItem->child(row));
    if (chapter)
      maxEndTimestamp = std::max(maxEndTimestamp, std::max(FindChildValue<KaxChapterTimeStart>(*chapter, 0ull), FindChildValue<KaxChapterTimeEnd>(*chapter, 0ull)));
  }

  GenerateSubChaptersParametersDialog dlg{this, numRows + 1, maxEndTimestamp, usedNameLanguages()};
  if (!dlg.exec())
    return;

  auto toCreate      = dlg.numberOfEntries();
  auto chapterNumber = dlg.firstChapterNumber();
  auto timestamp     = dlg.startTimestamp();
  auto duration      = dlg.durationInNs();
  auto nameTemplate  = dlg.nameTemplate();
  auto language      = dlg.language();

  while (toCreate > 0) {
    auto chapter = createEmptyChapter(timestamp, chapterNumber, nameTemplate, language);
    timestamp   += duration;

    ++chapterNumber;
    --toCreate;

    p->chapterModel->appendChapter(chapter, selectedIdx);
  }

  expandCollapseAll(true, selectedIdx);

  Q_EMIT numberOfEntriesChanged();
}

bool
Tab::changeChapterName(QModelIndex const &parentIdx,
                       int row,
                       int chapterNumber,
                       QString const &nameTemplate,
                       RenumberSubChaptersParametersDialog::NameMatch nameMatchingMode,
                       mtx::bcp47::language_c const &languageOfNamesToReplace,
                       bool skipHidden) {
  auto p       = p_func();
  auto idx     = p->chapterModel->index(row, 0, parentIdx);
  auto item    = p->chapterModel->itemFromIndex(idx);
  auto chapter = p->chapterModel->chapterFromItem(item);

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
    p->chapterModel->updateRow(idx);

    return true;
  }

  for (auto const &element : *chapter) {
    auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
    if (!kDisplay)
      continue;

    auto lists = NameModel::effectiveLanguagesForDisplay(*kDisplay);

    if (   (RenumberSubChaptersParametersDialog::NameMatch::All == nameMatchingMode)
        || (std::find_if(lists.languageCodes.begin(), lists.languageCodes.end(), [&languageOfNamesToReplace](auto const &actualLanguage) {
              return (languageOfNamesToReplace == actualLanguage);
            }) != lists.languageCodes.end()))
      GetChild<KaxChapterString>(*kDisplay).SetValue(name);
  }

  p->chapterModel->updateRow(idx);

  return true;
}

void
Tab::renumberSubChapters() {
  auto p           = p_func();
  auto selectedIdx = Util::selectedRowIdx(p->ui->elements);

  if (!selectedIdx.isValid())
    return;

  if (!copyControlsToStorage())
    return;

  auto selectedItem    = p->chapterModel->itemFromIndex(selectedIdx);
  auto selectedChapter = p->chapterModel->chapterFromItem(selectedItem);
  auto numRows         = selectedItem->rowCount();
  auto chapterTitles   = QStringList{};
  auto firstName       = QString{};

  for (auto row = 0; row < numRows; ++row) {
    auto chapter = p->chapterModel->chapterFromItem(selectedItem->child(row));
    if (!chapter)
      continue;

    auto start = GetChild<KaxChapterTimeStart>(*chapter).GetValue();
    auto end   = FindChild<KaxChapterTimeEnd>(*chapter);
    auto name  = ChapterModel::chapterDisplayName(*chapter);

    if (firstName.isEmpty())
      firstName = name;

    if (end)
      chapterTitles << Q("%1 (%2 – %3)").arg(name).arg(Q(mtx::string::format_timestamp(start))).arg(Q(mtx::string::format_timestamp(end->GetValue())));
    else
      chapterTitles << Q("%1 (%2)").arg(name).arg(Q(mtx::string::format_timestamp(start)));
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
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  Util::expandCollapseAll(p_func()->ui->elements, expand, parentIdx);
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
      value += fmt::format("{0:02x}", static_cast<unsigned int>(*data));

  return Q(value);
}

void
Tab::setupCopyToOtherTabMenu() {
  auto p         = p_func();
  auto idx       = 0;
  auto otherTabs = MainWindow::chapterEditorTool()->tabs();

  otherTabs.removeOne(this);
  p->copyToOtherTabMenu->setEnabled(!otherTabs.isEmpty());

  p->copyToOtherTabMenu->clear();

  for (auto const &otherTab : otherTabs) {
    auto action = new QAction{p->copyToOtherTabMenu};
    action->setText(otherTab->title());
    action->setData(idx);

    p->copyToOtherTabMenu->addAction(action);

    connect(action, &QAction::triggered, this, &Tab::copyElementToOtherTab);

    ++idx;
  }
}

void
Tab::showChapterContextMenu(QPoint const &pos) {
  auto p               = p_func();
  auto selectedIdx     = Util::selectedRowIdx(p->ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();
  auto hasEntries      = !!p->chapterModel->rowCount();
  auto hasSubEntries   = selectedIdx.isValid() ? !!p->chapterModel->rowCount(selectedIdx) : false;

  p->addChapterBeforeAction->setEnabled(chapterSelected);
  p->addChapterAfterAction->setEnabled(chapterSelected);
  p->addSubChapterAction->setEnabled(hasSelection);
  p->generateSubChaptersAction->setEnabled(hasSelection);
  p->renumberSubChaptersAction->setEnabled(hasSelection && hasSubEntries);
  p->removeElementAction->setEnabled(hasSelection);
  p->duplicateAction->setEnabled(hasSelection);
  p->expandAllAction->setEnabled(hasEntries);
  p->collapseAllAction->setEnabled(hasEntries);

  setupCopyToOtherTabMenu();

  QMenu menu{this};

  menu.addAction(p->addEditionBeforeAction);
  menu.addAction(p->addEditionAfterAction);
  menu.addSeparator();
  menu.addAction(p->addChapterBeforeAction);
  menu.addAction(p->addChapterAfterAction);
  menu.addAction(p->addSubChapterAction);
  menu.addAction(p->generateSubChaptersAction);
  menu.addSeparator();
  menu.addAction(p->duplicateAction);
  menu.addMenu(p->copyToOtherTabMenu);
  menu.addSeparator();
  menu.addAction(p->removeElementAction);
  menu.addSeparator();
  menu.addAction(p->renumberSubChaptersAction);
  menu.addAction(p->massModificationAction);
  menu.addSeparator();
  menu.addAction(p->expandAllAction);
  menu.addAction(p->collapseAllAction);

  menu.exec(p->ui->elements->viewport()->mapToGlobal(pos));
}

bool
Tab::isSourceMatroska()
  const {
  auto p = p_func();

  return !!p->analyzer;
}

bool
Tab::hasChapters()
  const {
  auto p = p_func();

  for (auto idx = 0, numEditions = p->chapterModel->rowCount(); idx < numEditions; ++idx)
    if (p->chapterModel->item(idx)->rowCount())
      return true;
  return false;
}

QString
Tab::currentState()
  const {
  auto p        = p_func();
  auto chapters = p->chapterModel->allChapters();

  return chapters ? Q(ebml_dumper_c::dump_to_string(chapters.get(), static_cast<ebml_dumper_c::dump_style_e>(ebml_dumper_c::style_with_values | ebml_dumper_c::style_with_indexes))) : QString{};
}

bool
Tab::hasBeenModified()
  const {
  auto p = p_func();

  return currentState() != p->savedState;
}

bool
Tab::focusNextChapterName() {
  auto p            = p_func();
  auto selectedRows = p->ui->tvChNames->selectionModel()->selectedRows();

  if (selectedRows.isEmpty())
    return false;

  auto nextRow = selectedRows.at(0).row() + 1;
  if (nextRow >= p->nameModel->rowCount())
    return false;

  Util::selectRow(p->ui->tvChNames, nextRow);

  return true;
}

bool
Tab::focusNextChapterAtom(FocusElementType toFocus) {
  auto p        = p_func();
  auto doSelect = [p, toFocus](QModelIndex const &idx) -> bool {
    Util::selectRow(p->ui->elements, idx.row(), idx.parent());

    auto lineEdit = FocusChapterStartTime == toFocus ? p->ui->leChStart : p->ui->leChName;
    lineEdit->selectAll();
    lineEdit->setFocus();

    return true;
  };

  auto selectedRows = p->ui->elements->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return false;

  auto selectedIdx  = selectedRows.at(0);
  selectedIdx       = selectedIdx.sibling(selectedIdx.row(), 0);
  auto selectedItem = p->chapterModel->itemFromIndex(selectedIdx);

  if (selectedItem->rowCount())
    return doSelect(p->chapterModel->index(0, 0, selectedIdx));

  auto parentIdx  = selectedIdx.parent();
  auto parentItem = p->chapterModel->itemFromIndex(parentIdx);
  auto nextRow    = selectedIdx.row() + 1;

  if (nextRow < parentItem->rowCount())
    return doSelect(p->chapterModel->index(nextRow, 0, parentIdx));

  while (parentIdx.parent().isValid()) {
    nextRow    = parentIdx.row() + 1;
    parentIdx  = parentIdx.parent();
    parentItem = p->chapterModel->itemFromIndex(parentIdx);

    if (nextRow < parentItem->rowCount())
      return doSelect(p->chapterModel->index(nextRow, 0, parentIdx));
  }

  auto numEditions = p->chapterModel->rowCount();
  auto editionIdx  = parentIdx.sibling((parentIdx.row() + 1) % numEditions, 0);

  while (numEditions) {
    if (p->chapterModel->itemFromIndex(editionIdx)->rowCount())
      return doSelect(p->chapterModel->index(0, 0, editionIdx));

    editionIdx = editionIdx.sibling((editionIdx.row() + 1) % numEditions, 0);
  }

  return false;
}

void
Tab::focusNextChapterElement(bool keepSameElement) {
  auto p = p_func();

  if (!copyControlsToStorage())
    return;

  if (QObject::sender() == p->ui->leChName) {
    if (focusNextChapterName())
      return;

    focusNextChapterAtom(keepSameElement ? FocusChapterName : FocusChapterStartTime);
    return;
  }

  if (!keepSameElement) {
    p->ui->leChName->selectAll();
    p->ui->leChName->setFocus();
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
  Q_EMIT removeThisTab();
}

void
Tab::addSegmentUIDFromFile() {
  auto p = p_func();

  Util::addSegmentUIDFromFileToLineEdit(*this, *p->ui->leChSegmentUid, false);
}

QStringList
Tab::usedNameLanguages(QStandardItem *rootItem) {
  auto p = p_func();

  if (!rootItem)
    rootItem = p->chapterModel->invisibleRootItem();

  auto languages = QSet<QString>{};

  std::function<void(QStandardItem *)> collector = [p, &collector, &languages](auto *currentItem) {
    if (!currentItem)
      return;

    auto chapter = p->chapterModel->chapterFromItem(currentItem);
    if (chapter)
      for (auto const &element : *chapter) {
        auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
        if (!kDisplay)
          continue;

        auto lists = NameModel::effectiveLanguagesForDisplay(*static_cast<libmatroska::KaxChapterDisplay *>(kDisplay));
        for (auto const &languageCodeHere : lists.languageCodes)
          languages << Q(languageCodeHere.format());
      }

    for (int row = 0, numRows = currentItem->rowCount(); row < numRows; ++row)
      collector(currentItem->child(row));
  };

  collector(rootItem);

  return languages.values();
}

QHash<KaxChapterAtom *, ChapterAtomDataPtr>
Tab::collectChapterAtomDataForEdition(QStandardItem *item) {
  auto p = p_func();

  if (!item)
    return {};

  QHash<KaxChapterAtom *, ChapterAtomDataPtr> allAtoms;
  QHash<KaxChapterAtom *, QVector<ChapterAtomDataPtr>> atomsByParent;
  QVector<ChapterAtomDataPtr> atomList;

  // Collect all existing start and end timestamps.
  std::function<void(QStandardItem *, KaxChapterAtom *, int)> collector = [p, &collector, &allAtoms, &atomsByParent, &atomList](auto *currentItem, auto *parentAtom, int level) {
    if (!currentItem)
      return;

    QVector<ChapterAtomDataPtr> currentAtoms;

    auto chapter = p->chapterModel->chapterFromItem(currentItem);
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

  std::function<void(QStandardItem *)> calculator = [p, &calculator, &allAtoms, &atomsByParent](auto *parentItem) {
    if (!parentItem || !parentItem->rowCount())
      return;

    auto parentAtom         = p->chapterModel->chapterFromItem(parentItem);
    auto parentData         = allAtoms[parentAtom.get()];
    auto &sortedData        = atomsByParent[parentAtom.get()];
    auto parentEndTimestamp = parentData ? parentData->calculatedEnd : p->fileEndTimestamp;

    for (int row = 0, numRows = parentItem->rowCount(); row < numRows; ++row) {
      auto atom = p->chapterModel->chapterFromItem(parentItem->child(row));
      auto data = allAtoms[atom.get()];

      if (!data)
        continue;

      auto itr            = std::lower_bound(sortedData.begin(), sortedData.end(), data, [](auto const &a, auto const &b) { return a->start <= b->start; });
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
    std::sort(atomsByParent[key].begin(), atomsByParent[key].end(), [](ChapterAtomDataPtr const &a, ChapterAtomDataPtr const &b) { return a->start < b->start; });

  // Calculate end timestamps.
  calculator(item);

  // Output debug info.
  for (auto const &data : atomList)
    qDebug() <<
      Q(fmt::format("collectChapterAtomData: data {0}{1} start {2} end {3} [{4}] atom {5} parent {6}",
                    std::string(data->level * 2, ' '),
                    to_utf8(data->primaryName),
                    data->start,
                    data->end,
                    data->calculatedEnd,
                    static_cast<void *>(data->atom),
                    static_cast<void *>(data->parentAtom)));

  return allAtoms;
}

}
