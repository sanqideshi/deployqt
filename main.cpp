#include <QCoreApplication>
#include <QLibraryInfo>
#include <QDebug>
#include <QString>
#include "cmdutil.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QThread>
#include <QProcess>
enum ArchEnum{
  X86_64,
  aarch64
};
int main1(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setSetuidAllowed(true);
    //qDebug() << QLibraryInfo::location(QLibraryInfo::PrefixPath);
    QString qtDir = QLibraryInfo::location(QLibraryInfo::ArchDataPath);
    QString qtLibsDir =  QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    QString workDirectorty = QCoreApplication::applicationDirPath();

    QCommandLineParser parser;
    QCommandLineOption option1("path");
    option1.setDescription("设置位置");
    option1.setValueName("path");
    parser.setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode::ParseAsLongOptions);
    parser.addOption(option1);

    parser.parse(a.arguments());

    if(!parser.isSet(option1)){
        qDebug() << "请输入必要的参数，如：--path /your/app/path";
        a.exit();
        return 0;
    }

    QString appPathStr = parser.value(option1);
    QFileInfo appPath(appPathStr);
    if(!appPath.exists()){
        qDebug() << "没有对应的文件";
        a.exit();
        return 0;
    }
    QString output;
    QString error;
    QStringList argument;
    argument << "-c"<<appPathStr;
    QProcess::startDetached("/usr/bin/bash",argument);
    //CmdUtil::execShell(appPathStr,output,error);
    QThread::sleep(2);
    CmdUtil::execShell("uname -a",output,error);

    int arch= ArchEnum::X86_64;
    if(output.contains("x86_64")){
        arch = ArchEnum::X86_64;
    }

    if(output.contains("arm64")){
        arch = ArchEnum::aarch64;
    }

    QString applicationName = appPath.fileName();

    QString pidStr = QString::fromLatin1("ps aux | grep %1 | grep -v grep | grep -v deploy_qt| awk '{print $2}'").arg(applicationName);



    CmdUtil::execShell(workDirectorty,pidStr,output,error);
    QString pid = output;
    CmdUtil::execShell(workDirectorty,QString::fromLatin1("pldd %1").arg(pid),output,error);
    QStringList libsStr = output.split("\n");

    QDir dir;
    dir.mkpath("./output/"+applicationName);
    QString outputPath = "./output/"+applicationName+"/";

    for(int i=2;i<libsStr.length();i++){
        QString lib = libsStr[i];
        QFileInfo fileInfo(lib);
        QString realLib = fileInfo.canonicalFilePath();
        bool isQtLibPrefix = realLib.startsWith(qtLibsDir);
        bool isQtDirPrefix = realLib.startsWith(qtDir);


        //QString fileName = fileInfo.baseName()+".so";

        QString fileName = fileInfo.fileName();

        if(isQtDirPrefix){
            QString mkpath = realLib;
            mkpath = "." + mkpath.replace(0,qtDir.length(),"");
            mkpath = mkpath.replace(mkpath.indexOf(fileName),mkpath.length(),"");
            dir.mkpath(outputPath +mkpath);
            QString newName =outputPath + mkpath + fileName;

            QFile::copy(realLib,newName);
        }else if(isQtLibPrefix){
            dir.mkpath(outputPath + "./lib");
            QString newName =outputPath + "./lib/" + fileName;

            QFile::copy(realLib,newName);
        }
    }

    //复制传入的app
    CmdUtil::execShell("cp -f "+appPathStr+ " " + outputPath,output,error);
    //用patchelf修改app
    CmdUtil::execShell("patchelf --set-rpath ./lib " + outputPath+applicationName,output,error);
    QString ldName;
    if(arch == ArchEnum::X86_64){
        ldName = "ld-linux-x86-64.so.2";
    }
    if(arch == ArchEnum::aarch64){
        ldName = "ld-linux-aarch64.so.1";
    }

    CmdUtil::execShell(QString::fromLatin1("patchelf --set-interpreter ./lib/%1 %2").arg(ldName).arg(outputPath+applicationName),output,error);
    //制作app.sh文件
    QFile depFile(":/deploy.sh");
    depFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray byteArr = depFile.readAll();
    depFile.close();
    QString all(byteArr);
    all = all.arg(applicationName);

    QFile appSh(outputPath+applicationName+".sh");
    appSh.open(QFile::OpenModeFlag::ReadWrite);
    appSh.write(all.toLatin1());
    appSh.close();

    //制作qt.conf文件
    QFile qtConfFile(":/qt.conf.template");
    qtConfFile.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray qtConfByteArr = qtConfFile.readAll();
    qtConfFile.close();
    QString qtConfText(qtConfByteArr);

    qDebug() << "qtConfText:" << qtConfText;
    QFile qtConf(outputPath+"qt.conf");
    qtConf.open(QFile::OpenModeFlag::ReadWrite);
    qtConf.write(qtConfText.toLatin1());
    qtConf.close();


    QString archPrefix;
    if(arch == ArchEnum::X86_64){
        archPrefix = "x86_64-linux-gnu";
    }
    if(arch == ArchEnum::aarch64){
        archPrefix = "aarch64-linux-gnu";
    }
    //复制libqxcb.so
    CmdUtil::execShell(QString::fromLatin1("cp -f /usr/lib/%2/qt5/plugins/platforms/libqxcb.so %1/plugins/platforms/").arg(outputPath).arg(archPrefix),output,error);
    //复制libpthread.so.0 librt.so.1
    CmdUtil::execShell(QString::fromLatin1("cp -f /usr/lib/%2/libpthread.so.0 %1").arg(outputPath + "./lib/").arg(archPrefix),output,error);
    CmdUtil::execShell(QString::fromLatin1("cp -f /usr/lib/%2/librt.so.1 %1").arg(outputPath + "./lib/").arg(archPrefix),output,error);

    CmdUtil::execShell(QString::fromLatin1("kill -9 %1").arg(pid),output,error);
    //制作app.desktop文件
    //a.exec()
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QProcess process;
    QStringList params;
    process.start("ls",QStringList() << "-l");
    process.waitForStarted();
    process.waitForFinished();
    process.waitForReadyRead();

    qDebug() << process.readAllStandardOutput();
    process.start("ls",QStringList() << "-l");
    process.waitForFinished();
    qDebug() << process.readAllStandardOutput();
    process.close();

    return a.exec();
}
