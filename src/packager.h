#ifndef PACKAGER_H
#define PACKAGER_H

#include <QFileInfo>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include "Arch.h"
#include "collectthread.h"
#include "controlthread.h"
class Packager : public QObject
{
    Q_OBJECT
public:
    explicit Packager(QFileInfo appPath,QObject *parent = nullptr);
    void pathPack();
    void watchPack();
    void watchPack(QString pid);
    void packWithWenengine();

    void setQmlPaths(QStringList qmlPaths);
    ControlThread controlThread;
    CollectThread collectthread;


    bool getHasExec() const;
    void setHasExec(bool newHasExec);

private:
    QStringList qmlPaths;
    QString thisAppName;
    QFileInfo appPath;
    QString appName;
    QString qtLibsDir;
    QString qtDir;
    QSharedPointer<QSet<QString>> soPaths;
    QString outputPath;
    bool hasExec;

    const QString &getAppName() const;
    void setAppName(const QString &newAppName);

    const QString &getThisAppName() const;
    void setThisAppName(const QString &newThisAppName);


    const QString &getQtLibsDir() const;
    void setQtLibsDir(const QString &newQtLibsDir);

    const QString &getQtDir() const;
    void setQtDir(const QString &newQtDir);

    const QString &getOutputPath() const;
    void setOutputPath(const QString &newOutputPath);



    ArchEnum getArch();
    void runApp(QString appPathStr);
    QString getPid();
    QString getPlddApp(QString pid);
    void collectSo(QString plddStr);
    void copySo();
    void copyExtraFiles(ArchEnum arch);
    void patchelf(ArchEnum arch);

    void makeFiles();
    void copyQml();
    void chmodX();
    void cpExec();

signals:

};

#endif // PACKAGER_H
