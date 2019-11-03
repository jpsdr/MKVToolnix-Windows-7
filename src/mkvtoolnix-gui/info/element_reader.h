#pragma once

#include "common/common_pch.h"

#include <QObject>
#include <QModelIndex>

#include "mkvtoolnix-gui/util/kax_info.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx::gui::Info {

class ElementReader : public QObject, public Util::Runnable {
  Q_OBJECT

protected:
  mm_io_c &m_in;
  EbmlElement &m_element;
  QModelIndex m_idx;

public:
  explicit ElementReader(mm_io_c &in, EbmlElement &m_element, QModelIndex const &idx);
  virtual ~ElementReader();

  virtual void run() override;
  virtual void abort() override;

signals:
  void elementRead(const QModelIndex &idx);
};

}
