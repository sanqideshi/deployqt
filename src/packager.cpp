#include "packager.h"
#include "cmdutil.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QList>
#include <QProcess>
#include <QThread>
Packager::Packager(QFileInfo appPath, QFileInfo qmakePath, QObject *parent)
    : QObject{parent}, appPath(appPath), qmakePath(qmakePath), hasExec(false)
{
    qDebug() << "Packager init ...";
    thisAppName = QCoreApplication::applicationName();
    QString appDir = appPath.dir().path();
    initVars(qmakePath);

    appName = appPath.fileName();
    outputPath = "./output/" + appName + "/";
    outLibexecPath = outputPath + "libexec";
    outTranslationsPath = outputPath + "translations";
    outResourcesPath = outputPath + "resources";
    outTranslationsPath2 = outputPath + "libexec/translations";
    outResourcesPath2 = outputPath + "libexec/resources";

    qDebug() << outputPath << Qt::endl;
    qDebug() << outLibexecPath << Qt::endl;
    qDebug() << outTranslationsPath << Qt::endl;
    qDebug() << outResourcesPath << Qt::endl;
    qDebug() << outTranslationsPath2 << Qt::endl;
    qDebug() << outResourcesPath2 << Qt::endl;
    qDebug() << qtLibexecPath << Qt::endl;
    qDebug() << qtResourcesPath << Qt::endl;
    qDebug() << qtTranslationsPath << Qt::endl;
}
void Packager::initVars(QFileInfo qmakePath)
{
    QString output, error;
    QString qmakeRealPath = qmakePath.absoluteFilePath();
    // exec "qmake -query"
    CmdUtil::execShell(QString::fromLatin1("%1 -query").arg(qmakeRealPath), output, error);
    qDebug() << "initVars : " << output;
    QStringList arr = output.split("\n");
    for (auto &&str : arr)
    {
        QStringList qtArr = str.split(":");
        if (str.contains("QT_INSTALL_ARCHDATA"))
            qtDir = qtArr[1];
        if (str.contains("QT_INSTALL_DATA"))
            qtDataPath = qtArr[1];
        if (str.contains("QT_INSTALL_LIBS"))
            qtLibsDir = qtArr[1];
        if (str.contains("QT_INSTALL_LIBEXECS"))
            qtLibexecPath = qtArr[1];
        if (str.contains("QT_INSTALL_PLUGINS"))
            qtPluginsPath = qtArr[1];
        if (str.contains("QT_INSTALL_TRANSLATIONS"))
            qtTranslationsPath = qtArr[1];
    }
    qtResourcesPath = qtDataPath + "/resources";
}

void Packager::pathPack()
{
    ArchEnum arch = getArch();
    runApp(appPath.absoluteFilePath());
    QString pid = getPid();
    QString plddStr = getPlddApp(pid);
    soPaths = QSharedPointer<QSet<QString>>(new QSet<QString>());
    collectSo(plddStr);
    QString output, error;
    CmdUtil::execShell(QString::fromLatin1("kill -9 %1").arg(pid), output, error);
    copySo();
    copyQml();
    copyExtraFiles(arch);
    patchelf(arch, outputPath, appName);

    makeFiles();
    soPaths->clear();

    chmodX();
    mvSo();
    // emit execWebengine();
    //  emit exitSignal();
    //  QCoreApplication::exit(0);
}

void Packager::watchPack()
{
    // ArchEnum arch = getArch();
    runApp(appPath.absoluteFilePath() + " ");
    QString pids = getPid();
    QStringList pidArr = pids.split("\n");
    qDebug() << "pid:" << pidArr[0];
    watchPack(pidArr[0]);
}

