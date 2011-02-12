#include "VideoReceiver.h"

#include <QWidget>
#include <QDebug>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <glib.h>

VideoReceiver::VideoReceiver(QWidget *parent):
  QWidget(parent), xid(0), pipeline(NULL)
{


  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
	g_thread_init(NULL);
  }

  xid = winId();
  qDebug() << __FUNCTION__ << "xid:" << xid;
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

gboolean VideoReceiver::busCall(GstBus     *bus,
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
  GstElement *rtph263pdepay, *queue2, *ffdec_h263, *videosink, *udpsrc;
  GstBus *bus;
  GstCaps *caps;
  gboolean link_ok;


  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
	qCritical("Failed to init GST");
	return false;
  }

  // Create receiving video pipeline 

  pipeline = gst_pipeline_new("videopipeline");

  udpsrc        = gst_element_factory_make("udpsrc", "udpsrc");
  rtph263pdepay = gst_element_factory_make("rtph263pdepay", "rtph263pdepay");
  queue2        = gst_element_factory_make("queue2", "queue2");
  ffdec_h263    = gst_element_factory_make("ffdec_h263", "ffdec_h263");
  videosink     = gst_element_factory_make("xvimagesink", "videosink");

  g_object_set(G_OBJECT(udpsrc), "port", 12345, NULL);

  gst_bin_add_many(GST_BIN(pipeline), 
				   udpsrc, rtph263pdepay, queue2, ffdec_h263,
				   videosink, NULL);

  caps = gst_caps_new_simple("application/x-rtp",
							 "media", G_TYPE_STRING, "video",
							 "clock-rate", G_TYPE_INT, 90000,
							 "encoding-name", G_TYPE_STRING, "H263-1998",
							 "payload", G_TYPE_INT, 96,
							 NULL);
  
  link_ok = gst_element_link_filtered(udpsrc, rtph263pdepay, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
    qCritical("Failed to link udpsrc and rtph263pdepay!");
	return false;
  }

  // link 
  if (!gst_element_link_many(rtph263pdepay, queue2, ffdec_h263, videosink, NULL)) {
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
