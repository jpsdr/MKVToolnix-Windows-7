#pragma once

#include "common/common_pch.h"

#include <QTreeWidget>

#include "common/bluray/disc_library.h"

namespace mtx::gui::Merge {

class DiscLibraryInformationWidget : public QTreeWidget {
  Q_OBJECT

protected:
  mtx::bluray::disc_library::disc_library_t m_discLibrary;

public:
  explicit DiscLibraryInformationWidget(QWidget *parent);
  ~DiscLibraryInformationWidget();

  void setDiscLibrary(mtx::bluray::disc_library::disc_library_t const &discLibrary);
  bool isEmpty() const;

  std::optional<mtx::bluray::disc_library::info_t> selectedInfo();

  void setup();
};

}
