#include "cmdutil.h"
#include "packager.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QProcess>
#include <QString>
#include <QThread>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QCoreApplication::setSetuidAllowed(true);
#if (QT_VERSION >= 0x050000 && QT_VERSION < 0x060000)
  QString qtDir = QLibraryInfo::location(QLibraryInfo::ArchDataPath);
  QString qtLibsDir = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
#endif
#if (QT_VERSION >= 0x060000)
  QString qtDir = QLibraryInfo::path(QLibraryInfo::ArchDataPath);
  QString qtLibsDir = QLibraryInfo::path(QLibraryInfo::LibrariesPath);
#endif

  QString workDirectorty = QCoreApplication::applicationDirPath();

  QCommandLineParser parser;
  parser.addHelpOption();
  QCommandLineOption pathOption(QStringList() << "p"
                                              << "path",
                                "设置app位置", "app path");
  parser.addOption(pathOption);

  QCommandLineOption qmakeOption(QStringList() << "qmake",
                                "设置qmake位置", "qmake path");
  parser.addOption(qmakeOption);


  QCommandLineOption pidOption(QStringList() << "P"
                                             << "pid",
                               "设置进程id,P为大写", "pid");
  parser.addOption(pidOption);
  QCommandLineOption qmlOption(QStringList() << "q"
                                             << "qml",
                               "设置qml路径", "path1 path2...");
  parser.addOption(qmlOption);
  QCommandLineOption watchOption("watch", "运行时动态收集so");
  parser.addOption(watchOption);
  QCommandLineOption execOption("exec", "时候运行了libexec下的进程");
  parser.addOption(execOption);
  parser.process(app);

  if (!parser.isSet(pathOption)) {
    qDebug() << "请输入必要的参数，如：--path /your/app/path";
    app.exit();
    return 0;
  }
  if (!parser.isSet(qmakeOption)) {
    qDebug() << "请输入必要的参数，如：--path /your/app/path --qmake /your/qmake/path";
    app.exit();
    return 0;
  }

  QString appPathStr = parser.value(pathOption);
  QFileInfo appPath(appPathStr);
  if (!appPath.exists()) {
    qDebug() << "没有对应的app文件";
    app.exit();
    return 0;
  }

  QString qmakePathStr = parser.value(qmakeOption);
  QFileInfo qmakePath(qmakePathStr);
  if (!qmakePath.exists()) {
    qDebug() << "没有对应的qmake文件";
    app.exit();
    return 0;
  }

  QString output, error;

  CmdUtil::execShell("whoami", output, error);
  if (output != "root\n") {
    qDebug() << output;
    qDebug() << "请在root下运行";
    app.exit();
    return 0;
  }
  CmdUtil::execShell("which patchelf", output, error);
  if (output == "") {
    qDebug() << "which patchelf" << output;
    qDebug() << "请安装patchelf后再运行";
    app.exit();
    return 0;
  }

  Packager packager(appPath,qmakePath);
  QStringList qmlPaths = parser.values(qmlOption);
  packager.setQmlPaths(qmlPaths);

  if (parser.isSet(execOption)) {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--no-sandbox");
    packager.packWithWenengine();
  }

  if (parser.isSet(watchOption)) {
    packager.watchPack();
  } else if (parser.isSet(pidOption)) {
    packager.watchPack(parser.value(pidOption));
  } else {
    packager.pathPack();
  }

  return app.exec();
}
