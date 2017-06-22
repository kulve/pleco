/*
 * Copyright 2012 Tuomas Kulve, <tuomas@kulve.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "VideoSender.h"

#include <QObject>
#include <QDebug>
#include <QProcess>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

// High quality: 1024kbps, low quality: 256kbps
static const int video_quality_bitrate[] = {256, 1024, 2048};

#define OB_VIDEO_A             'A'
#define OB_VIDEO_Z             'Z'
#define OB_VIDEO_PARAM_A         0
#define OB_VIDEO_PARAM_WD3       1
#define OB_VIDEO_PARAM_HD3       2
#define OB_VIDEO_PARAM_BPP       3
#define OB_VIDEO_PARAM_CONTINUE  4
#define OB_VIDEO_PARAM_Z         5

VideoSender::VideoSender(Hardware *hardware):
  QObject(), pipeline(NULL), videoSource("v4l2src"), hardware(hardware),
  encoder(NULL), bitrate(video_quality_bitrate[0]), quality(0),
  ODprocess(NULL), ODprocessReady(false)
{

  ODdata[OB_VIDEO_PARAM_A] =        OB_VIDEO_A;
  ODdata[OB_VIDEO_PARAM_Z] =        OB_VIDEO_Z;

#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and its threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

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
  GstElement *sink;
#define USE_TEE 0
#if USE_TEE
  GstElement *ob;
#endif
  GError *error = NULL;

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
    encoder = NULL;

    ODdata[OB_VIDEO_PARAM_CONTINUE] = 0;
    if (ODprocess) {
      ODprocess->write((const char *)ODdata, sizeof(ODdata));
    }

    return true;
  }

  if (pipeline) {
    // Do nothing as the pipeline has already been created and is
    // probably running
    qCritical("Pipeline exists already, doing nothing");
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

  QString pipelineString = "";
  pipelineString.append(videoSource + " name=source");
  pipelineString.append(" ! ");
  switch(quality) {
  default:
  case 0:
    pipelineString.append("capsfilter caps=\"video/x-raw,format=(string)I420,width=(int)320,height=(int)240,framerate=(fraction)30/1\"");
    break;
  case 1:
    pipelineString.append("capsfilter caps=\"video/x-raw,format=(string)I420,width=(int)640,height=(int)480,framerate=(fraction)30/1\"");
    break;
  case 2:
    pipelineString.append("capsfilter caps=\"video/x-raw,format=(string)I420,width=(int)800,height=(int)600,framerate=(fraction)30/1\"");
    break;
  }

#if USE_TEE
  pipelineString.append(" ! ");
  pipelineString.append("tee name=scripttee");
  // FIXME: does this case latency?
  pipelineString.append(" ! ");
  pipelineString.append("queue");
#endif
  pipelineString.append(" ! ");
  pipelineString.append(hardware->getEncodingPipeline());
  pipelineString.append(" ! ");
  pipelineString.append("rtph264pay name=rtppay config-interval=1 mtu=500");
  pipelineString.append(" ! ");
  pipelineString.append("appsink name=sink sync=false max-buffers=1 drop=true");
#if USE_TEE
  // Tee (branch) frames for external components
  pipelineString.append(" scripttee. ");
  // TODO: downscale to 320x240?
  pipelineString.append(" ! ");
  pipelineString.append("appsink name=ob sync=false max-buffers=1 drop=true");
#endif
  qDebug() << "Using pipeline:" << pipelineString;

  // Create encoding video pipeline
  pipeline = gst_parse_launch(pipelineString.toUtf8(), &error);
  if (!pipeline) {
    qCritical("Failed to parse pipeline: %s", error->message);
    g_error_free(error);
    return false;
  }

  encoder = gst_bin_get_by_name(GST_BIN(pipeline), "encoder");
  if (!encoder) {
    qCritical("Failed to get encoder");
    return false;
  }

  // Assuming here that X86 uses x264enc
  if (hardware->getHardwareName() == "generic_x86") {
    g_object_set(G_OBJECT(encoder), "speed-preset", 1, NULL); // ultrafast
    g_object_set(G_OBJECT(encoder), "tune", 0x00000004, NULL); // zerolatency
  }

  if (hardware->getHardwareName() == "tegrak1" ||
      hardware->getHardwareName() == "tegrax1") {
    //g_object_set(G_OBJECT(encoder), "input-buffers", 2, NULL); // not valid on 1.0
    //g_object_set(G_OBJECT(encoder), "output-buffers", 2, NULL); // not valid on 1.0
    //g_object_set(G_OBJECT(encoder), "quality-level", 0, NULL);
    //g_object_set(G_OBJECT(encoder), "rc-mode", 0, NULL);
  }

  setBitrate(bitrate);

  {
    GstElement *source;
    source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (!source) {
      qCritical("Failed to get source");
      return false;
    }

    g_object_set(G_OBJECT(source), "do-timestamp", true, NULL);

    if (videoSource == "videotestsrc") {
      g_object_set(G_OBJECT(source), "is-live", true, NULL);
    } else if (videoSource == "v4l2src") {
      //g_object_set(G_OBJECT(source), "always-copy", false, NULL);

      const char *camera = "/dev/video0";
      QByteArray env_camera = qgetenv("PLECO_SLAVE_CAMERA");
      if (!env_camera.isNull()) {
        camera = env_camera.data();
      }
      g_object_set(G_OBJECT(source), "device", camera, NULL);
    }

    if (hardware->getHardwareName() == "tegrak1" ||
        hardware->getHardwareName() == "tegrax1") {
      g_object_set(G_OBJECT(source), "io-mode", 1, NULL);
    }
  }


  sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (!sink) {
    qCritical("Failed to get sink");
    return false;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks appSinkCallbacks;
  appSinkCallbacks.eos             = NULL;
  appSinkCallbacks.new_preroll     = NULL;
  appSinkCallbacks.new_sample      = &newBufferCB;

  gst_app_sink_set_callbacks(GST_APP_SINK(sink), &appSinkCallbacks, this, NULL);
#if USE_TEE
  // Callbacks for the OB process appsink
  ob = gst_bin_get_by_name(GST_BIN(pipeline), "ob");
  if (!ob) {
    qCritical("Failed to get ob appsink");
    return false;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks obCallbacks;
  obCallbacks.eos             = NULL;
  obCallbacks.new_preroll     = NULL;
  obCallbacks.new_sample      = &newBufferOBCB;

  gst_app_sink_set_callbacks(GST_APP_SINK(ob), &obCallbacks, this, NULL);
#endif
  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  launchObjectDetection();

  return true;
}



/*
 * Objection Detection process exited
 */
