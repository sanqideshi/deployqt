#ifndef CMDUTIL_H
#define CMDUTIL_H

#include <QObject>

class CmdUtil : public QObject {
  Q_OBJECT
public:
  explicit CmdUtil(QObject *parent = nullptr);
  // static QByteArray exec(QString cmd);
  static bool exec(QString cmd, QString &standard, QString &error);
  static bool exec(QString workDirectorty, QString cmd, QString &standard,
                   QString &error);
  static bool execShell(QString shell, QString &standard, QString &error);
  static bool execShell(QString &workDirectorty, QString shell,
                        QString &standard, QString &error);
signals:
};

#endif // CMDUTIL_H
