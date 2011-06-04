#include "VideoSender.h"

#include <QObject>
#include <QDebug>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

VideoSender::VideoSender(Hardware *hardware):
  QObject(), pipeline(NULL), videoSource("videotestsrc"), hardware(hardware)
{

  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
	g_thread_init(NULL);
  }

}



VideoSender::~VideoSender()
{ 

  // Clean up
  qDebug() << "Stopping video encoding";
  if (pipeline) {
	gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  qDebug() << "Deleting pipeline";
  if (pipeline) {
	gst_object_unref(GST_OBJECT(pipeline));
	pipeline = NULL;
  }

}



bool VideoSender::enableSending(bool enable)
{
  GstElement *source, *colorspace, *encoder, *rtppay, *sink;
  GstCaps *caps;
  gboolean link_ok;

  qDebug() << "In" << __FUNCTION__ << ", Enable:" << enable;

  // Disable video sending
  if (enable == false) {
	qDebug() << "Stopping video encoding";
	if (pipeline) {
	  gst_element_set_state(pipeline, GST_STATE_NULL);
	}

	qDebug() << "Deleting pipeline";
	if (pipeline) {
	  gst_object_unref(GST_OBJECT(pipeline));
	  pipeline = NULL;
	}

	return true;
  }

  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
	qCritical("Failed to init GST");
	return false;
  }

  if (!hardware) {
	qCritical("No hardware plugin");
	return false;
  }
  QByteArray encoderName = hardware->getVideoEncoderName().toAscii().data();
  qDebug() << "Using encoder:" << (gchar *)(encoderName.data());

  // Create encoding video pipeline
  pipeline = gst_pipeline_new("videopipeline");

  source        = gst_element_factory_make(videoSource.data(), "source");
  colorspace    = gst_element_factory_make("ffmpegcolorspace", "colorspace");
  encoder       = gst_element_factory_make((gchar *)(encoderName.data()), "encoder");
  rtppay        = gst_element_factory_make("rtph263pay", "rtppay");
  sink          = gst_element_factory_make("appsink", "sink");

  // FIXME: how to do these through the hw plugin?
  // Limit encoder bitrate
  //g_object_set(G_OBJECT(encoder), "bitrate", 64000, NULL); // for ffenc_h263 and dsp
  //g_object_set(G_OBJECT(encoder), "mode", 1, NULL);  // only for dsp

  g_object_set(G_OBJECT(encoder), "rtp-payload-size", 15, NULL);  // only for ffenc_h263
  
  g_object_set(G_OBJECT(sink), "sync", false, NULL);

  gst_app_sink_set_max_buffers(GST_APP_SINK(sink), 8);// 8 buffers is hopefully enough
  gst_app_sink_set_drop(GST_APP_SINK(sink), true); // drop old data, if needed

  // Set appsink callbacks
  GstAppSinkCallbacks appSinkCallbacks;
  appSinkCallbacks.eos             = NULL;
  appSinkCallbacks.new_preroll     = NULL;
  appSinkCallbacks.new_buffer      = &newBufferCB;
  appSinkCallbacks.new_buffer_list = NULL;

  gst_app_sink_set_callbacks(GST_APP_SINK(sink), &appSinkCallbacks, this, NULL);


  gst_bin_add_many(GST_BIN(pipeline), 
				   source, colorspace, encoder, rtppay, sink,
				   NULL);

  caps = gst_caps_new_simple("video/x-raw-yuv",
							 "width", G_TYPE_INT, 352,
							 "height", G_TYPE_INT, 288,
							 "framerate", GST_TYPE_FRACTION, 10, 1,
							 NULL);
  
  link_ok = gst_element_link_filtered(source, colorspace, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
    qCritical("Failed to link source and colorspace!");
	return false;
  }

  // link 
  if (!gst_element_link_many(colorspace, encoder, rtppay, sink, NULL)) {
    qCritical("Failed to link elements!");
	return false;
  }

  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}



void VideoSender::emitMedia(QByteArray *data)
{
  qDebug() << "In" << __FUNCTION__;

  emit(media(data));

}



GstFlowReturn VideoSender::newBufferCB(GstAppSink *sink, gpointer user_data)
{
  qDebug() << "In" << __FUNCTION__;

  VideoSender *vs = static_cast<VideoSender *>(user_data);

  // Get new video buffer
  GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(sink));

  if (buffer == NULL) {
	qWarning("%s: Failed to get new buffer", __FUNCTION__);
	return GST_FLOW_OK;
  }
  
  // Copy the data to QBytArray
  // FIXME: zero copy?
  QByteArray *data = new QByteArray((char *)(buffer->data), (int)(buffer->size));
  gst_buffer_unref(buffer);

  vs->emitMedia(data);

  return GST_FLOW_OK;
}



void VideoSender::setVideoSource(int index)
{
  switch (index) {
  case 0: // videotestsrc
	videoSource = "videotestsrc";
	break;
  case 1: // v4l2src
	videoSource = "v4l2src";
	break;
  default:
	qWarning("%s: Unknown video source index: %d", __FUNCTION__, index);

  }

}
