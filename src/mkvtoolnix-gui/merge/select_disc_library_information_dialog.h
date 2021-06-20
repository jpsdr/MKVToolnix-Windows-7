#pragma once

#include "common/common_pch.h"

#include "common/bluray/disc_library.h"

#include <QDialog>

namespace mtx::gui::Merge {

namespace Ui {
class SelectDiscLibraryInformationDialog;
}

class SelectDiscLibraryInformationDialog : public QDialog {
  Q_OBJECT
protected:
  std::unique_ptr<Ui::SelectDiscLibraryInformationDialog> m_ui;

public:
  explicit SelectDiscLibraryInformationDialog(QWidget *parent, mtx::bluray::disc_library::disc_library_t const &discLibrary);
  ~SelectDiscLibraryInformationDialog();

  std::optional<mtx::bluray::disc_library::info_t> selectedInfo() const;

public Q_SLOTS:
  void selectionChanged();
};

}
