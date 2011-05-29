#include "VideoReceiver.h"

#include <QWidget>
#include <QDebug>
#include <QMouseEvent>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/app/gstappsrc.h>
#include <glib.h>

#include <string.h>                          /* memcpy */

VideoReceiver::VideoReceiver(QWidget *parent):
  QWidget(parent), xid(0), pipeline(NULL), source(NULL)
{


  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
	g_thread_init(NULL);
  }

  xid = winId();
  qDebug() << __FUNCTION__ << "xid:" << xid;


  setFocusPolicy(Qt::StrongFocus);
}



VideoReceiver::~VideoReceiver(void)
{

  // Clean up
  qDebug() << "Stopping playback";
  if (pipeline) {
	gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  qDebug() << "Deleting pipeline";
  if (pipeline) {
	gst_object_unref(GST_OBJECT(pipeline));
	pipeline = NULL;
  }

}

gboolean VideoReceiver::busCall(GstBus     *,
								GstMessage *message,
								gpointer    data)
{


  VideoReceiver *vr = static_cast<VideoReceiver *>(data);
  GError *error = NULL;
  gchar *debug = NULL;

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
	gst_message_parse_error(message, &error, &debug);
	qWarning("Error: %s", error->message);
	g_error_free(error);
	g_free(debug);
    
	break;
  case GST_MESSAGE_EOS:
	// end-of-stream
	// g_main_loop_quit(loop);
	qWarning("%s: Warning: EOS", __FUNCTION__);
	break;
  case GST_MESSAGE_WARNING:
	gst_message_parse_warning(message, &error, &debug);
	if (error != NULL) {
	  qWarning("Warning: %s", error->message);
	  g_error_free(error);
	}
	if (debug != NULL) {
	  qDebug("Debug: %s", debug);
	  g_free(debug);
	}
	break;
  case GST_MESSAGE_ELEMENT:
	
	if (gst_structure_has_name(message->structure, "prepare-xwindow-id") && vr->xid != 0) {
	  qDebug("%s - prepare-xwindow-id", __FUNCTION__);
	  gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(GST_MESSAGE_SRC(message)), vr->xid);
	}
    
	break;
  default:
	// Unhandled message 
	//qWarning("Unhandled message type: %d", GST_MESSAGE_TYPE(message));
	break;
  }
  
  return true;
}



bool VideoReceiver::enableVideo(bool enable)
{
  GstElement *rtpdepay, *decoder, *sink;
  GstBus *bus;
  GstCaps *caps;

  if (!enable) {
	qCritical("disabling VideoReceiver not implemented");
	return false;
  }


  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
	qCritical("Failed to init GST");
	return false;
  }

  // Create receiving video pipeline 

  pipeline = gst_pipeline_new("videopipeline");

  source        = gst_element_factory_make("appsrc", "source");
  rtpdepay      = gst_element_factory_make("rtph263depay", "rtpdepay");
  decoder       = gst_element_factory_make("ffdec_h263", "decoder");
  sink          = gst_element_factory_make("xvimagesink", "sink");

  g_object_set(G_OBJECT(sink), "sync", false, NULL);
  g_object_set(G_OBJECT(source), "do-timestamp", true, NULL);
  // Set the stream to act "live stream"
  g_object_set(G_OBJECT(source), "is-live", true, NULL);
  // Set the stream type to "stream"
  g_object_set(G_OBJECT(source), "stream-type", 0, NULL);

  // Set the caps for appsrc
  caps = gst_caps_new_simple("application/x-rtp",
							 "media", G_TYPE_STRING, "video",
							 "clock-rate", G_TYPE_INT, 90000,
							 "encoding-name", G_TYPE_STRING, "H263",
							 "payload", G_TYPE_INT, 96,
							 "framerate", GST_TYPE_FRACTION, 10, 1,
							 NULL);
  gst_app_src_set_caps(GST_APP_SRC(source), caps);
  gst_caps_unref (caps);

  gst_bin_add_many(GST_BIN(pipeline), 
				   source, rtpdepay, decoder, sink, NULL);

  // Link 
  if (!gst_element_link_many(source, rtpdepay, decoder, sink, NULL)) {
    qCritical("Failed to link elements!");
	return false;
  }

  // Add a watch for new messages on our pipeline's message bus
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, busCall, this);
  gst_object_unref(bus);
  bus = NULL;

  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}



void VideoReceiver::consumeVideo(QByteArray *media)
{
  qDebug() << "In" << __FUNCTION__;

  GstBuffer *buffer = gst_buffer_new_and_alloc(media->length());

  // FIXME: zero copy?
  memcpy(GST_BUFFER_DATA(buffer), media->data(), media->length());

  if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK) {
	qWarning("Error with gst_app_src_push_buffer");
  }
}



void VideoReceiver::mouseMoveEvent(QMouseEvent *event)
{

  double x_percent = event->x() / (double)(this->width());

  // reverse y
  double y_percent = 1 - (event->y() / (double)(this->height()));

  //clamp to 0-1
  if (x_percent < 0) x_percent = 0;
  if (y_percent < 0) y_percent = 0;
  if (x_percent > 1) x_percent = 1;
  if (y_percent > 1) y_percent = 1;

  //qDebug() << "In" << __FUNCTION__ << ", X Y:" << x_percent << y_percent;

  emit(pos(x_percent, y_percent));
}





void VideoReceiver::keyPressEvent(QKeyEvent *event)
{
  emit(motorControlEvent(event));
}
