#pragma once

#include "common/common_pch.h"

#include <QDialog>

class QCheckBox;
class QGridLayout;
class QLineEdit;

namespace mtx { namespace gui { namespace Merge {

namespace Ui {
class AdditionalCommandLineOptionsDialog;
}

class AdditionalCommandLineOptionsDialog : public QDialog {
  Q_OBJECT;
protected:
  struct Option {
    QString title;
    QCheckBox *control{};
    QLineEdit *value{};

    Option(QString const &title_, QCheckBox *control_)
      : title{title_}
      , control{control_}
    {
    }
  };
  using OptionPtr = std::shared_ptr<Option>;

  std::unique_ptr<Ui::AdditionalCommandLineOptionsDialog> m_ui;
  QString m_customOptions;
  QList<OptionPtr> m_options;

public:
  explicit AdditionalCommandLineOptionsDialog(QWidget *parent, QString const &options);
  ~AdditionalCommandLineOptionsDialog();

  void hideSaveAsDefaultCheckbox();

  QString additionalOptions() const;
  bool saveAsDefault() const;

public slots:
  void enableOkButton();

private:
  void add(QString const &title, bool requiresValue, QGridLayout *layout, QStringList const &description);
};

}}}
