#include "common/common_pch.h"

#include <Qt>
#include <QDir>
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
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
  return u"%1.%2.%3"_s.arg(PUGIXML_VERSION / 1'000).arg((PUGIXML_VERSION / 10) % 100).arg(PUGIXML_VERSION % 10);
#else
  return u"%1.%2.%3"_s.arg(PUGIXML_VERSION / 100).arg((PUGIXML_VERSION / 10) % 10).arg(PUGIXML_VERSION % 10);
#endif
}

void
gatherGeneralInfo(QStringList &info) {
  InstallationChecker checker;
  checker.runChecks();

  info << u"# MKVToolNix"_s << u""_s;
  info << u"## General"_s << u""_s;

  info << u"* MKVToolNix GUI version: %1"_s.arg(Q(get_current_version().to_string()));
  info << u"* mkvmerge version: %1"_s.arg(checker.mkvmergeVersion().isEmpty() ? u"unknown"_s : checker.mkvmergeVersion());

  //
  // Original command-line arguments
  //

  info << QString{} << u"## Original command-line arguments"_s << QString{};
  auto &args = App::originalCLIArgs();

  if (args.isEmpty())
    info << u"The application was started without command-line arguments."_s;

  else
    for (auto const &arg : args)
      info << u"* `%1`"_s.arg(arg);

  //
  // Installation problems
  //

  info << u""_s << u"## Installation problems"_s << u""_s;

  auto problems = checker.problems();
  if (problems.isEmpty()) {
    info << u"No problems were found with the installation."_s;
    return;
  }

  for (auto const &problem : problems) {
    auto name = problem.first == InstallationChecker::ProblemType::FileNotFound                  ? u"FileNotFound"_s
              : problem.first == InstallationChecker::ProblemType::MkvmergeNotFound              ? u"MkvmergeNotFound"_s
              : problem.first == InstallationChecker::ProblemType::MkvmergeCannotBeExecuted      ? u"MkvmergeCannotBeExecuted"_s
              : problem.first == InstallationChecker::ProblemType::MkvmergeVersionNotRecognized  ? u"MkvmergeVersionNotRecognized"_s
              : problem.first == InstallationChecker::ProblemType::MkvmergeVersionDiffers        ? u"MkvmergeVersionDiffers"_s
              : problem.first == InstallationChecker::ProblemType::TemporaryDirectoryNotWritable ? u"TemporaryDirectoryNotWritable"_s
              : problem.first == InstallationChecker::ProblemType::PortableDirectoryNotWritable  ? u"PortableDirectoryNotWritable"_s
              :                                                                                    u"unknown"_s;
    info << u"* Type: %1, info: %2"_s.arg(name).arg(problem.second.isEmpty() ? u"—"_s : problem.second);
  }
}

void
gatherOperatingSystemInfo(QStringList &info) {
  QString osName, osVersion;
  auto versionInfo = QOperatingSystemVersion::current();

  if (static_cast<QOperatingSystemVersion::OSType>(versionInfo.type()) != QOperatingSystemVersion::Unknown) {
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

  info << u""_s << u"## Operating system"_s;
  info << u"* Name: %1"_s.arg(osName);
  info << u"* Version: %1"_s.arg(osVersion);
  info << u"* Pretty name: %1"_s.arg(QSysInfo::prettyProductName());
}

void
gatherPathInformation(QStringList &info) {
  info << u""_s << u"## Paths"_s;
#if defined(SYS_WINDOWS)
  info << u"* Installation type: %1"_s.arg(App::isInstalled() ? u"installed"_s : u"portable"_s);
#endif
  info << u"* Application directory: %1"_s.arg(QDir::toNativeSeparators(App::applicationDirPath()));
  info << u"* INI (preferences) file location: %1"_s.arg(QDir::toNativeSeparators(Util::Settings::iniFileName()));
  info << u"* Job queue location: %1"_s.arg(QDir::toNativeSeparators(Jobs::Job::queueLocation()));
  info << u"* Package data path: %1"_s.arg(QDir::toNativeSeparators(Q(mtx::sys::get_package_data_folder())));
  info << u"* Cache directory (via Qt): %1"_s.arg(QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));
}

