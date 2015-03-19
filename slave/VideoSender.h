#ifndef _VIDEOSENDER_H
#define _VIDEOSENDER_H

#include "Hardware.h"

#include <QObject>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

class VideoSender : public QObject
{
  Q_OBJECT;

 public:
  VideoSender(Hardware *hardware);
  ~VideoSender();
  bool enableSending(bool enable);
  void setVideoSource(int index);
  void setVideoQuality(quint16 quality);

 signals:
  void media(QByteArray *media);

 private:
  void setBitrate(int bitrate);
  void emitMedia(QByteArray *data);
  static GstFlowReturn newBufferCB(GstAppSink *sink, gpointer user_data);

  GstElement *pipeline;
  QString videoSource;

  Hardware *hardware;

  GstElement *encoder;

  int bitrate;
  quint16 quality;
};

#endif
