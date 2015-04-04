#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H

#include "common/common_pch.h"

#include "common/qt_kax_analyzer.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"

class QVBoxLayout;

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class Tab;
}

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QVBoxLayout *m_pageContainerLayout{};

  QString m_fileName;
  std::unique_ptr<QtKaxAnalyzer> m_analyzer;
  PageModel *m_model;

  std::shared_ptr<EbmlElement> m_eSegmentInfo, m_eTracks;

public:
  explicit Tab(QWidget *parent, QString const &fileName);
  ~Tab();

  PageModel *getModel() const;

  virtual bool hasBeenModified();
  virtual void retranslateUi();
  virtual QWidget *getPageContainer() const;
  virtual void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  virtual QString const &getFileName() const;
  virtual void validate();

signals:
  void removeThisTab();

public slots:
  virtual void selectionChanged(QModelIndex const &current, QModelIndex const &previous);
  virtual void load();

protected:
  void setupUi();
  void handleSegmentInfo(kax_analyzer_data_c &data);
  void handleTracks(kax_analyzer_data_c &data);
  void populateTree();
  void resetData();
  void doModifications();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H
