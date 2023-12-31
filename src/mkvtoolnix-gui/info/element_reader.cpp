#include "common/common_pch.h"

#include <QModelIndex>

#include <ebml/EbmlStream.h>
#include <matroska/KaxSegment.h>

#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "mkvtoolnix-gui/info/element_reader.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx::gui::Info {

ElementReader::ElementReader(mm_io_c &in,
                             libebml::EbmlElement &element,
                             QModelIndex const &idx)
  : m_in{in}
  , m_element{element}
  , m_idx{idx}
{
}

ElementReader::~ElementReader() {
}

void
ElementReader::abort() {
}

void
ElementReader::run() {
  try {
    m_in.setFilePointer(m_element.GetElementPosition());

    auto callbacks = find_ebml_callbacks(EBML_INFO(libmatroska::KaxSegment), libebml::EbmlId(m_element));
    if (!callbacks)
      callbacks = &EBML_CLASS_CALLBACK(libmatroska::KaxSegment);

    libebml::EbmlStream es(m_in);
    auto upper_lvl_el = static_cast<libebml::EbmlElement *>(nullptr);
    auto upper_lvl    = 0;

    m_element.Read(es, EBML_INFO_CONTEXT(*callbacks), upper_lvl, upper_lvl_el, true);

    if (upper_lvl)
      delete upper_lvl_el;

  } catch (mtx::mm_io::exception &) {
  }

  Q_EMIT elementRead(m_idx);
}

}
