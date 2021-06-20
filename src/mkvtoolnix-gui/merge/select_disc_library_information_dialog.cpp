#include "common/common_pch.h"

#include <QPushButton>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/merge/select_disc_library_information_dialog.h"
#include "mkvtoolnix-gui/merge/select_disc_library_information_dialog.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

SelectDiscLibraryInformationDialog::SelectDiscLibraryInformationDialog(QWidget *parent,
                                                                       mtx::bluray::disc_library::disc_library_t const &discLibrary)
  : QDialog{parent}
  , m_ui{new Ui::SelectDiscLibraryInformationDialog}
{
  // Setup UI controls.
  m_ui->setupUi(this);

  m_ui->discLibrary->setDiscLibrary(discLibrary);
  m_ui->discLibrary->setup();

  Util::buttonForRole(m_ui->buttons, QDialogButtonBox::AcceptRole)->setText(QY("&Use information"));
  Util::buttonForRole(m_ui->buttons, QDialogButtonBox::RejectRole)->setText(QY("&Don't use information"));

  Util::restoreWidgetGeometry(this);

  selectionChanged();

  connect(m_ui->discLibrary->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SelectDiscLibraryInformationDialog::selectionChanged);
}

SelectDiscLibraryInformationDialog::~SelectDiscLibraryInformationDialog() {
  Util::saveWidgetGeometry(this);
}

void
SelectDiscLibraryInformationDialog::selectionChanged() {
  auto hasSelection = m_ui->discLibrary->selectionModel()->hasSelection();
  Util::buttonForRole(m_ui->buttons, QDialogButtonBox::AcceptRole)->setEnabled(hasSelection);

}

std::optional<mtx::bluray::disc_library::info_t>
SelectDiscLibraryInformationDialog::selectedInfo()
  const {
  return m_ui->discLibrary->selectedInfo();
}

}
