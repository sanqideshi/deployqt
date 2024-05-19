#ifndef WATCHOR_H
#define WATCHOR_H

#include <QObject>

class Watchor : public QObject
{
    Q_OBJECT
public:
    explicit Watchor(QObject *parent = nullptr);

signals:

};

#endif // WATCHOR_H
