#include "common/common_pch.h"

#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QSettings>

#include "common/fs_sys_helpers.h"
#include "common/kax_info.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/main_window/update_checker.h"
#include "mkvtoolnix-gui/merge/file_identification_pack.h"
#include "mkvtoolnix-gui/merge/source_file.h"
#include "mkvtoolnix-gui/util/installation_checker.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/settings_names.h"

using namespace mtx::gui;

namespace {

void
enableOrDisableHighDPIScaling() {
  // Get this one setting from the ini file directly without
  // instantiating & loading Util::Settings as that requires
  // Application to be instantiated — which in turn requires that the
  // DPI setting's been set.
  auto reg = Util::Settings::registry();
  reg->beginGroup(s_grpSettings);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  if (reg->value(s_valUiDisableHighDPIScaling).toBool())
    return;

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps,    true);
# if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
# endif
#else
  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#endif
}

void
initiateSettings() {
#if defined(SYS_WINDOWS)
  if (mtx::sys::get_environment_variable("QT_MESSAGE_PATTERN").empty())
    mtx::sys::set_environment_variable("QT_MESSAGE_PATTERN", "[%{type}] %{appname} (%{file}:%{line}) - %{message}");

  QApplication::setStyle(Q("Fusion"));
#endif

  QCoreApplication::setOrganizationName("bunkus.org");
  QCoreApplication::setOrganizationDomain("bunkus.org");
  QCoreApplication::setApplicationName("mkvtoolnix-gui");
}

}

Q_DECLARE_METATYPE(std::shared_ptr<Merge::SourceFile>)
Q_DECLARE_METATYPE(QList<std::shared_ptr<Merge::SourceFile>>)

static void
registerMetaTypes() {
  qRegisterMetaType<Jobs::Job::LineType>("Job::LineType");
  qRegisterMetaType<Jobs::Job::Status>("Job::Status");
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<std::shared_ptr<Merge::SourceFile>>("std::shared_ptr<SourceFile>");
  qRegisterMetaType<QList<std::shared_ptr<Merge::SourceFile>>>("QList<std::shared_ptr<SourceFile>>");
  qRegisterMetaType<QVector<std::shared_ptr<Merge::SourceFile>>>("QVector<std::shared_ptr<SourceFile>>");
  qRegisterMetaType<QFileInfoList>("QFileInfoList");
  qRegisterMetaType<mtx_release_version_t>("mtx_release_version_t");
  qRegisterMetaType<std::shared_ptr<pugi::xml_document>>("std::shared_ptr<pugi::xml_document>");
#if defined(HAVE_UPDATE_CHECK)
  qRegisterMetaType<UpdateCheckStatus>("UpdateCheckStatus");
#endif  // HAVE_UPDATE_CHECK
  qRegisterMetaType<Util::InstallationChecker::Problems>("Util::InstallationChecker::Problems");
  qRegisterMetaType<mtx::kax_info_c::result_e>("mtx::kax_info_c::result_e");
  qRegisterMetaType<int64_t>("int64_t");
  qRegisterMetaType<libebml::EbmlElement *>("EbmlElement *");
  qRegisterMetaType<std::optional<int64_t>>("std::optional<int64_t>");
  qRegisterMetaType<Merge::IdentificationPack>("Merge::IdentificationPack");
}

int
main(int argc,
     char **argv) {
  mtx_common_init("mkvtoolnix-gui", argv[0]);

  initiateSettings();

  enableOrDisableHighDPIScaling();

  registerMetaTypes();

  App::registerOriginalCLIArgs(argc, argv);

  auto app = std::make_unique<App>(argc, argv);

  app->run();

  app.reset();

  mxexit();
}