void Packager::watchPack(QString pid)
{
    ArchEnum arch = getArch();
    soPaths = QSharedPointer<QSet<QString>>(new QSet<QString>());

    connect(&controlThread, &ControlThread::mstop, this, [=]()
            {
    qDebug() << "collectthread is stoped";
    collectthread.setFlag(false); });
    connect(&controlThread, &ControlThread::mexit, this, [=]()
            {
    qDebug() << "collectthread is mexit";
    chownToUser();
    removeDebugFiles();
    QCoreApplication::exit(0); });

    connect(&collectthread, &CollectThread::collect, this, [=]()
            {

    mutex.lock();
    QString plddStr = getPlddApp(pid);

    collectSo(plddStr);
    mutex.unlock(); });
    connect(&collectthread, &CollectThread::finished, this, [=]()
            {
                qDebug() << "finished";
                QString output, error;
                CmdUtil::execShell(QString::fromLatin1("kill -9 %1").arg(pid), output,
                                   error);
                copySo();
                copyQml();
                copyExtraFiles(arch);
                patchelf(arch, outputPath, appName);

                makeFiles();
                soPaths->clear();

                chmodX();
                mvSo();
                emit execWebengine();
                // QCoreApplication::exit(0);
            });

    controlThread.start();
    collectthread.start();
}

void Packager::packWithWenengine()
{

    connect(&collectthread, &CollectThread::collect, this, [=]()
            {
 

    mutex.lock();
    QString pidStr =
        QString::fromLatin1(
            "ps aux | grep %1 | grep -v grep | grep -v %2| awk '{print $2}'")
            .arg("QtWebEngineProcess")
            .arg(thisAppName);
    QString output;
    QString error;
    CmdUtil::execShell(pidStr, output, error);
    // QString pids = output;
    QStringList pidArr = output.split("\n");
    QString webEnginePid = pidArr[0];

    //qDebug() << "webEnginePid:" << webEnginePid;
    QString plddStr = getPlddApp(webEnginePid);
    collectSo(plddStr);
    mutex.unlock(); });

    connect(this, &Packager::execWebengine, [=]()
            {
    qDebug() << "execWebengine";
    ArchEnum arch = getArch();
    QString output, error;
    QDir dir;
    dir.mkpath(outLibexecPath);
    // dir.mkpath(outResourcesPath2);
    // dir.mkpath(outTranslationsPath2);
    dir.mkpath(outResourcesPath);
    dir.mkpath(outTranslationsPath);
    QString cpLibexec =
        QString::fromLatin1("cp -rfL %1 %2").arg(qtLibexecPath).arg(outputPath);
    QString cpResources = QString::fromLatin1("cp -rfL %1 %2")
                              .arg(qtResourcesPath)
                              .arg(outputPath);
    QString cpTranslations = QString::fromLatin1("cp -rfL %1 %2")
                                 .arg(qtTranslationsPath)
                                 .arg(outputPath);

    CmdUtil::execShell(cpLibexec, output, error);
    CmdUtil::execShell(cpResources, output, error);
    CmdUtil::execShell(cpTranslations, output, error);

    CmdUtil::execShell("ls " + qtLibexecPath, output, error);
    qDebug() << "libexec output:" << output;
    QStringList libexecArr = output.split("\n");

    for (auto libexec : libexecArr) {
      qDebug() << "libexec:" << libexec;
      if (libexec != "") {
        patchelf(arch, outputPath + "libexec/", libexec);
      }
    }
    // QString cpResources2 = QString::fromLatin1("cp -rfL %1
    // %2").arg(qtResourcesPath).arg(outLibexecPath); QString cpTranslations2 =
    // QString::fromLatin1("cp -rfL %1
    // %2").arg(qtTranslationsPath).arg(outLibexecPath);
    qDebug() << cpLibexec;
    qDebug() << cpResources;
    qDebug() << cpTranslations;
    // qDebug() <<cpResources2;
    // qDebug() <<cpTranslations2;
    // CmdUtil::execShell(cpResources2,output,error);
    // CmdUtil::execShell(cpTranslations2,output,error);
    // CmdUtil::execShell("cp -rfL " + outputPath + "/qt.conf " + outputPath +
    // "/libexec/",output,error); 将libexec/qt.conf 内容替换 制作qt.conf文件
    QFile qtConfFile(":/qt.conf.template2");
    qtConfFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray qtConfByteArr = qtConfFile.readAll();
    qtConfFile.close();
    QFile qtConf(outputPath + "/libexec/qt.conf");
    qtConf.open(QFile::OpenModeFlag::ReadWrite);
    qtConf.write(qtConfByteArr);
    qtConf.close(); });
}

