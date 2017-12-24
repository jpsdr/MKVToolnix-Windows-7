/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A Qt GUI for mkvinfo

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <QFile>
#include <QMainWindow>
#include <QString>
#include <QTreeWidgetItem>
#include <QVector>

#include "common/qt.h"
#include "info/options.h"
#include "info/ui/mainwindow.h"

class main_window_c: public QMainWindow, public Ui_main_window {
  Q_OBJECT;

private:
  int last_percent{-1}, num_elements{};

  QVector<QTreeWidgetItem *> parent_items;
  QString current_file;
  QTreeWidgetItem *root{};

  options_c m_options;

public:
  main_window_c(options_c const &options);

  void expand_all_elements(QTreeWidgetItem *item, bool expand);

  void parse_file(const QString &file_name);

  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dropEvent(QDropEvent *event);

public slots:
  void open();
  void save_text_file();

  void show_all();

  void about();

  void show_error(QString const &message);
  void show_progress(int percentage, const QString &text);
  void show_element(int level, QString const &text, int64_t position, int64_t size);
  void add_item(int level, QString const &text);
  void toggle_element_expansion(QTreeWidgetItem *item);

private:
  void expand_elements();
  void write_tree(QFile &file, QTreeWidgetItem *item, int level);
};
