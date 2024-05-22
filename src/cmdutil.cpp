#include "cmdutil.h"
#include <QProcess>
CmdUtil::CmdUtil(QObject *parent)
    : QObject{parent}
{

}

bool CmdUtil::exec(QString cmd,QString &standard,QString &error)
{
    QProcess process;
    QStringList params;
    params << "/c" <<cmd;
    process.start("cmd",params);

    process.waitForStarted();
    process.waitForFinished();
    process.waitForReadyRead();
    //QString errorStr = process.errorString();

    error = process.readAllStandardError();
    if(error== ""){
        standard = process.readAll();
        process.close();
        return true;
    }else{
        process.close();
        return false;
    }


}

bool CmdUtil::exec(QString workDirectorty, QString cmd, QString &standard, QString &error)
{
    QProcess process;
    QStringList params;
    params << "/c" <<cmd;
    process.setWorkingDirectory(workDirectorty);
    process.start("cmd",params);

    process.waitForStarted();
    process.waitForFinished();
    process.waitForReadyRead();
    //QString errorStr = process.errorString();

    error = process.readAllStandardError();
    if(error== ""){
        standard = process.readAll();
        process.close();
        return true;
    }else{
        process.close();
        return false;
    }
}

bool CmdUtil::execShell(QString shell, QString &standard, QString &error)
{

    QProcess process;
    QStringList params;
    params << "-c" <<shell;
    process.start("bash",params);

    process.waitForStarted();
    process.waitForFinished();
    process.waitForReadyRead();

    error = process.readAllStandardError();
    if(error== ""){
        standard = process.readAll();
        process.close();
        return true;
    }else{
        process.close();
        return false;
    }

}

bool CmdUtil::execShell(QString &workDirectorty, QString shell, QString &standard, QString &error)
{

    QProcess process;
    QStringList params;
    params << "-c" <<shell;
    process.setWorkingDirectory(workDirectorty);
    process.start("bash",params);

    process.waitForStarted();
    process.waitForFinished();
    process.waitForReadyRead();
    error = process.readAllStandardError();
    if(error== ""){
        standard = process.readAll();
        process.close();
        return true;
    }else{
        process.close();
        return false;
    }

}
