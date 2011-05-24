#ifndef _PLOTTER_H
#define _PLOTTER_H

#include <QWidget>

#include <gst/gst.h>
#include <glib.h>

class Plotter : public QWidget
{
  Q_OBJECT;

 public:
  Plotter(QWidget *parent = 0);
  ~Plotter(void);

  bool push(quint value);

 protected:
  void paintEvent(QPaintEvent *event);

 private:
  QVector<int> data;
};

#endif
