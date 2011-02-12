#ifndef _VIDEOSENDER_H
#define _VIDEOSENDER_H

#include <QObject>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

class VideoSender : public QObject
{
  Q_OBJECT;

 public:
  VideoSender(void);
  ~VideoSender();
  bool enableSending(bool enable);

 signals:
  void media(QByteArray *media);

 private:
  void emitMedia(QByteArray *data);
  static GstFlowReturn newBufferCB(GstAppSink *sink, gpointer user_data);

  GstElement *pipeline;


};

#endif
