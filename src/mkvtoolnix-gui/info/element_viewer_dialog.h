#pragma once

#include "common/common_pch.h"

#include <QColor>
#include <QDialog>

#include "mkvtoolnix-gui/info/element_highlighter.h"

namespace mtx { namespace gui { namespace Info {

namespace Ui {
class ElementViewerDialog;
}

class ElementViewerDialogPrivate;
class ElementViewerDialog : public QDialog {
  Q_OBJECT;

public:
  static int constexpr DetachWindow = 42;

protected:
  MTX_DECLARE_PRIVATE(ElementViewerDialogPrivate);

  std::unique_ptr<ElementViewerDialogPrivate> const p_ptr;

  explicit ElementViewerDialog(ElementViewerDialogPrivate &p);

public:
  explicit ElementViewerDialog(QWidget *parent);
  ~ElementViewerDialog();

  ElementViewerDialog &setContent(memory_c const &mem, ElementHighlighter::Highlights const &highlights = {});
  ElementViewerDialog &setId(uint32_t id);
  ElementViewerDialog &setPosition(uint64_t position);
  ElementViewerDialog &setSize(boost::optional<uint64_t> signaledSize, uint64_t effectiveSize);

  void detachWindow();

public slots:
  virtual int exec() override;
  virtual void retranslateUi();
  virtual void requestDetachingWindow();

protected:
  QString createHexDump(memory_c const &mem, ElementHighlighter::Highlights const &highlights);
  QString createLegend(ElementHighlighter::Highlights const &highlights);
  QString createHtmlHead(ElementHighlighter::Highlights const &highlights, QString const &css = {});
  QString elementName() const;
};

}}}
