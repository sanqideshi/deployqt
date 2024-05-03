#ifndef COLLECTTHREAD_H
#define COLLECTTHREAD_H

#include <QObject>
#include <QThread>

class CollectThread:public QThread
{
    Q_OBJECT
public:
    CollectThread();
    bool getFlag() const;
    void setFlag(bool newFlag);
signals:
    void collect();
private:
    void run() override;
    bool flag;
};

#endif // COLLECTTHREAD_H
