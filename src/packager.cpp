#include "cmdutil.h"
#include "packager.h"
#include <QDebug>
#include <QDir>
#include <QList>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>
#include <QLibraryInfo>
Packager::Packager(QFileInfo appPath,QObject *parent)
    : QObject{parent},appPath(appPath),hasExec(false)
{
    thisAppName = QCoreApplication::applicationName();
#if (QT_VERSION >= 0x050000 && QT_VERSION<0x060000)
    qtDir = QLibraryInfo::location(QLibraryInfo::ArchDataPath);
    qtLibsDir =  QLibraryInfo::location(QLibraryInfo::LibrariesPath);
#endif
#if (QT_VERSION >= 0x060000)
    qtDir = QLibraryInfo::path(QLibraryInfo::ArchDataPath);
    qtLibsDir =  QLibraryInfo::path(QLibraryInfo::LibrariesPath);
#endif

    appName = appPath.fileName();
    outputPath = "./output/"+appName+"/";
}

void Packager::pathPack()
{
    ArchEnum arch = getArch();
    runApp(appPath.absoluteFilePath());
    QString pid =  getPid(); 
    QString plddStr = getPlddApp(pid);
    soPaths = QSharedPointer<QSet<QString>>(new QSet<QString>());
    collectSo(plddStr);
    QString output,error;
    CmdUtil::execShell(QString::fromLatin1("kill -9 %1").arg(pid),output,error);
    copySo();
    copyQml();
    copyExtraFiles(arch);
    patchelf(arch);

    makeFiles();
    soPaths->clear();

    chmodX();
    QCoreApplication::exit(0);
}



void Packager::watchPack()
{
     //ArchEnum arch = getArch();
     runApp(appPath.absoluteFilePath());
     QString pid =  getPid();
     watchPack(pid);
}

void Packager::watchPack(QString pid)
{
    ArchEnum arch = getArch();
    soPaths = QSharedPointer<QSet<QString>>(new QSet<QString>());
    QThread::sleep(2);

   connect(&controlThread,&ControlThread::mstop,this,[=](){
       qDebug() << "collectthread is stoped";
       collectthread.setFlag(false);
   });
   connect(&collectthread,&CollectThread::collect,this,[=](){
       qDebug() << "collect";
       QString plddStr = getPlddApp(pid);

       collectSo(plddStr);
   });
   connect(&collectthread,&CollectThread::finished,this,[=](){
       qDebug() << "finished";
       QString output,error;
       CmdUtil::execShell(QString::fromLatin1("kill -9 %1").arg(pid),output,error);
       copySo();
       copyQml();
       copyExtraFiles(arch);
       patchelf(arch);

       makeFiles();
       soPaths->clear();


       chmodX();

       QCoreApplication::exit(0);
   });
   controlThread.start();
   collectthread.start();

}

void Packager::packWithWenengine()
{
    watchPack();
    cpExec();
}

void Packager::setQmlPaths(QStringList qmlPaths)
{
    this->qmlPaths = qmlPaths;
}

bool Packager::getHasExec() const
{
    return hasExec;
}

void Packager::setHasExec(bool newHasExec)
{
    hasExec = newHasExec;
}

const QString &Packager::getThisAppName() const
{
    return thisAppName;
}

void Packager::setThisAppName(const QString &newThisAppName)
{
    thisAppName = newThisAppName;
}

const QString &Packager::getQtLibsDir() const
{
    return qtLibsDir;
}

void Packager::setQtLibsDir(const QString &newQtLibsDir)
{
    qtLibsDir = newQtLibsDir;
}

const QString &Packager::getQtDir() const
{
    return qtDir;
}

void Packager::setQtDir(const QString &newQtDir)
{
    qtDir = newQtDir;
}

const QString &Packager::getOutputPath() const
{
    return outputPath;
}

void Packager::setOutputPath(const QString &newOutputPath)
{
    outputPath = newOutputPath;
}

