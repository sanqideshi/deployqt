#ifndef CONVERT_H
#define CONVERT_H
#include <QObject>
class Convert : public QObject {
  Q_OBJECT
public:
  explicit Convert(QObject *parent = nullptr);
  virtual ~Convert();
  explicit Convert(Convert &other) = delete;
  Convert &operator=(const Convert &other) = delete;
  QString gbkToUtf8(QString &src);
  QString utf8ToGbk(QString &src);

private:
};

#endif