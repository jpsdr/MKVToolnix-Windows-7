#pragma once

#include "common/common_pch.h"

#include <QFileDialog>

class QString;

namespace mtx { namespace gui { namespace Util {

QString dirPath(QDir const &dir);
QString dirPath(QString const &dir);

QString sanitizeDirectory(QString const &directory, bool withFileName);
QString getOpenFileName(QWidget *parent = nullptr, QString const &caption = QString{}, QString const &dir = QString{}, QString const &filter = QString{},
                        QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options{});
QStringList getOpenFileNames(QWidget *parent = nullptr, QString const &caption = QString{}, QString const &dir = QString{}, QString const &filter = QString{},
                             QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options{});
QString getSaveFileName(QWidget *parent = nullptr, QString const &caption = QString{}, QString const &dir = QString{}, QString const &filter = QString{},
                        QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options{});
QString getExistingDirectory(QWidget *parent = nullptr, QString const &caption = QString{}, QString const &dir = QString{}, QFileDialog::Options options = QFileDialog::ShowDirsOnly);

}}}