const QString &Packager::getAppName() const
{
    return appName;
}

void Packager::setAppName(const QString &newAppName)
{
    appName = newAppName;
}

ArchEnum Packager::getArch()
{

    QString output;
    QString error;
    CmdUtil::execShell("uname -a",output,error);

    ArchEnum arch= ArchEnum::X86_64;
    if(output.contains("x86_64")){
        arch = ArchEnum::X86_64;
    }

    if(output.contains("aarch64")){
        arch = ArchEnum::aarch64;
    }
    return arch;
}

void Packager::runApp(QString appPathStr)
{

    QString output;
    QString error;
    QStringList argument;
    argument << "-c"<<appPathStr;
    QProcess::startDetached("/usr/bin/bash",argument);
    //CmdUtil::execShell(appPathStr,output,error);
    QThread::sleep(2);
}

QString Packager::getPid()
{
    QString pidStr = QString::fromLatin1("ps aux | grep %1 | grep -v grep | grep -v %2| awk '{print $2}'").arg(appName).arg(thisAppName);
    QString output;
    QString error;
    CmdUtil::execShell(pidStr,output,error);
    QString pid = output;
    return pid;
}

QString Packager::getPlddApp(QString pid)
{
    QString output;
    QString error;
    CmdUtil::execShell(QString::fromLatin1("pldd %1").arg(pid),output,error);

    return output;
}

void Packager::collectSo(QString plddStr)
{
    //QString plddStr = getPlddApp(pid);
    QStringList libsStr = plddStr.split("\n");

    QDir dir;
    dir.mkpath("./output/"+appName);
    //QString outputPath = "./output/"+appName+"/";

    //setOutputPath(outputPath);
    for(int i=0;i<libsStr.length();i++){

        QString lib = libsStr[i];
        QFileInfo fileInfo(lib);
        if(!fileInfo.isFile()){
            continue;
        }
        //真实so文件
        //QString realLib = fileInfo.canonicalFilePath();
        QString realLib = fileInfo.filePath();
        if(!soPaths->contains(realLib)){
            qDebug() << "realLib add:" <<realLib;
            soPaths->insert(realLib);
        }


    }
}

void Packager::copySo()
{

    QList<QString> list = soPaths->values();

    QDir dir;
    dir.mkpath(outputPath+"./lib/opt");
    //QString outputPath = getOutputPath();
    for(int i=2;i<list.size();i++){
        QString realLib = list.at(i);

        if(realLib.startsWith("/lib")){
            realLib = realLib.replace(0,4,"/usr/lib");
        }
        qDebug()<<"realLib:" <<realLib;
        QFileInfo fileInfo(realLib);
        bool isQtLibPrefix = realLib.startsWith(qtLibsDir);
        bool isQtDirPrefix = realLib.startsWith(qtDir);


        //QString fileName = fileInfo.baseName()+".so";

        QString fileName = fileInfo.fileName();

        if(isQtDirPrefix){
            QString mkpath = realLib;
            mkpath = "." + mkpath.replace(0,qtDir.length(),"");
            mkpath = mkpath.replace(mkpath.indexOf(fileName),mkpath.length(),"");
            dir.mkpath(outputPath+mkpath);
            QString newName =outputPath + mkpath + fileName;

            QFile::copy(realLib,newName);
            //delete mkpath;
        }else if(isQtLibPrefix){
            dir.mkpath(outputPath + "./lib");
            QString newName =outputPath + "./lib/" + fileName;

            QFile::copy(realLib,newName);
        }else {
            QString newName =outputPath + "./lib/opt/" + fileName;
            QFile::copy(realLib,newName);
        }
        //delete realLib;
    }
}

