#pragma once

#include "common/common_pch.h"

#include <QLabel>

#include "mkvtoolnix-gui/header_editor/page_base.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace libebml;

class EmptyPage: public PageBase {
  Q_OBJECT;

public:
  translatable_string_c m_content;

  QLabel *m_lTitle{}, *m_lContent{};

public:
  EmptyPage(Tab &parent, translatable_string_c const &title, translatable_string_c const &content, bool customLayout = false);
  virtual ~EmptyPage();

  virtual bool hasThisBeenModified() const override;
  virtual bool validateThis() const override;
  virtual void modifyThis() override;
  virtual void retranslateUi() override;

protected:
  virtual void setupUi();
};
}}}
