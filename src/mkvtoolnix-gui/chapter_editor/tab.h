#pragma once

#include "common/common_pch.h"

#include <QDateTime>
#include <QModelIndex>

#include "common/qt_kax_analyzer.h"
#include "common/timestamp.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"
#include "mkvtoolnix-gui/chapter_editor/renumber_sub_chapters_parameters_dialog.h"
#include "mkvtoolnix-gui/types.h"

class QItemSelection;

namespace libebml {
class EbmlBinary;
};

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class Tab;
}

class ChapterModel;
class NameModel;
class TabPrivate;

struct ChapterAtomData {
  KaxChapterAtom *atom, *parentAtom;
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
  Q_OBJECT;

protected:
  enum FocusElementType {
    FocusChapterName,
    FocusChapterStartTime,
  };

  using ValidationResult = std::pair<bool, QString>;
  using LoadResult       = std::pair<ChaptersPtr, bool>;

protected:
  Q_DECLARE_PRIVATE(Tab);

  QScopedPointer<TabPrivate> const d_ptr;

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

signals:
  void removeThisTab();
  void titleChanged();
  void numberOfEntriesChanged();

public slots:
  virtual void newFile();
  virtual void load();
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
  virtual void massModify();
  virtual void generateSubChapters();
  virtual void renumberSubChapters();
  virtual void addSegmentUIDFromFile();

  virtual void chapterSelectionChanged(QItemSelection const &selected, QItemSelection const &deselected);
  virtual void expandInsertedElements(QModelIndex const &parentIdx, int start, int end);

  virtual void nameSelectionChanged(QItemSelection const &selected, QItemSelection const &deselected);
  virtual void chapterNameEdited(QString const &text);
  virtual void chapterNameLanguageChanged(int index);
  virtual void chapterNameCountryChanged(int index);
  virtual void addChapterName();
  virtual void removeChapterName();

  virtual void showChapterContextMenu(QPoint const &pos);

  virtual void focusOtherControlInNextChapterElement();
  virtual void focusSameControlInNextChapterElement();

  virtual void closeTab();

protected:
  void setup();
  void setupUi();
  void resetData();
  void expandCollapseAll(bool expand, QModelIndex const &parentIdx = {});

  LoadResult loadFromChapterFile();
  LoadResult loadFromMatroskaFile();
  LoadResult loadFromMplsFile();
  LoadResult checkSimpleFormatForBomAndNonAscii(ChaptersPtr const &chapters);

  bool readFileEndTimestampForMatroska();

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

  void withSelectedName(std::function<void(QModelIndex const &, KaxChapterDisplay &)> const &worker);

  void selectChapterRow(QModelIndex const &idx, bool ignoreSelectionChanges);
  bool handleChapterDeselection(QItemSelection const &deselected);

  QModelIndex addEdition(bool before);
  QModelIndex addChapter(bool before);

  ChapterPtr createEmptyChapter(int64_t startTime, int chapterNumber, OptQString const &nameTemplate = OptQString{}, OptQString const &language = OptQString{}, OptQString const &country = OptQString{});

  void saveAsImpl(bool requireNewFileName, std::function<bool(bool, QString &)> const &worker);
  void saveAsXmlImpl(bool requireNewFileName);
  void saveToMatroskaImpl(bool requireNewFileName);
  void updateFileNameDisplay();

  void applyModificationToTimestamps(QStandardItem *item, std::function<int64_t(int64_t)> const &unaryOp);
  void multiplyTimestamps(QStandardItem *item, double factor);
  void shiftTimestamps(QStandardItem *item, int64_t delta);
  void constrictTimestamps(QStandardItem *item, boost::optional<uint64_t> const &constrictStart, boost::optional<uint64_t> const &constrictEnd);
  std::pair<boost::optional<uint64_t>, boost::optional<uint64_t>> expandTimestamps(QStandardItem *item);
  void setLanguages(QStandardItem *item, QString const &language);
  void setCountries(QStandardItem *item, QString const &country);
  void setEndTimestamps(QStandardItem *startItem);

protected:
  void setupToolTips();

  void chaptersLoaded(ChaptersPtr const &chapters, bool canBeWritten);

  QString currentState() const;
  QStringList usedNameLanguages(QStandardItem *parentItem = nullptr);
  QStringList usedNameCountryCodes(QStandardItem *parentItem = nullptr);
  ChaptersPtr timestampsToChapters(std::vector<timestamp_c> const &timestamps) const;
  QHash<KaxChapterAtom *, ChapterAtomDataPtr> collectChapterAtomDataForEdition(QStandardItem *item);
  QString formatChapterName(QString const &nameTemplate, int chapterNumber, timestamp_c const &startTimestamp) const;
  bool changeChapterName(QModelIndex const &parentIdx, int row, int chapterNumber, QString const &nameTemplate, RenumberSubChaptersParametersDialog::NameMatch nameMatchingMode, QString const &languageOfNamesToReplace,
                         bool skipHidden);

  bool focusNextChapterAtom(FocusElementType toFocus);
  bool focusNextChapterName();
  void focusNextChapterElement(bool keepSameControl);

  static QString formatEbmlBinary(EbmlBinary *binary);
};

}}}
