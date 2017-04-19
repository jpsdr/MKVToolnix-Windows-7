/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A Qt GUI for mkvinfo

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(SYS_WINDOWS)
# include <windows.h>
#endif

#include <ebml/EbmlVersion.h>
#include <matroska/KaxVersion.h>

#include "common/common_pch.h"
#include "common/locale.h"
#include "common/qt.h"
#include "common/version.h"
#include "info/qt_ui.h"
#include "info/mkvinfo.h"

#include <QIcon>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QFileDialog>

using namespace libebml;
using namespace libmatroska;

main_window_c::main_window_c():
  last_percent(-1), num_elements(0),
  root(nullptr) {

  setupUi(this);

  QIcon icon;
  auto sizes = QList<int>{} << 32 << 48 << 64 << 128 << 256;
  for (auto size : sizes)
    icon.addFile(QString{":/icons/%1x%1/mkvinfo.png"}.arg(size));

  setWindowIcon(icon);

  connect(action_Open,           &QAction::triggered, this, &main_window_c::open);
  connect(action_Save_text_file, &QAction::triggered, this, &main_window_c::save_text_file);
  connect(action_Exit,           &QAction::triggered, this, &main_window_c::close);

  connect(action_Show_all,       &QAction::triggered, this, &main_window_c::show_all);

  connect(action_About,          &QAction::triggered, this, &main_window_c::about);

  action_Save_text_file->setEnabled(false);

  action_Show_all->setCheckable(true);
  action_Show_all->setChecked(0 < g_options.m_verbose);

  action_Expand_important->setCheckable(true);
  action_Expand_important->setChecked(true);

  tree->setHeaderLabels(QStringList(QY("Elements")));
  tree->setRootIsDecorated(true);

  root = new QTreeWidgetItem(tree);
  root->setText(0, QY("no file loaded"));

  setAcceptDrops(true);
}

void
main_window_c::open() {
  auto matroska_extensions = Q("*.mkv *.mka *.mks *.mk3d");
  auto webm_extensions     = Q("*.webm *.webma *.webmv");
  auto filter              = Q("%1 (%2);;%3 (%4);;%5 (%6);;%7 (*)")
    .arg(QY("All supported media files")).arg(Q("%1 %2").arg(matroska_extensions).arg(webm_extensions))
    .arg(QY("Matroska files")).arg(matroska_extensions)
    .arg(QY("WebM files")).arg(webm_extensions)
    .arg(QY("All files"));

  QString file_name = QFileDialog::getOpenFileName(this, QY("Open File"), "", filter);
  if (!file_name.isEmpty())
    parse_file(file_name);
}

void
main_window_c::save_text_file() {
  QString file_name = QFileDialog::getSaveFileName(this, QY("Save information as"), "", Q("%1 (*.txt);;%2 (*)").arg(QY("Text files")).arg(QY("All files")));
  if (file_name.isEmpty())
    return;

  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(this, QY("Error saving the information"), QY("The file could not be opened for writing."));
    return;
  }

  write_tree(file, root, 0);

  file.close();
}

void
main_window_c::write_tree(QFile &file,
                          QTreeWidgetItem *item,
                          int level) {
  int i;

  for (i = 0; item->childCount() > i; ++i) {
    QTreeWidgetItem *child = item->child(i);

    char *level_buffer = new char[level + 1];
    level_buffer[0] = '|';
    memset(&level_buffer[1], ' ', level);
    level_buffer[level] = 0;

    file.write(level_buffer, level);
    file.write(QString("+ %1\n").arg(child->text(0)).toUtf8());
    write_tree(file, child, level + 1);
  }
}

void
main_window_c::show_all() {
  g_options.m_verbose = action_Show_all->isChecked() ? 2 : 0;
  if (!current_file.isEmpty())
    parse_file(current_file);
}

void
main_window_c::about() {
  auto msg =
    Q("%1\n"
      "%2\n\n"
      "%3\n"
      "%4\n")
    .arg(Q(get_version_info("mkvinfo GUI")))
    .arg(QY("Compiled with libebml %1 + libmatroska %2.").arg(Q(EbmlCodeVersion)).arg(Q(KaxCodeVersion)))
    .arg(QY("This program is licensed under the GPL v2 (see COPYING)."))
    .arg(QY("It was written by Moritz Bunkus <moritz@bunkus.org>."));

  QMessageBox::about(this, QY("About mkvinfo"), msg);
}

void
main_window_c::show_error(const QString &msg) {
  QMessageBox::critical(this, QY("Error"), msg);
}