void Packager::setQmlPaths(QStringList qmlPaths) { this->qmlPaths = qmlPaths; }

bool Packager::getHasExec() const { return hasExec; }

void Packager::setHasExec(bool newHasExec)
{
    hasExec = newHasExec;
    // packWithWenengine();
}

const QString &Packager::getThisAppName() const { return thisAppName; }

void Packager::setThisAppName(const QString &newThisAppName)
{
    thisAppName = newThisAppName;
}

const QString &Packager::getQtLibsDir() const { return qtLibsDir; }

void Packager::setQtLibsDir(const QString &newQtLibsDir)
{
    qtLibsDir = newQtLibsDir;
}

const QString &Packager::getQtDir() const { return qtDir; }

void Packager::setQtDir(const QString &newQtDir) { qtDir = newQtDir; }

const QString &Packager::getOutputPath() const { return outputPath; }

void Packager::setOutputPath(const QString &newOutputPath)
{
    outputPath = newOutputPath;
}

const QString &Packager::getAppName() const { return appName; }

void Packager::setAppName(const QString &newAppName) { appName = newAppName; }

ArchEnum Packager::getArch()
{

    QString output;
    QString error;
    CmdUtil::execShell("uname -a", output, error);

    ArchEnum arch = ArchEnum::X86_64;
    if (output.contains("x86_64"))
    {
        arch = ArchEnum::X86_64;
    }

    if (output.contains("aarch64"))
    {
        arch = ArchEnum::aarch64;
    }
    return arch;
}

void Packager::runApp(QString appPathStr)
{

    QString output;
    QString error;
    QStringList argument;
    argument << "-c" << appPathStr;
    QProcess::startDetached("/usr/bin/bash", argument);
    // CmdUtil::execShell(appPathStr,output,error);
    QThread::sleep(2);
}

QString Packager::getPid()
{
    QString pidStr =
        QString::fromLatin1(
            "ps aux | grep %1 | grep -v grep | grep -v %2| awk '{print $2}'")
            .arg(appName)
            .arg(thisAppName);
    QString output;
    QString error;
    CmdUtil::execShell(pidStr, output, error);
    QString pid = output;
    return pid;
}

QString Packager::getPlddApp(QString pid)
{
    QString output;
    QString error;
    CmdUtil::execShell(QString::fromLatin1("pldd %1").arg(pid), output, error);

    return output;
}

void Packager::collectSo(QString plddStr)
{
    // QString plddStr = getPlddApp(pid);
    QStringList libsStr = plddStr.split("\n");

    QDir dir;
    dir.mkpath("./output/" + appName);
    // QString outputPath = "./output/"+appName+"/";

    // setOutputPath(outputPath);
    for (int i = 0; i < libsStr.length(); i++)
    {

        QString lib = libsStr[i];
        QFileInfo fileInfo(lib);
        if (!fileInfo.isFile())
        {
            continue;
        }
        // 真实so文件
        // QString realLib = fileInfo.canonicalFilePath();
        QString realLib = fileInfo.filePath();
        if (!soPaths->contains(realLib))
        {
            qDebug() << "realLib add:" << realLib;
            soPaths->insert(realLib);
        }
    }
}