void VideoSender::ODfinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  qDebug() << "In" << __FUNCTION__ << ", exitCode:" << exitCode << ", exitStatus:" << exitStatus;
  if (ODprocess) {
    ODprocess->close();
    delete ODprocess;
    ODprocess = NULL;
  }
}



/*
 * Objection Detection process has output
 */
void VideoSender::ODreadyRead()
{
  if (!ODprocess) {
    qDebug() << "In" << __FUNCTION__ << ", no ODprocess";
    return;
  }

  while (ODprocess->canReadLine()) {
    QByteArray msg = ODprocess->readLine();
    msg = msg.trimmed();
    qDebug() << "In" << __FUNCTION__ << "msg:" << msg;
    if (msg == "OD: ready") {
      ODprocessReady = true;
    }
  }
}



/*
 * Launch Objection Detection process
 */
void VideoSender::launchObjectDetection()
{
  qDebug() << "In" << __FUNCTION__;

  if (ODprocess) {
    qWarning("%s: ODprocess already exists, closing", __FUNCTION__);
    ODfinished(0, QProcess::NormalExit);
  }

  ODdata[OB_VIDEO_PARAM_CONTINUE] = 1;

  QString process_name = "./dlscript-dummy.py";
  //QString process_name = "./dump-wrapper.sh";

  ODprocess = new QProcess();

  QObject::connect(ODprocess, SIGNAL(readyReadStandardOutput()),
                   this, SLOT(ODreadyRead()));

  QObject::connect(ODprocess, SIGNAL(finished(int, QProcess::ExitStatus)),
                   this, SLOT(ODfinished(int, QProcess::ExitStatus)));

  ODprocess->start(process_name);
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

  // Get new video sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    qWarning("%s: Failed to get new sample", __FUNCTION__);
    return GST_FLOW_OK;
  }
  
  // FIXME: zero copy?
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  QByteArray *data = NULL;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    // Copy the data to QByteArray
    data = new QByteArray((char *)map.data, map.size);
    vs->emitMedia(data);
    gst_buffer_unmap(buffer, &map);
  } else {
    qWarning("Error with gst_buffer_map");
  }
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}