void
gatherScreenInfo(QStringList &info) {
  info << u""_s << u"## Screens"_s;

  int idx = 0;
  for (auto const &screen : App::screens()) {
    info << u""_s << u"### Screen %1"_s.arg(idx++) << u""_s;
    info << u"* Device pixel ratio: %1"_s.arg(screen->devicePixelRatio());
    info << u"* Logical DPI: %1x%2"_s.arg(screen->logicalDotsPerInchX()).arg(screen->logicalDotsPerInchX());
    info << u"* Physical DPI: %1x%2"_s.arg(screen->physicalDotsPerInchX()).arg(screen->physicalDotsPerInchX());
    info << u"* Physical size: %1x%2"_s.arg(screen->physicalSize().width()).arg(screen->physicalSize().height());
    info << u"* Virtual size: %1x%2"_s.arg(screen->virtualSize().width()).arg(screen->virtualSize().height());
    info << u"* Geometry: %1x%2@%3x%4"_s.arg(screen->geometry().width()).arg(screen->geometry().height()).arg(screen->geometry().x()).arg(screen->geometry().y());
  }
}

void
gatherDesktopScalingAndThemeSettings([[maybe_unused]] QStringList &info) {
#if defined(SYS_WINDOWS)
  QSettings regDesktop{u"HKEY_CURRENT_USER\\Control Panel\\Desktop"_s, QSettings::NativeFormat};
  QSettings regPersonalize{u"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"_s, QSettings::NativeFormat};

  info << u""_s << u"## Desktop scaling & theme settings"_s << u""_s;

  info << u"* Scaling mode (`Win8DpiScaling`): %1"_s.arg(regDesktop.value("Win8DpiScaling", "not set").toString());
  info << u"* Scaling override (`DesktopDPIOverride`): %1"_s.arg(regDesktop.value("DesktopDPIOverride", "not set").toString());
  info << u"* System-wide scale factor (`LogPixels`): %1"_s.arg(regDesktop.value("LogPixels", "not set").toString());
  info << u"* Windows 11 application light theme (`AppsUseLightTheme`): %1"_s.arg(regPersonalize.value("AppsUseLightTheme", "not set").toString());
  info << u"* High contrast mode: %1"_s.arg(mtx::sys::is_high_contrast_enabled());
#endif
}

void
gatherEnvironmentVariables(QStringList &info) {
  info << u""_s << u"## Environment variables"_s << u""_s;

  auto keys = QStringList{} << u"QT_AUTO_SCREEN_SCALE_FACTOR"_s << u"QT_SCALE_FACTOR"_s << u"QT_SCREEN_SCALE_FACTORS"_s << u"QT_DEVICE_PIXEL_RATIO"_s << u"QT_SCALE_FACTOR_ROUNDING_POLICY"_s
                            << u"QT_STYLE_OVERRIDE"_s           << u"QT_QPA_PLATFORM"_s << u"QT_QPA_PLATFORMTHEME"_s    << u"QT_QPA_GENERIC_PLUGINS"_s
                            << u"MTX_LOGGER"_s                  << u"MTX_DEBUG"_s       << u"MKVTOOLNIX_DEBUG"_s        << u"MKVMERGE_DEBUG"_s
                            << u"LC_ALL"_s                      << u"LC_MESSAGES"_s     << u"LC_CTYPE"_s                << u"LANG"_s << u"LANGUAGE"_s;
  keys.sort();

  for (auto const &name : keys)
    info << u"* `%1=%2`"_s.arg(name).arg(Q(mtx::sys::get_environment_variable(to_utf8(name))));
}

