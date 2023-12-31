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
  libebml::EbmlElement &m_element;
  QModelIndex m_idx;

public:
  explicit ElementReader(mm_io_c &in, libebml::EbmlElement &m_element, QModelIndex const &idx);
  virtual ~ElementReader();

  virtual void run() override;
  virtual void abort() override;

Q_SIGNALS:
  void elementRead(const QModelIndex &idx);
};

}
