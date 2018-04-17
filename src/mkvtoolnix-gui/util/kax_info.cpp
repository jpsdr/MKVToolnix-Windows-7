/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <ebml/EbmlStream.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxSegment.h>

#include "common/at_scope_exit.h"
#include "common/ebml.h"
#include "common/kax_element_names.h"
#include "common/kax_file.h"
#include "common/mm_io_x.h"
#include "common/mm_proxy_io.h"
#include "common/mm_read_buffer_io.h"
#include "common/qt.h"
#include "common/kax_info_p.h"
#include "mkvtoolnix-gui/util/kax_info.h"

namespace mtx { namespace gui { namespace Util {

class KaxInfoPrivate: public mtx::kax_info::private_c {
public:
  KaxInfo::ScanType m_scanType{KaxInfo::ScanType::StartOfFile};
  boost::optional<uint64_t> m_firstLevel1ElementPosition;
  QMutex m_mutex;

  explicit KaxInfoPrivate()
    : m_mutex{QMutex::Recursive}
  {
  }
};

KaxInfo::KaxInfo()
  : mtx::kax_info_c{*new KaxInfoPrivate}
{
}

KaxInfo::KaxInfo(QString const &file_name)
  : mtx::kax_info_c{*new KaxInfoPrivate}
{
  set_source_file_name(to_utf8(file_name));
}

KaxInfo::~KaxInfo() {
}

void
KaxInfo::ui_show_error(std::string const &error) {
  emit errorFound(Q(error));
}

void
KaxInfo::ui_show_element_info(int level,
                              std::string const &text,
                              boost::optional<int64_t> position,
                              boost::optional<int64_t> size) {
  if (p_func()->m_use_gui)
    emit elementInfoFound(level, Q(text), position, size);

  else
    kax_info_c::ui_show_element_info(level, text, position, size);
}

void
KaxInfo::ui_show_element(EbmlElement &e) {
  auto p = p_func();

  if (p->m_use_gui) {
    if ((p->m_scanType == ScanType::StartOfFile) && Is<KaxCluster>(e))
      p->m_firstLevel1ElementPosition = e.GetElementPosition();
    else
      emit elementFound(p->m_level, &e, p->m_scanType == ScanType::StartOfFile);

  } else
    kax_info_c::ui_show_element(e);
}

void
KaxInfo::ui_show_progress(int percentage,
                          std::string const &text) {
  emit progressChanged(percentage, Q(text));
}

void
KaxInfo::abort() {
  ::mtx::kax_info_c::abort();
}

void
KaxInfo::runScan(ScanType type) {
  QMutexLocker locker{&mutex()};

  auto p = p_func();

  p->m_scanType = type;

  if (type == ScanType::StartOfFile)
    scanStartOfFile();

  else if (type == ScanType::Level1Elements)
    scanLevel1Elements();

  else
    qDebug() << "programming error: unknown type" << static_cast<int>(type);
}

void
KaxInfo::scanStartOfFile() {
  QMutexLocker locker{&mutex()};

  auto p = p_func();

  emit startOfFileScanStarted();

  p->m_firstLevel1ElementPosition.reset();
  p->m_abort  = false;
  auto result = p->m_in ? process_file() : open_and_process_file(p->m_source_file_name);

  emit startOfFileScanFinished(result);
}

void
KaxInfo::scanLevel1Elements() {
  emit level1ElementsScanStarted();

  auto result = doScanLevel1Elements();

  emit level1ElementsScanFinished(result);
}

mtx::kax_info_c::result_e
KaxInfo::doScanLevel1Elements() {
  QMutexLocker locker{&mutex()};

  auto p     = p_func();
  p->m_abort = false;
  p->m_level = 1;

  if (!p->m_firstLevel1ElementPosition || !p->m_in)
    return result_e::succeeded;

  try {
    auto cache_io = dynamic_cast<mm_read_buffer_io_c *>(p->m_in.get());
    if (cache_io)
      cache_io->set_buffer_size(64);

    at_scope_exit_c reset_buffer_size([cache_io]() {
      if (cache_io)
        cache_io->set_buffer_size();
    });

    p->m_in->setFilePointer(*p->m_firstLevel1ElementPosition);

    while (!p->m_abort) {
      auto upper_lvl_el = 0;
      auto l1           = std::shared_ptr<EbmlElement>(p->m_es->FindNextElement(EBML_CLASS_CONTEXT(KaxSegment), upper_lvl_el, 0xFFFFFFFFL, true));

      if (!l1)
        break;

      retain_element(l1);
      ui_show_element(*l1);

      if (upper_lvl_el)
        break;

      p->m_in->setFilePointer(l1->GetElementPosition() + kax_file_c::get_element_size(*l1));
    }

  } catch (mtx::mm_io::exception &ex) {
    ui_show_error((boost::format("%1%: %2%") % Y("Caught exception") % ex.what()).str());
    return result_e::failed;
  }

  return p->m_abort ? result_e::aborted : result_e::succeeded;
}

QMutex &
KaxInfo::mutex() {
  return p_func()->m_mutex;
}

kax_info_c::result_e
KaxInfo::open_and_process_file(std::string const &fileName) {
  return kax_info_c::open_and_process_file(fileName);
}

kax_info_c::result_e
KaxInfo::open_and_process_file() {
  QMutexLocker locker{&mutex()};

  return kax_info_c::open_and_process_file();
}

kax_info_c::result_e
KaxInfo::process_file() {
  QMutexLocker locker{&mutex()};

  return kax_info_c::process_file();
}

void
KaxInfo::disableFrameInfo() {
  auto p = p_func();

  p->m_custom_element_post_processors.erase(KaxBlock::ClassInfos.GlobalId.GetValue());
  p->m_custom_element_post_processors.erase(KaxBlockGroup::ClassInfos.GlobalId.GetValue());
  p->m_custom_element_post_processors.erase(KaxSimpleBlock::ClassInfos.GlobalId.GetValue());
}

}}}
