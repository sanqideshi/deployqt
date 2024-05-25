#ifndef CONTROLTHREAD_H
#define CONTROLTHREAD_H

#include <QObject>
#include <QThread>

class ControlThread : public QThread {
  Q_OBJECT
public:
  explicit ControlThread();
  bool getIsExit() const;
  void setIsExit(bool newIsExit);
signals:
  void mstop();
  void mexit();

private:
  bool isExit;
  void run() override;
};

#endif // CONTROLTHREAD_H
