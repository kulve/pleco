#include "VideoSender.h"

#include <QObject>
#include <QDebug>

#include <gst/gst.h>
#include <glib.h>

VideoSender::VideoSender(void):
  QObject(), pipeline(NULL)
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
  GstElement *source, *colorspace, *encoder, *queue, *rtppay, *sink;
  GstCaps *caps;
  gboolean link_ok;


  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
	qCritical("Failed to init GST");
	return false;
  }

  // Create encoding video pipeline 

  pipeline = gst_pipeline_new("videopipeline");

  source        = gst_element_factory_make("videotestsrc", "source");
  colorspace    = gst_element_factory_make("ffmpegcolorspace", "colorspace");
  encoder       = gst_element_factory_make("ffenc_h263p", "encoder");
  queue         = gst_element_factory_make("queue2", "queue");
  rtppay        = gst_element_factory_make("rtph263ppay", "rtppay");
  sink          = gst_element_factory_make("udpsink", "sink");

  // limit encoder bitrate
  g_object_set(G_OBJECT(encoder), "bitrate", 64000, NULL);

  g_object_set(G_OBJECT(sink), "host", "localhost", NULL);
  g_object_set(G_OBJECT(sink), "port", 12345, NULL);

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