/*
 * Write camera frame to Objection Detection process
 */
GstFlowReturn VideoSender::newBufferOBCB(GstAppSink *sink, gpointer user_data)
{
  qDebug() << "In" << __FUNCTION__;

  VideoSender *vs = static_cast<VideoSender *>(user_data);

  // Get new video sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    qWarning("%s: Failed to get new sample", __FUNCTION__);
    return GST_FLOW_OK;
  }

  if (!vs->ODprocessReady) {
    qDebug() << "ODprocess not ready yet, not sending frame";
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  GstCaps *caps = gst_sample_get_caps(sample);
  if (caps == NULL) {
    qWarning("%s: Failed to get caps of the sample", __FUNCTION__);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  gint width, height;
  GstStructure *gststruct = gst_caps_get_structure(caps, 0);
  gst_structure_get_int(gststruct,"width", &width);
  gst_structure_get_int(gststruct,"height", &height);

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {

    vs->ODdata[OB_VIDEO_PARAM_WD3] = width >> 3;
    vs->ODdata[OB_VIDEO_PARAM_HD3] = height >> 3;
    vs->ODdata[OB_VIDEO_PARAM_BPP] = map.size * 8 / (width * height);

    if (vs->ODprocess) {
      vs->ODprocessReady = false;
      vs->ODprocess->write((const char *)vs->ODdata, sizeof(vs->ODdata));
      vs->ODprocess->write((const char *)map.data, map.size);
    }
    gst_buffer_unmap(buffer, &map);
  } else {
    qWarning("Error with gst_buffer_map");
  }
  gst_sample_unref(sample);
  return GST_FLOW_OK;
}


void VideoSender::setVideoSource(int index)
{
  switch (index) {
  case 0: // v4l2src
    videoSource = "v4l2src";
    break;
  case 1: // videotestsrc
    videoSource = "videotestsrc";
    break;
  default:
    qWarning("%s: Unknown video source index: %d", __FUNCTION__, index);
  }

}



void VideoSender::setBitrate(int bitrate)
{

  qDebug() << "In" << __FUNCTION__ << ", bitrate:" << bitrate;

  if (!encoder) {
    if (pipeline) {
      qWarning("Pipeline found, but no encoder?");
    }
    return;
  }

  int tmpbitrate = bitrate;
  if (!hardware->bitrateInKilobits()) {
    tmpbitrate *= 1024;
  }

  qDebug() << "In" << __FUNCTION__ << ", setting bitrate:" << tmpbitrate;
  if (hardware->getHardwareName() == "tegrak1" ||
      hardware->getHardwareName() == "tegrax1") {
    g_object_set(G_OBJECT(encoder), "target-bitrate", tmpbitrate, NULL);
  } else {
    g_object_set(G_OBJECT(encoder), "bitrate", tmpbitrate, NULL);
  }
}



void VideoSender::setVideoQuality(quint16 q)
{
  quality = q;

  if (quality < sizeof(video_quality_bitrate)) {
    bitrate = video_quality_bitrate[quality];
  } else {
    qWarning("%s: Unknown quality: %d", __FUNCTION__, quality);
    bitrate = video_quality_bitrate[0];
  }

  setBitrate(bitrate);
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