void
main_window_c::parse_file(const QString &file_name) {
  auto title = Q("%1 – mkvinfo").arg(QFileInfo{file_name}.fileName());
  setWindowTitle(title);

  tree->setEnabled(false);
  tree->clear();

  root = new QTreeWidgetItem(tree);
  root->setText(0, file_name);

  last_percent = -1;
  num_elements = 0;
  action_Save_text_file->setEnabled(false);

  parent_items.clear();
  parent_items.append(root);

  if (process_file(file_name.toUtf8().data())) {
    action_Save_text_file->setEnabled(true);
    current_file = file_name;
    if (action_Expand_important->isChecked())
      expand_elements();
  }

  statusBar()->showMessage(QY("Ready"), 5000);

  tree->setEnabled(true);
}

void
main_window_c::expand_all_elements(QTreeWidgetItem *item,
                                   bool expand) {
  int i;

  if (expand)
    tree->expandItem(item);
  else
    tree->collapseItem(item);
  for (i = 0; item->childCount() > i; ++i)
    expand_all_elements(item->child(i), expand);
}

void
main_window_c::expand_elements() {
  int l0, l1, c0, c1;
  QTreeWidgetItem *i0, *i1;
  const QString s_segment(QY("Segment"));
  const QString s_info(QY("Segment information"));
  const QString s_tracks(QY("Segment tracks"));

  setUpdatesEnabled(false);

  expand_all_elements(root, false);
  tree->expandItem(root);

  c0 = root->childCount();
  for (l0 = 0; l0 < c0; ++l0) {
    i0 = root->child(l0);
    tree->expandItem(i0);
    if (i0->text(0).left(7) == s_segment) {
      c1 = i0->childCount();
      for (l1 = 0; l1 < c1; ++l1) {
        i1 = i0->child(l1);
        if ((i1->text(0).left(19) == s_info) ||
            (i1->text(0).left(14) == s_tracks))
          expand_all_elements(i1, true);
      }
    }
  }

  setUpdatesEnabled(true);
}

void
main_window_c::add_item(int level,
                        const QString &text) {
  ++level;

  while (parent_items.count() > level)
    parent_items.erase(parent_items.end() - 1);
  QTreeWidgetItem *item = new QTreeWidgetItem(parent_items.last());
  item->setText(0, text);
  parent_items.append(item);
}

void
main_window_c::show_progress(int percentage,
                             const QString &text) {
  if ((percentage / 5) != (last_percent / 5)) {
    statusBar()->showMessage(QString("%1: %2%").arg(text).arg(percentage));
    last_percent = percentage;
    QCoreApplication::processEvents();
  }
}

void
main_window_c::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile()) {
      event->acceptProposedAction();
      return;
    }
}

void
main_window_c::dropEvent(QDropEvent *event) {
  if (!event->mimeData()->hasUrls())
    return;

  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile()) {
      parse_file(url.toLocalFile());
      event->acceptProposedAction();
      return;
    }
}

static main_window_c *gui;

rightclick_tree_widget::rightclick_tree_widget(QWidget *parent):
  QTreeWidget(parent) {
}

void
rightclick_tree_widget::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::RightButton) {
    QTreeWidget::mousePressEvent(event);
    return;
  }

  QTreeWidgetItem *item = itemAt(event->pos());
  if (item) {
    gui->expand_all_elements(item, !item->isExpanded());
  }
}

void
ui_show_error(const std::string &error) {
  if (g_options.m_use_gui)
    gui->show_error(Q(error));
  else
    console_show_error(error);
}

void
ui_show_element(int level,
                const std::string &text,
                int64_t position,
                int64_t size) {
  if (!g_options.m_use_gui)
    console_show_element(level, text, position, size);

  else if (0 <= position)
    gui->add_item(level, Q(create_element_text(text, position, size)));

  else
    gui->add_item(level, Q(text));
}

void
ui_show_progress(int percentage,
                 const std::string &text) {
  gui->show_progress(percentage, Q(text));
}

int
ui_run(int argc,
       char **argv) {
#if defined(SYS_WINDOWS)
  FreeConsole();
#endif

  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvinfo");

#ifdef SYS_WINDOWS
  QApplication::setStyle(Q("windowsvista"));
#endif

  main_window_c main_window;
  gui = &main_window;
  main_window.show();

  if (!g_options.m_file_name.empty())
    gui->parse_file(Q(g_options.m_file_name));

  return app.exec();
}

bool
ui_graphical_available() {
  return true;
}
