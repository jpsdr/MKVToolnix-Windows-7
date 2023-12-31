#pragma once

#include "common/common_pch.h"

#include <QDateTime>
#include <QModelIndex>

#include "common/bcp47.h"
#include "common/qt.h"
#include "common/timestamp.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"
#include "mkvtoolnix-gui/chapter_editor/renumber_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/util/kax_analyzer.h"
#include "mkvtoolnix-gui/types.h"

class QComboBox;
class QItemSelection;
class QLineEdit;

namespace libebml {
class EbmlBinary;
}

namespace mtx::bcp47 {
class language_c;
}

namespace mtx::bluray::mpls {
struct chapter_t;
}

namespace mtx::gui::ChapterEditor {

namespace Ui {
class Tab;
}

class ChapterModel;
class NameModel;
class TabPrivate;

struct ChapterAtomData {
  libmatroska::KaxChapterAtom *atom, *parentAtom;
  timestamp_c start, end, calculatedEnd;
  QString primaryName;
  int level;

  ChapterAtomData()
    : atom{}
    , parentAtom{}
    , level{}
  {
  }
};
using ChapterAtomDataPtr = std::shared_ptr<ChapterAtomData>;

class Tab : public QWidget {
  Q_OBJECT

protected:
  enum FocusElementType {
    FocusChapterName,
    FocusChapterStartTime,
  };

  using ValidationResult = std::pair<bool, QString>;
  using LoadResult       = std::pair<ChaptersPtr, bool>;

protected:
  MTX_DECLARE_PRIVATE(TabPrivate)

  std::unique_ptr<TabPrivate> const p_ptr;

  explicit Tab(QWidget *parent, TabPrivate &d);

public:
  explicit Tab(QWidget *parent, QString const &fileName = QString{});
  virtual ~Tab();

  virtual void retranslateUi();
  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual bool hasChapters() const;
  virtual bool hasBeenModified() const;
  virtual bool areWidgetsEnabled() const;
  virtual bool isSourceMatroska() const;

Q_SIGNALS:
  void removeThisTab();
  void titleChanged();
  void numberOfEntriesChanged();

public Q_SLOTS:
  virtual void newFile();
  virtual void load();
  virtual void append(QString const &fileName);
  virtual void appendSimpleChaptersWithCharacterSet(QString const &characterSet);
  virtual void reloadSimpleChaptersWithCharacterSet(QString const &characterSet);
  virtual void save();
  virtual void saveAsXml();
  virtual void saveToMatroska();
  virtual void expandAll();
  virtual void collapseAll();
  virtual void addEditionBefore();
  virtual void addEditionAfter();
  virtual void addChapterBefore();
  virtual void addChapterAfter();
  virtual void addSubChapter();
  virtual void addEditionOrChapterAfter();
  virtual void removeElement();
  virtual void duplicateElement();
  virtual void copyElementToOtherTab();
  virtual void massModify();
  virtual void generateSubChapters();
  virtual void renumberSubChapters();
  virtual void addSegmentUIDFromFile();

  virtual void chapterSelectionChanged(QItemSelection const &selected, QItemSelection const &deselected);
  virtual void expandInsertedElements(QModelIndex const &parentIdx, int start, int end);

  virtual void nameSelectionChanged(QItemSelection const &selected, QItemSelection const &deselected);
  virtual void chapterNameEdited(QString const &text);
  virtual void chapterNameLanguageChanged();
  virtual void addChapterName();
  virtual void addChapterNameLanguage();
  virtual void removeChapterName();
  virtual void removeChapterNameLanguage();

  virtual void showChapterContextMenu(QPoint const &pos);

  virtual void focusOtherControlInNextChapterElement();
  virtual void focusSameControlInNextChapterElement();

  virtual void closeTab();

protected:
  void setup();
  void setupUi();
  void resetData();
  void expandCollapseAll(bool expand, QModelIndex const &parentIdx = {});

  LoadResult loadFromChapterFile(QString const &fileName, bool append);
  LoadResult loadFromDVD(QString const &fileName, bool append);
  LoadResult loadFromMatroskaFile(QString const &fileName, bool append);
  LoadResult loadFromMplsFile(QString const &fileName, bool append);
  LoadResult checkSimpleFormatForBomAndNonAscii(ChaptersPtr const &chapters, QString const &fileName, bool append);
  void reloadOrAppendSimpleChaptersWithCharacterSet(QString const &characterSet, bool append);

