#include "common/common_pch.h"

#include <Qt>
#include <QDir>
#if HAVE_QMEDIAPLAYER && (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
# include <QMediaFormat>
#endif
#include <QOperatingSystemVersion>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QSysInfo>

#include <boost/version.hpp>
#include <cmark.h>
#if defined(HAVE_DVDREAD)
# include <dvdread/dvd_reader.h>
#endif
#include <ebml/EbmlVersion.h>
#if defined(HAVE_FLAC_FORMAT_H)
# include <FLAC/format.h>
#endif
#include <nlohmann/json.hpp>
#include <matroska/KaxVersion.h>
#include <pugixml.hpp>
#include <vorbis/codec.h>
#include <zlib.h>

#include "common/fs_sys_helpers.h"
#include "common/version.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/jobs/job.h"
#include "mkvtoolnix-gui/util/installation_checker.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/system_information.h"

namespace mtx::gui::Util {

using namespace mtx::gui;

namespace {

QString
formatPugixmlVersion() {
  // From pugixml.hpp:
  //   Define version macro; evaluates to major * 1000 + minor * 10 +
  //   patch so that it's safe to use in less-than comparisons
  //
  //   Note: pugixml used major * 100 + minor * 10 + patch format up
  //   until 1.9 (which had version identifier 190); starting from
  //   pugixml 1.10, the minor version number is two digits
#if PUGIXML_VERSION >= 1000
  return Q("%1.%2.%3").arg(PUGIXML_VERSION / 1'000).arg((PUGIXML_VERSION / 10) % 100).arg(PUGIXML_VERSION % 10);
#else
  return Q("%1.%2.%3").arg(PUGIXML_VERSION / 100).arg((PUGIXML_VERSION / 10) % 10).arg(PUGIXML_VERSION % 10);
#endif
}

void
gatherGeneralInfo(QStringList &info) {
  InstallationChecker checker;
  checker.runChecks();

  info << Q("# MKVToolNix") << Q("");
  info << Q("## General") << Q("");

  info << Q("* MKVToolNix GUI version: %1").arg(Q(get_current_version().to_string()));
  info << Q("* mkvmerge version: %1").arg(checker.mkvmergeVersion().isEmpty() ? Q("unknown") : checker.mkvmergeVersion());
#if defined(SYS_WINDOWS)
  info << Q("* Installation type: %1").arg(App::isInstalled() ? Q("installed") : Q("portable"));
#endif
  info << Q("* Installation path: %1").arg(QDir::toNativeSeparators(App::applicationDirPath()));
  info << Q("* INI file location: %1").arg(QDir::toNativeSeparators(Util::Settings::iniFileName()));
  info << Q("* Job queue location: %1").arg(QDir::toNativeSeparators(Jobs::Job::queueLocation()));

  //
  // Original command-line arguments
  //

  info << QString{} << Q("## Original command-line arguments") << QString{};
  auto &args = App::originalCLIArgs();

  if (args.isEmpty())
    info << Q("The application was started without command-line arguments.");

  else
    for (auto const &arg : args)
      info << Q("* `%1`").arg(arg);

  //
  // Installation problems
  //

  info << Q("") << Q("## Installation problems") << Q("");

  auto problems = checker.problems();
  if (problems.isEmpty()) {
    info << Q("No problems were found with the installation.");
    return;
  }

  for (auto const &problem : problems) {
    auto name = problem.first == InstallationChecker::ProblemType::FileNotFound                  ? Q("FileNotFound")
              : problem.first == InstallationChecker::ProblemType::MkvmergeNotFound              ? Q("MkvmergeNotFound")
              : problem.first == InstallationChecker::ProblemType::MkvmergeCannotBeExecuted      ? Q("MkvmergeCannotBeExecuted")
              : problem.first == InstallationChecker::ProblemType::MkvmergeVersionNotRecognized  ? Q("MkvmergeVersionNotRecognized")
              : problem.first == InstallationChecker::ProblemType::MkvmergeVersionDiffers        ? Q("MkvmergeVersionDiffers")
              : problem.first == InstallationChecker::ProblemType::TemporaryDirectoryNotWritable ? Q("TemporaryDirectoryNotWritable")
              : problem.first == InstallationChecker::ProblemType::PortableDirectoryNotWritable  ? Q("PortableDirectoryNotWritable")
              :                                                                                    Q("unknown");
    info << Q("* Type: %1, info: %2").arg(name).arg(problem.second.isEmpty() ? Q("—") : problem.second);
  }
}

void
gatherOperatingSystemInfo(QStringList &info) {
  QString osName, osVersion;
  auto versionInfo = QOperatingSystemVersion::current();

  if (versionInfo.type() != QOperatingSystemVersion::Unknown) {
    osName = versionInfo.name();
  } else {
    osName = QSysInfo::productType();
  }

  if (versionInfo.majorVersion() != -1) {
    osVersion += QString::number(versionInfo.majorVersion());
    if (versionInfo.minorVersion() != -1) {
      osVersion += QLatin1Char('.');
      osVersion += QString::number(versionInfo.minorVersion());
      if (versionInfo.microVersion() != -1) {
        osVersion += QLatin1Char('.');
        osVersion += QString::number(versionInfo.microVersion());
      }
    }
  } else {
    osVersion = QSysInfo::productVersion();
  }

  info << Q("") << Q("## Operating system");
  info << Q("* Name: %1").arg(osName);
  info << Q("* Version: %1").arg(osVersion);
  info << Q("* Pretty name: %1").arg(QSysInfo::prettyProductName());
}

void
gatherPathInformation(QStringList &info) {
  info << Q("") << Q("## Paths");
  info << Q("* Is installed: %1").arg(App::isInstalled());
  info << Q("* Application directory: %1").arg(QDir::toNativeSeparators(App::applicationDirPath()));
  info << Q("* Cache directory (via Qt): %1").arg(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));
}

void
gatherScreenInfo(QStringList &info) {
  info << Q("") << Q("## Screens");

  int idx = 0;
  for (auto const &screen : App::screens()) {
    info << Q("") << Q("### Screen %1").arg(idx++) << Q("");
    info << Q("* Device pixel ratio: %1").arg(screen->devicePixelRatio());
    info << Q("* Logical DPI: %1x%2").arg(screen->logicalDotsPerInchX()).arg(screen->logicalDotsPerInchX());
    info << Q("* Physical DPI: %1x%2").arg(screen->physicalDotsPerInchX()).arg(screen->physicalDotsPerInchX());
    info << Q("* Physical size: %1x%2").arg(screen->physicalSize().width()).arg(screen->physicalSize().height());
    info << Q("* Virtual size: %1x%2").arg(screen->virtualSize().width()).arg(screen->virtualSize().height());
    info << Q("* Geometry: %1x%2@%3x%4").arg(screen->geometry().width()).arg(screen->geometry().height()).arg(screen->geometry().x()).arg(screen->geometry().y());
  }
}

void
gatherDesktopScalingAndThemeSettings([[maybe_unused]] QStringList &info) {
#if defined(SYS_WINDOWS)
  QSettings regDesktop{Q("HKEY_CURRENT_USER\\Control Panel\\Desktop"), QSettings::NativeFormat};
  QSettings regPersonalize{Q("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat};

  info << Q("") << Q("## Desktop scaling & theme settings") << Q("");

  info << Q("* Scaling mode (`Win8DpiScaling`): %1").arg(regDesktop.value("Win8DpiScaling", "not set").toString());
  info << Q("* Scaling override (`DesktopDPIOverride`): %1").arg(regDesktop.value("DesktopDPIOverride", "not set").toString());
  info << Q("* System-wide scale factor (`LogPixels`): %1").arg(regDesktop.value("LogPixels", "not set").toString());
  info << Q("* Windows 11 application light theme (`AppsUseLightTheme`): %1").arg(regPersonalize.value("AppsUseLightTheme", "not set").toString());
  info << Q("* High contrast mode: %1").arg(mtx::sys::is_high_contrast_enabled());
#endif
}

void
gatherEnvironmentVariables(QStringList &info) {
  info << Q("") << Q("## Environment variables") << Q("");

  auto keys = QStringList{} << Q("QT_AUTO_SCREEN_SCALE_FACTOR") << Q("QT_SCALE_FACTOR") << Q("QT_SCREEN_SCALE_FACTORS") << Q("QT_DEVICE_PIXEL_RATIO") << Q("QT_SCALE_FACTOR_ROUNDING_POLICY")
                            << Q("QT_STYLE_OVERRIDE")           << Q("QT_QPA_PLATFORM") << Q("QT_QPA_PLATFORMTHEME")    << Q("QT_QPA_GENERIC_PLUGINS")
                            << Q("MTX_LOGGER")                  << Q("MTX_DEBUG")       << Q("MKVTOOLNIX_DEBUG")        << Q("MKVMERGE_DEBUG")
                            << Q("LC_ALL")                      << Q("LC_MESSAGES")     << Q("LC_CTYPE")                << Q("LANG") << Q("LANGUAGE");
  keys.sort();

  for (auto const &name : keys)
    info << Q("* `%1=%2`").arg(name).arg(Q(mtx::sys::get_environment_variable(to_utf8(name))));
}

void
gatherQtInfo(QStringList &info) {
  info << Q("") << Q("# Qt") << Q("");

  info << Q("* Version: %1.%2.%3").arg((QT_VERSION >> 16) & 0xff).arg((QT_VERSION >> 8) & 0xff).arg(QT_VERSION & 0xff);
  info << Q("* Build ABI: %1").arg(QSysInfo::buildAbi());

#if !HAVE_QMEDIAPLAYER
  info << Q("* Multimedia module not found during build");
#elif QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  QMediaFormat formats;

  QStringList lines;

  auto disclaimer = Q("The following list is only relevant for the GUI's 'play sound after job completion' functionality and doesn't affect which formats can be multiplexed by mkvmerge.");

  for (auto const &format : formats.supportedAudioCodecs(QMediaFormat::Decode)) {
    auto name = format == QMediaFormat::AudioCodec::WMA         ? Q("WMA")
              : format == QMediaFormat::AudioCodec::AC3         ? Q("AC3")
              : format == QMediaFormat::AudioCodec::AAC         ? Q("AAC")
              : format == QMediaFormat::AudioCodec::ALAC        ? Q("ALAC")
              : format == QMediaFormat::AudioCodec::DolbyTrueHD ? Q("DolbyTrueHD")
              : format == QMediaFormat::AudioCodec::EAC3        ? Q("EAC3")
              : format == QMediaFormat::AudioCodec::MP3         ? Q("MP3")
              : format == QMediaFormat::AudioCodec::Wave        ? Q("Wave")
              : format == QMediaFormat::AudioCodec::Vorbis      ? Q("Vorbis")
              : format == QMediaFormat::AudioCodec::FLAC        ? Q("FLAC")
              : format == QMediaFormat::AudioCodec::Opus        ? Q("Opus")
              :                                                   Q("unknown (%1)").arg(static_cast<int>(format));
    lines << Q("* %1").arg(name);
  }

  lines.sort();

  info << Q("") << Q("## Supported audio codecs") << Q("") << disclaimer << Q("");
  info += lines;

  lines.clear();

  for (auto const &format : formats.supportedFileFormats(QMediaFormat::Decode)) {
    auto name = format == QMediaFormat::WMA        ? Q("WMA")
              : format == QMediaFormat::AAC        ? Q("AAC")
              : format == QMediaFormat::Matroska   ? Q("Matroska")
              : format == QMediaFormat::WMV        ? Q("WMV")
              : format == QMediaFormat::MP3        ? Q("MP3")
              : format == QMediaFormat::Wave       ? Q("Wave")
              : format == QMediaFormat::Ogg        ? Q("Ogg")
              : format == QMediaFormat::MPEG4      ? Q("MPEG4")
              : format == QMediaFormat::AVI        ? Q("AVI")
              : format == QMediaFormat::QuickTime  ? Q("QuickTime")
              : format == QMediaFormat::WebM       ? Q("WebM")
              : format == QMediaFormat::Mpeg4Audio ? Q("Mpeg4Audio")
              : format == QMediaFormat::FLAC       ? Q("FLAC")
              :                                      Q("unknown (%1)").arg(static_cast<int>(format));

    lines << Q("* %1").arg(name);
  }

  lines.sort();

  info << Q("") << Q("## Supported file formats") << Q("") << disclaimer << Q("");
  info += lines;
#endif
}

void
gatherCompilerAndLibraryInfo(QStringList &info) {
  info << Q("") << Q("# Compiler and libraries") << Q("");

#if defined(__clang__)
  info << Q("* Compiler: clang++ %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
#elif defined(__GNUC__)
  info << Q("* Compiler: g++ %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#else
  info << Q("* Compiler: unknown");
#endif

  info << Q("* Boost: %1.%2.%3").arg(BOOST_VERSION / 100'000).arg((BOOST_VERSION / 100) % 1'000).arg(BOOST_VERSION % 100);
  info << Q("* cmark: %1").arg(Q(CMARK_VERSION_STRING));
#if defined(HAVE_DVDREAD)
  info << Q("* dvdread: %1.%2.%3").arg(DVDREAD_VERSION / 10'000).arg((DVDREAD_VERSION / 100) % 100).arg(DVDREAD_VERSION % 100);
#endif
  info << Q("* EBML: %1").arg(Q(libebml::EbmlCodeVersion));
#if defined(HAVE_FLAC_FORMAT_H)
  info << Q("* FLAC: %1").arg(Q(FLAC__VERSION_STRING));
#endif
  info << Q("* fmt: %1.%2.%3").arg(FMT_VERSION / 10'000).arg((FMT_VERSION / 100) % 100).arg(FMT_VERSION % 100);
  info << Q("* Matroska: %1").arg(Q(libmatroska::KaxCodeVersion));
  info << Q("* nlohmann-json: %1.%2.%3").arg(NLOHMANN_JSON_VERSION_MAJOR).arg(NLOHMANN_JSON_VERSION_MINOR).arg(NLOHMANN_JSON_VERSION_PATCH);
  info << Q("* pugixml: %1").arg(formatPugixmlVersion());
  info << Q("* Vorbis: %1").arg(Q(vorbis_version_string()));
  info << Q("* zlib: %1").arg(Q(ZLIB_VERSION));
}

} // anonymous namespace

QString
gatherSystemInformation() {
  QStringList info;

  gatherGeneralInfo(info);

  info << Q("") << Q("# System");

  gatherOperatingSystemInfo(info);
  gatherPathInformation(info);
  gatherScreenInfo(info);
  gatherDesktopScalingAndThemeSettings(info);
  gatherEnvironmentVariables(info);

  gatherQtInfo(info);

  gatherCompilerAndLibraryInfo(info);

  return info.join(Q("\n"));
}

}
