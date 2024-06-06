#include "controlthread.h"
#include <QTextStream>
ControlThread::ControlThread() : isExit(false) {}

bool ControlThread::getIsExit() const { return isExit; }

void ControlThread::setIsExit(bool newIsExit) { isExit = newIsExit; }

void ControlThread::run()
{
  QTextStream qin(stdin);
  QString qstr;
  while (1)
  {
    qstr = qin.readLine();
    if (qstr == "stop")
    {
      // setIsExit(true);
      emit mstop();
    }
    if (qstr == "exit")
    {
      // setIsExit(true);
      emit mexit();
      break;
    }
  }
}