  bool readFileEndTimestampForMatroska(kax_analyzer_c &analyzer);

  void resizeChapterColumnsToContents() const;
  void resizeNameColumnsToContents() const;

  bool copyControlsToStorage();
  bool copyControlsToStorage(QModelIndex const &idx);
  ValidationResult copyControlsToStorageImpl(QModelIndex const &idx);
  ValidationResult copyChapterControlsToStorage(ChapterPtr const &chapter);
  ValidationResult copyEditionControlsToStorage(EditionPtr const &edition);

  bool setControlsFromStorage();
  bool setControlsFromStorage(QModelIndex const &idx);
  bool setChapterControlsFromStorage(ChapterPtr const &chapter);
  bool setEditionControlsFromStorage(EditionPtr const &edition);

  bool setNameControlsFromStorage(QModelIndex const &idx);
  void enableNameWidgets(bool enable);

  void withSelectedName(std::function<void(QModelIndex const &, libmatroska::KaxChapterDisplay &)> const &worker);

  void selectChapterRow(QModelIndex const &idx, bool ignoreSelectionChanges);
  bool handleChapterDeselection(QItemSelection const &deselected);

  QModelIndex addEdition(bool before);
  QModelIndex addChapter(bool before);

  ChapterPtr createEmptyChapter(int64_t startTime, int chapterNumber, std::optional<QString> const &nameTemplate = {}, mtx::bcp47::language_c const &language = {});

  void saveAsImpl(bool requireNewFileName, std::function<bool(bool, QString &)> const &worker);
  void saveAsXmlImpl(bool requireNewFileName);
  void saveToMatroskaImpl(bool requireNewFileName);
  void updateFileNameDisplay();

  void applyModificationToTimestamps(QStandardItem *item, std::function<int64_t(int64_t)> const &unaryOp);
  void multiplyTimestamps(QStandardItem *item, double factor);
  void shiftTimestamps(QStandardItem *item, int64_t delta);
  void constrictTimestamps(QStandardItem *item, std::optional<uint64_t> const &constrictStart, std::optional<uint64_t> const &constrictEnd);
  std::pair<std::optional<uint64_t>, std::optional<uint64_t>> expandTimestamps(QStandardItem *item);
  void setLanguages(QStandardItem *item, mtx::bcp47::language_c const &language);
  void setEndTimestamps(QStandardItem *startItem);
  void removeEndTimestamps(QStandardItem *startItem);
  void removeNames(QStandardItem *startItem);

protected:
  void setupToolTips();
  void setupCopyToOtherTabMenu();

  void chaptersLoaded(ChaptersPtr const &chapters, bool canBeWritten);
  void appendTheseChapters(ChaptersPtr const &chapters);

  QString currentState() const;
  QStringList usedNameLanguages(QStandardItem *parentItem = nullptr);
  ChaptersPtr mplsChaptersToMatroskaChapters(std::vector<mtx::bluray::mpls::chapter_t> const &mplsChapters) const;
  QHash<libmatroska::KaxChapterAtom *, ChapterAtomDataPtr> collectChapterAtomDataForEdition(QStandardItem *item);
  QString formatChapterName(QString const &nameTemplate, int chapterNumber, timestamp_c const &startTimestamp) const;
  bool changeChapterName(QModelIndex const &parentIdx, int row, int chapterNumber, QString const &nameTemplate, RenumberSubChaptersParametersDialog::NameMatch nameMatchingMode,
                         mtx::bcp47::language_c const &languageOfNamesToReplace, bool skipHidden);

  bool focusNextChapterAtom(FocusElementType toFocus);
  bool focusNextChapterName();
  void focusNextChapterElement(bool keepSameControl);

  void addOneChapterNameLanguage(mtx::bcp47::language_c const &languageCode, QStringList const &usedLanguageCodes);

  QString fixAndGetTimestampString(QLineEdit &lineEdit);

  static QString formatEbmlBinary(libebml::EbmlBinary *binary);
};

}
