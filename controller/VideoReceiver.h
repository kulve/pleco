#ifndef _VIDEORECEIVER_H
#define _VIDEORECEIVER_H

#include <QWidget>

#include <gst/gst.h>
#include <glib.h>

class VideoReceiver : public QWidget
{
  Q_OBJECT;

 public:
  VideoReceiver(QWidget *parent = 0);
  ~VideoReceiver(void);
  bool enableVideo(bool enable);

 public slots:
  void consumeVideo(QByteArray *media);

 signals:
  void pos(double x_percent, double y_percent);
  void motorControlEvent(QKeyEvent *event);

 private:
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  static gboolean busCall(GstBus     *bus,
						  GstMessage *msg,
						  gpointer    data);

  WId xid;
  GstElement *pipeline;
  GstElement *source;
};

#endif