void
gatherQtInfo(QStringList &info) {
  info << u""_s << u"# Qt"_s << u""_s;

  info << u"* Version: %1.%2.%3"_s.arg((QT_VERSION >> 16) & 0xff).arg((QT_VERSION >> 8) & 0xff).arg(QT_VERSION & 0xff);
  info << u"* Build ABI: %1"_s.arg(QSysInfo::buildAbi());

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
  QMediaFormat formats;

  QStringList lines;

  auto disclaimer = u"The following list is only relevant for the GUI's 'play sound after job completion' functionality and doesn't affect which formats can be multiplexed by mkvmerge."_s;

  for (auto const &format : formats.supportedAudioCodecs(QMediaFormat::Decode)) {
    auto name = format == QMediaFormat::AudioCodec::WMA         ? u"WMA"_s
              : format == QMediaFormat::AudioCodec::AC3         ? u"AC3"_s
              : format == QMediaFormat::AudioCodec::AAC         ? u"AAC"_s
              : format == QMediaFormat::AudioCodec::ALAC        ? u"ALAC"_s
              : format == QMediaFormat::AudioCodec::DolbyTrueHD ? u"DolbyTrueHD"_s
              : format == QMediaFormat::AudioCodec::EAC3        ? u"EAC3"_s
              : format == QMediaFormat::AudioCodec::MP3         ? u"MP3"_s
              : format == QMediaFormat::AudioCodec::Wave        ? u"Wave"_s
              : format == QMediaFormat::AudioCodec::Vorbis      ? u"Vorbis"_s
              : format == QMediaFormat::AudioCodec::FLAC        ? u"FLAC"_s
              : format == QMediaFormat::AudioCodec::Opus        ? u"Opus"_s
              :                                                   u"unknown (%1)"_s.arg(static_cast<int>(format));
    lines << u"* %1"_s.arg(name);
  }

  lines.sort();

  info << u""_s << u"## Supported audio codecs"_s << u""_s << disclaimer << u""_s;
  info += lines;

  lines.clear();

  for (auto const &format : formats.supportedFileFormats(QMediaFormat::Decode)) {
    auto name = format == QMediaFormat::WMA        ? u"WMA"_s
              : format == QMediaFormat::AAC        ? u"AAC"_s
              : format == QMediaFormat::Matroska   ? u"Matroska"_s
              : format == QMediaFormat::WMV        ? u"WMV"_s
              : format == QMediaFormat::MP3        ? u"MP3"_s
              : format == QMediaFormat::Wave       ? u"Wave"_s
              : format == QMediaFormat::Ogg        ? u"Ogg"_s
              : format == QMediaFormat::MPEG4      ? u"MPEG4"_s
              : format == QMediaFormat::AVI        ? u"AVI"_s
              : format == QMediaFormat::QuickTime  ? u"QuickTime"_s
              : format == QMediaFormat::WebM       ? u"WebM"_s
              : format == QMediaFormat::Mpeg4Audio ? u"Mpeg4Audio"_s
              : format == QMediaFormat::FLAC       ? u"FLAC"_s
              :                                      u"unknown (%1)"_s.arg(static_cast<int>(format));

    lines << u"* %1"_s.arg(name);
  }

  lines.sort();

  info << u""_s << u"## Supported file formats"_s << u""_s << disclaimer << u""_s;
  info += lines;
#endif
}

void
gatherCompilerAndLibraryInfo(QStringList &info) {
  info << u""_s << u"# Compiler and libraries"_s << u""_s;

#if defined(__clang__)
  info << u"* Compiler: clang++ %1.%2.%3"_s.arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
#elif defined(__GNUC__)
  info << u"* Compiler: g++ %1.%2.%3"_s.arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#else
  info << u"* Compiler: unknown"_s;
#endif

  info << u"* Boost: %1.%2.%3"_s.arg(BOOST_VERSION / 100'000).arg((BOOST_VERSION / 100) % 1'000).arg(BOOST_VERSION % 100);
  info << u"* cmark: %1"_s.arg(Q(CMARK_VERSION_STRING));
#if defined(HAVE_DVDREAD)
  info << u"* dvdread: %1.%2.%3"_s.arg(DVDREAD_VERSION / 10'000).arg((DVDREAD_VERSION / 100) % 100).arg(DVDREAD_VERSION % 100);
#endif
  info << u"* EBML: %1"_s.arg(Q(libebml::EbmlCodeVersion));
#if defined(HAVE_FLAC_FORMAT_H)
  info << u"* FLAC: %1"_s.arg(Q(FLAC__VERSION_STRING));
#endif
  info << u"* fmt: %1.%2.%3"_s.arg(FMT_VERSION / 10'000).arg((FMT_VERSION / 100) % 100).arg(FMT_VERSION % 100);
  info << u"* Matroska: %1"_s.arg(Q(libmatroska::KaxCodeVersion));
  info << u"* nlohmann-json: %1.%2.%3"_s.arg(NLOHMANN_JSON_VERSION_MAJOR).arg(NLOHMANN_JSON_VERSION_MINOR).arg(NLOHMANN_JSON_VERSION_PATCH);
  info << u"* pugixml: %1"_s.arg(formatPugixmlVersion());
  info << u"* Vorbis: %1"_s.arg(Q(vorbis_version_string()));
  info << u"* zlib: %1"_s.arg(Q(ZLIB_VERSION));
}

} // anonymous namespace

QString
gatherSystemInformation() {
  QStringList info;

  gatherGeneralInfo(info);

  info << u""_s << u"# System"_s;

  gatherOperatingSystemInfo(info);
  gatherPathInformation(info);
  gatherScreenInfo(info);
  gatherDesktopScalingAndThemeSettings(info);
  gatherEnvironmentVariables(info);

  gatherQtInfo(info);

  gatherCompilerAndLibraryInfo(info);

  return info.join(u"\n"_s);
}

}