void Packager::copySo()
{

    QList<QString> list = soPaths->values();

    QDir dir;
    dir.mkpath(outputPath + "./lib");
    dir.mkpath(outputPath + "./lib/opt");
    // QString outputPath = getOutputPath();
    for (int i = 2; i < list.size(); i++)
    {
        QString realLib = list.at(i);

        // if(realLib.startsWith("/lib")){
        //     realLib = realLib.replace(0,4,"/usr/lib");
        // }
        qDebug() << "realLib:" << realLib;
        QFileInfo fileInfo(realLib);
        bool isQtLibPrefix = realLib.startsWith(qtLibsDir);
        bool isQtDirPrefix = realLib.startsWith(qtDir);

        // QString fileName = fileInfo.baseName()+".so";

        QString fileName = fileInfo.fileName();
        // QString newName =outputPath + "./lib/" + fileName;

        // QFile::copy(realLib,newName);
        if (isQtDirPrefix)
        {
            QString mkpath = realLib;
            mkpath = "." + mkpath.replace(0, qtDir.length(), "");
            mkpath = mkpath.replace(mkpath.indexOf(fileName), mkpath.length(), "");
            dir.mkpath(outputPath + mkpath);
            QString newName = outputPath + mkpath + fileName;

            QFile::copy(realLib, newName);
            // delete mkpath;
        }
        else
        {
            QString newName = outputPath + "./lib/" + fileName;
            QFile::copy(realLib, newName);
        }
        // delete realLib;
    }
}

void Packager::copyExtraFiles(ArchEnum arch)
{
    QString archPrefix, output, error;
    if (arch == ArchEnum::X86_64)
    {
        archPrefix = "x86_64-linux-gnu";
    }
    if (arch == ArchEnum::aarch64)
    {
        archPrefix = "aarch64-linux-gnu";
    }
    // 复制传入的app
    CmdUtil::execShell("cp -rfL " + appPath.absoluteFilePath() + " " + outputPath,
                       output, error);
    // 复制libqxcb.so qtPluginsPath
    //  /usr/lib/%2/qt5/plugins/platforms/libqxcb.so %1/plugins/platforms/
    CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2")
                           .arg(qtPluginsPath + "/platforms/libqxcb.so")
                           .arg(outputPath + "plugins/platforms/libqxcb.so"),
                       output, error);
    // 复制libpthread.so.0 librt.so.1
    CmdUtil::execShell(
        QString::fromLatin1("cp -rfL /usr/lib/%2/libpthread.so.0 %1")
            .arg(outputPath + "./lib/")
            .arg(archPrefix),
        output, error);
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%2/librt.so.1 %1")
                           .arg(outputPath + "./lib/")
                           .arg(archPrefix),
                       output, error);
}

void Packager::patchelf(ArchEnum arch, QString outputPath, QString appName)
{

    QString ldName, output, error;
    QString archPrefix;
    if (arch == ArchEnum::X86_64)
    {
        archPrefix = "x86_64-linux-gnu";
    }
    if (arch == ArchEnum::aarch64)
    {
        archPrefix = "aarch64-linux-gnu";
    }

    if (arch == ArchEnum::X86_64)
    {
        ldName = "ld-linux-x86-64.so.2";
    }
    if (arch == ArchEnum::aarch64)
    {
        ldName = "ld-linux-aarch64.so.1";
    }

    // 复制ld-linux-xxx到目录
    // CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%1/%2
    // %3").arg(archPrefix).arg(ldName).arg(outputPath+"./lib/"),output,error);
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%1/%2 %3")
                           .arg(archPrefix)
                           .arg(ldName)
                           .arg(outputPath),
                       output, error);

    qDebug() << "output1:" << error;
    CmdUtil::execShell("patchelf --set-rpath ./lib " + outputPath + appName,
                       output, error);
    qDebug() << "output2:" << error;
    // CmdUtil::execShell(QString::fromLatin1("patchelf --set-interpreter ./lib/%1
    // %2").arg(ldName).arg(outputPath+appName),output,error);
    CmdUtil::execShell(QString::fromLatin1("patchelf --set-interpreter ./%1 %2")
                           .arg(ldName)
                           .arg(outputPath + appName),
                       output, error);
    qDebug() << "output3:" << error;
}

