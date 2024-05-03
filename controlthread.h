#ifndef CONTROLTHREAD_H
#define CONTROLTHREAD_H

#include <QObject>
#include <QThread>

class ControlThread:public QThread
{
    Q_OBJECT
public:
    ControlThread();
    bool getIsExit() const;
    void setIsExit(bool newIsExit);
signals:
    void mstop();
private:
    bool isExit;
    void run() override;
};

#endif // CONTROLTHREAD_H
