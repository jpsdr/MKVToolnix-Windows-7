#include "common/common_pch.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/empty_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

EmptyPage::EmptyPage(Tab &parent,
                     translatable_string_c const &title,
                     translatable_string_c const &content,
                     bool customLayout)
  : PageBase{parent, title}
  , m_content{content}
{
  if (!customLayout)
    setupUi();
}

EmptyPage::~EmptyPage() {
}

void
EmptyPage::setupUi() {
  m_lTitle = new QLabel{this};
  m_lTitle->setWordWrap(true);

  auto line = new QFrame{this};
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);

  m_lContent = new QLabel{this};
  m_lContent->setWordWrap(true);

  // ----------------------------------------------------------------------

  auto layout = new QVBoxLayout{this};
  layout->addWidget(m_lTitle);
  layout->addWidget(line);
  layout->addWidget(m_lContent);
  layout->addItem(new QSpacerItem{0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding});

  // ----------------------------------------------------------------------
}

bool
EmptyPage::hasThisBeenModified()
  const {
  return false;
}

bool
EmptyPage::validateThis()
  const {
  return true;
}

void
EmptyPage::modifyThis() {
}

void
EmptyPage::retranslateUi() {
  if (!m_lTitle)
    return;

  m_lTitle->setText(Q(m_title.get_translated()));
  m_lContent->setText(Q(m_content.get_translated()));
}

}