void Packager::makeFiles()
{
    // 制作app.sh文件
    QFile depFile(":/deploy.sh");
    depFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray byteArr = depFile.readAll();
    depFile.close();
    QString all(byteArr);
    all = all.arg(appName);

    QFile appSh(outputPath + appName + ".sh");
    appSh.open(QFile::OpenModeFlag::ReadWrite);
    appSh.write(all.toLatin1());
    appSh.close();

    // 制作qt.conf文件
    QFile qtConfFile(":/qt.conf.template");
    qtConfFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray qtConfByteArr = qtConfFile.readAll();
    qtConfFile.close();
    QString qtConfText(qtConfByteArr);

    // qDebug() << "qtConfText:" << qtConfText;
    QFile qtConf(outputPath + "qt.conf");
    qtConf.open(QFile::OpenModeFlag::ReadWrite);
    qtConf.write(qtConfText.toLatin1());
    qtConf.close();
}

void Packager::copyQml()
{
    if (qmlPaths.isEmpty())
    {
        return;
    }
    QString output, error;
    QDir dir;
    dir.mkpath(outputPath + "./qml");
    qDebug() << "copyQml : "
             << QString::fromLatin1("cp -rfL %1 %2")
                    .arg(qtDir + "/qml/*")
                    .arg(outputPath + "./qml/");
    CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2")
                           .arg(qtDir + "/qml/*")
                           .arg(outputPath + "./qml/"),
                       output, error);
    for (QString qmlPath : qmlPaths)
    {
        CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2")
                               .arg(qmlPath + "/*.qml")
                               .arg(outputPath + "./qml/"),
                           output, error);
    }
}

void Packager::chmodX()
{
    QString output, error;
    CmdUtil::execShell(QString::fromLatin1("chmod a+x %1")
                           .arg(outputPath + appPath.fileName() + ".sh"),
                       output, error);
}

void Packager::cpExec()
{
    QString output, error;
    CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2")
                           .arg(qtDir + "/libexec/")
                           .arg(outputPath),
                       output, error);
}

void Packager::mvSo()
{
    QStringList mvSoList;
    mvSoList << "libdrm*";
    mvSoList << "libGL*";
    mvSoList << "libGLdispatch*";
    mvSoList << "libGLX*";
    mvSoList << "libGLX_mesa*";
    mvSoList << "libpciaccess*";
    mvSoList << "libudev.so*";
    mvSoList << "libusbmuxd*";
    mvSoList << "libX11.so*";
    mvSoList << "libX11-xcb.so*";
    mvSoList << "radeonsi_dri*";
    mvSoList << "libxkbcommon-x11*";
    mvSoList << "libKF5*";
    QString output, error;
    for (auto &&str : mvSoList)
    {
        CmdUtil::execShell(QString::fromLatin1("mv %1 %2")
                               .arg(outputPath + "lib/" + str)
                               .arg(outputPath + "lib/opt/"),
                           output, error);
    }
}

QString Packager::getUserName()
{
    QString output, error;
    CmdUtil::execShell(
        QString::fromLatin1("getent passwd `who` | head -n 1 | cut -d : -f 1"),
        output, error);
    qDebug() << "getUserName: " << output << error;
    return output.trimmed();
}

void Packager::chownToUser()
{
    QString output, error;
    QString userName = getUserName();
    CmdUtil::execShell(QString::fromLatin1("chown %1:%1 %2 %3")
                           .arg(userName)
                           .arg("./output")
                           .arg("-R"),
                       output, error);
    qDebug() << "chownToUser: " << output << error;
}

void Packager::removeDebugFiles()
{
    QString output, error;
    QString userName = getUserName();
    CmdUtil::execShell(
        QString::fromLatin1("find '%1' -type f -name '*debug' -exec rm -f {} \\;")
            .arg(outputPath),
        output, error);
    qDebug() << "removeDebugFiles: " << output << error;
}
