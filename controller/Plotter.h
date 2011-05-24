#ifndef _PLOTTER_H
#define _PLOTTER_H

#include <QWidget>

class Plotter : public QWidget
{
  Q_OBJECT;

 public:
  Plotter(QWidget *parent = 0);
  ~Plotter(void);

  void push(int value);
  virtual QSize sizeHint(void) const;

 protected:
  void paintEvent(QPaintEvent *event);

 private:
  QVector<int> data;
};

#endif
