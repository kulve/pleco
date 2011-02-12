#ifndef _VIDEOSENDER_H
#define _VIDEOSENDER_H

#include <QObject>

#include <gst/gst.h>
#include <glib.h>

class VideoSender : public QObject
{
  Q_OBJECT;

 public:
  VideoSender(void);
  ~VideoSender();
  bool enableSending(bool enable);

 private:
  GstElement *pipeline;
};

#endif
