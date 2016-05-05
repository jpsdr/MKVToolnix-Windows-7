#include <memory>

#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>

int
main(int argc,
     char **argv) {
  auto app        = std::make_shared<QCoreApplication>(argc, argv);
  auto guiRE      = QRegularExpression{QString::fromUtf8("-gui(\\.exe)?$"), QRegularExpression::CaseInsensitiveOption};
  auto mkvinfoExe = QCoreApplication::applicationFilePath().replace(guiRE, QString{});
  auto args       = app->arguments();

  // On Windows the first argument may or may not be the current
  // executable.
  if (!args.isEmpty() && args.at(0).contains(guiRE))
    args.removeFirst();

  args.push_front(QString::fromUtf8("--gui"));

  QProcess::startDetached(mkvinfoExe, args);

  return 0;
}
