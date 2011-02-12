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
  
 private:
  static gboolean busCall(GstBus     *bus,
						  GstMessage *msg,
						  gpointer    data);

  WId xid;
  GstElement *decoder;
  GstElement *pipeline;

};

#endif
