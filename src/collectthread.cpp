#include "collectthread.h"
#include <QTextStream>
#include <iostream>
CollectThread::CollectThread() : flag(true) {}

void CollectThread::run() {
  // QTextStream qout(stdout);

  while (flag) {
    emit collect();
    msleep(2000);
  }
}

bool CollectThread::getFlag() const { return flag; }

void CollectThread::setFlag(bool newFlag) { flag = newFlag; }