void Packager::copyExtraFiles(ArchEnum arch)
{
    QString archPrefix,output,error;
    if(arch == ArchEnum::X86_64){
        archPrefix = "x86_64-linux-gnu";
    }
    if(arch == ArchEnum::aarch64){
        archPrefix = "aarch64-linux-gnu";
    }
    //复制传入的app
    CmdUtil::execShell("cp -rfL "+appPath.absoluteFilePath()+ " " + outputPath,output,error);
    //复制libqxcb.so
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%2/qt5/plugins/platforms/libqxcb.so %1/plugins/platforms/").arg(outputPath).arg(archPrefix),output,error);
    //复制libpthread.so.0 librt.so.1
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%2/libpthread.so.0 %1").arg(outputPath + "./lib/").arg(archPrefix),output,error);
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%2/librt.so.1 %1").arg(outputPath + "./lib/").arg(archPrefix),output,error);
}

void Packager::patchelf(ArchEnum arch)
{

    QString ldName,output,error;
    QString archPrefix;
    if(arch == ArchEnum::X86_64){
        archPrefix = "x86_64-linux-gnu";
    }
    if(arch == ArchEnum::aarch64){
        archPrefix = "aarch64-linux-gnu";
    }

    if(arch == ArchEnum::X86_64){
        ldName = "ld-linux-x86-64.so.2";
    }
    if(arch == ArchEnum::aarch64){
        ldName = "ld-linux-aarch64.so.1";
    }

    //复制ld-linux-xxx到目录
    //CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%1/%2 %3").arg(archPrefix).arg(ldName).arg(outputPath+"./lib/"),output,error);
    CmdUtil::execShell(QString::fromLatin1("cp -rfL /usr/lib/%1/%2 %3").arg(archPrefix).arg(ldName).arg(outputPath),output,error);
    CmdUtil::execShell("patchelf --set-rpath ./lib " + outputPath+appName,output,error);
    //CmdUtil::execShell(QString::fromLatin1("patchelf --set-interpreter ./lib/%1 %2").arg(ldName).arg(outputPath+appName),output,error);
    CmdUtil::execShell(QString::fromLatin1("patchelf --set-interpreter ./%1 %2").arg(ldName).arg(outputPath+appName),output,error);
}

void Packager::makeFiles()
{
    //制作app.sh文件
    QFile depFile(":/deploy.sh");
    depFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray byteArr = depFile.readAll();
    depFile.close();
    QString all(byteArr);
    all = all.arg(appName);

    QFile appSh(outputPath+appName+".sh");
    appSh.open(QFile::OpenModeFlag::ReadWrite);
    appSh.write(all.toLatin1());
    appSh.close();

    //制作qt.conf文件
    QFile qtConfFile(":/qt.conf.template");
    qtConfFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray qtConfByteArr = qtConfFile.readAll();
    qtConfFile.close();
    QString qtConfText(qtConfByteArr);

    //qDebug() << "qtConfText:" << qtConfText;
    QFile qtConf(outputPath+"qt.conf");
    qtConf.open(QFile::OpenModeFlag::ReadWrite);
    qtConf.write(qtConfText.toLatin1());
    qtConf.close();
}

void Packager::copyQml()
{
    if(qmlPaths.isEmpty()){
        return;
    }
    QString output,error;
    QDir dir;
    dir.mkpath(outputPath+"./qml");
    qDebug() << "copyQml : " <<QString::fromLatin1("cp -rfL %1 %2").arg(qtDir+"/qml/*").arg(outputPath+"./qml/");
    CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2").arg(qtDir+"/qml/*").arg(outputPath+"./qml/"),output,error);
    for(QString qmlPath: qmlPaths){
        CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2").arg(qmlPath+"/*").arg(outputPath+"./qml/"),output,error);
    }
}

void Packager::chmodX()
{
    QString output,error;
    CmdUtil::execShell(QString::fromLatin1("chmod a+x %1").arg(outputPath + appPath.fileName()+".sh"),output,error);
}

void Packager::cpExec()
{
    QString output,error;
    CmdUtil::execShell(QString::fromLatin1("cp -rfL %1 %2").arg(qtDir+"/libexec/").arg(outputPath),output,error);
}
