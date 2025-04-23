/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "VideoSender.h"
#include "Timer.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <vector>
#include <array>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

#define ENABLE_OBJECT_DETECTION 0

// High quality: 1024kbps, low quality: 256kbps
static const int video_quality_bitrate[] = {256, 1024, 2048, 8192};

#define OB_VIDEO_A             'A'
#define OB_VIDEO_Z             'Z'
#define OB_VIDEO_PARAM_A         0
#define OB_VIDEO_PARAM_WD3       1
#define OB_VIDEO_PARAM_HD3       2
#define OB_VIDEO_PARAM_BPP       3
#define OB_VIDEO_PARAM_CONTINUE  4
#define OB_VIDEO_PARAM_Z         5

VideoSender::VideoSender(EventLoop& eventLoop, Hardware *hardware):
  eventLoop(eventLoop),
  pipeline(nullptr),
  encoder(nullptr),
  processStdout(nullptr),
  processStderr(nullptr),
  processStdin(nullptr),
  processPid(-1),
  processReady(false),
  videoSource(hardware->getCameraSrc()),
  bitrate(video_quality_bitrate[0]),
  quality(0),
  hardware(hardware)
{
  ODdata[OB_VIDEO_PARAM_A] = OB_VIDEO_A;
  ODdata[OB_VIDEO_PARAM_Z] = OB_VIDEO_Z;

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
  std::cout << "Stopping video encoding" << std::endl;
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  std::cout << "Deleting pipeline" << std::endl;
  if (pipeline) {
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = nullptr;
  }

  // Close process streams if open
  processStdin.reset();
  processStdout.reset();
  processStderr.reset();

  // Kill the process if still running
  if (processPid > 0) {
    kill(processPid, SIGTERM);
    // Wait for process to terminate
    int status;
    waitpid(processPid, &status, 0);
    processPid = -1;
  }
}

bool VideoSender::enableSending(bool enable)
{
  GstElement *sink;
#define USE_TEE 0
#if USE_TEE
  GstElement *ob;
#endif
  GError *error = nullptr;

  std::cout << "In " << __FUNCTION__ << ", Enable: " << (enable ? "true" : "false") << std::endl;

  // Disable video sending
  if (!enable) {
    std::cout << "Stopping video encoding" << std::endl;
    if (pipeline) {
      gst_element_set_state(pipeline, GST_STATE_NULL);
    }

    std::cout << "Deleting pipeline" << std::endl;
    if (pipeline) {
      gst_object_unref(GST_OBJECT(pipeline));
      pipeline = nullptr;
    }
    encoder = nullptr;

    ODdata[OB_VIDEO_PARAM_CONTINUE] = 0;
    if (processStdin && processStdin->is_open()) {
      asio::error_code ec;
      asio::write(*processStdin, asio::buffer(ODdata, sizeof(ODdata)), ec);
      if (ec) {
        std::cerr << "Failed to write to process: " << ec.message() << std::endl;
      }
    }

    return true;
  }

  if (pipeline) {
    // Do nothing as the pipeline has already been created and is
    // probably running
    std::cerr << "Pipeline exists already, doing nothing" << std::endl;
    return true;
  }

  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
    std::cerr << "Failed to init GST" << std::endl;
    return false;
  }

  if (!hardware) {
    std::cerr << "No hardware plugin" << std::endl;
    return false;
  }

  std::string pipelineString = "";
  pipelineString += videoSource + " name=source";
  pipelineString += " ! ";
  if (hardware->getHardwareName() == "tegra_nano") {
    pipelineString += "capsfilter caps=\"video/x-raw(memory:NVMM),format=(string)NV12,framerate=(fraction)60/1,";
    quality = 3;
  } else {
    pipelineString += "capsfilter caps=\"video/x-raw,format=(string)I420,framerate=(fraction)30/1,";
  }

  switch(quality) {
  default:
  case 0:
    pipelineString += "width=(int)320,height=(int)240";
    break;
  case 1:
    pipelineString += "width=(int)640,height=(int)480";
    break;
  case 2:
    pipelineString += "width=(int)800,height=(int)600";
    break;
  case 3:
    pipelineString += "width=(int)1280,height=(int)720";
    break;
  }

  pipelineString += "\"";
  if (hardware->getHardwareName() == "generic_x86") {
    // WAR because the old Playstation camera needs this
    pipelineString += " ! ";
    pipelineString += "videoconvert";
  }

#if USE_TEE
  pipelineString += " ! ";
  pipelineString += "tee name=scripttee";
  // FIXME: does this case latency?
  pipelineString += " ! ";
  pipelineString += "queue";
#endif
  pipelineString += " ! ";
  pipelineString += hardware->getEncodingPipeline();
  pipelineString += " ! ";
  pipelineString += "rtph264pay name=rtppay config-interval=-1 mtu=500";
  pipelineString += " ! ";
  pipelineString += "appsink name=sink sync=false max-buffers=1 drop=true";
#if USE_TEE
  // Tee (branch) frames for external components
  pipelineString += " scripttee. ";
  // TODO: downscale to 320x240?
  pipelineString += " ! ";
  pipelineString += "appsink name=ob sync=false max-buffers=1 drop=true";
#endif
  std::cout << "Using pipeline: " << pipelineString << std::endl;

  // Create encoding video pipeline
  pipeline = gst_parse_launch(pipelineString.c_str(), &error);
  if (!pipeline) {
    std::cerr << "Failed to parse pipeline: " << error->message << std::endl;
    g_error_free(error);
    return false;
  }

  encoder = gst_bin_get_by_name(GST_BIN(pipeline), "encoder");
  if (!encoder) {
    std::cerr << "Failed to get encoder" << std::endl;
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

  if (hardware->getHardwareName() == "tegrax2") {
    g_object_set(G_OBJECT(encoder), "preset-level", 0, NULL); // 0 == UltraFastPreset for high perf
  }

  if (hardware->getHardwareName() == "tegra_nano") {
    g_object_set(G_OBJECT(encoder), "control-rate",   2, NULL);
    g_object_set(G_OBJECT(encoder), "preset-level",   0, NULL); // 0 == UltraFastPreset for high perf
    g_object_set(G_OBJECT(encoder), "profile",        8, NULL);
    g_object_set(G_OBJECT(encoder), "iframeinterval", 120, NULL);
    g_object_set(G_OBJECT(encoder), "insert-sps-pps", 1, NULL);
  }

  setBitrate(bitrate);

  {
    GstElement *source;
    source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (!source) {
      std::cerr << "Failed to get source" << std::endl;
      return false;
    }

    g_object_set(G_OBJECT(source), "do-timestamp", true, NULL);

    if (videoSource == "videotestsrc") {
      g_object_set(G_OBJECT(source), "is-live", true, NULL);
    } else if (videoSource == hardware->getCameraSrc()) {
      //g_object_set(G_OBJECT(source), "always-copy", false, NULL);
    }

    if (hardware->getCameraSrc() == "v4l2src") {
      std::string cameraPath = "/dev/video0";
      const char* camera = cameraPath.c_str();

      char* env_camera = std::getenv("PLECO_SLAVE_CAMERA");
      if (env_camera != nullptr) {
        camera = env_camera;
      }

      g_object_set(G_OBJECT(source), "device", camera, NULL);
    } else if (hardware->getCameraSrc() == "nvarguscamerasrc") {
      g_object_set(G_OBJECT(source), "sensor-id", 0, NULL);
    }

    if (hardware->getHardwareName() == "tegrak1" ||
        hardware->getHardwareName() == "tegrax1") {
      g_object_set(G_OBJECT(source), "io-mode", 1, NULL);
    }
  }

  sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (!sink) {
    std::cerr << "Failed to get sink" << std::endl;
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
    std::cerr << "Failed to get ob appsink" << std::endl;
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

  if (ENABLE_OBJECT_DETECTION) {
    launchObjectDetection();
  }

  return true;
}

/*
 * Handle Object Detection process exit
 */
void VideoSender::handleObjectDetectionExit(int exitCode)
{
  std::cout << "In " << __FUNCTION__ << ", exitCode: " << exitCode << std::endl;

  // Close streams
  processStdin.reset();
  processStdout.reset();
  processStderr.reset();

  processPid = -1;
}

/*
 * Process Object Detection output
 */
void VideoSender::processObjectDetectionOutput()
{
  // Process only if there's a valid stdout descriptor
  if (!processStdout || !processStdout->is_open()) {
    std::cout << "In " << __FUNCTION__ << ", no processStdout" << std::endl;
    return;
  }

  // Create a persistent buffer for reading
  auto bufferPtr = std::make_shared<std::array<char, 1024>>();

  // Start asynchronous read
  processStdout->async_read_some(
    asio::buffer(*bufferPtr),
    [this, bufferPtr](const asio::error_code& error, std::size_t bytes) {
      if (!error) {
        // Process the new data
        std::string output(bufferPtr->data(), bytes);

        // Check for "OD: ready" message
        if (output.find("OD: ready") != std::string::npos) {
          processReady = true;
          std::cout << "Object detection process is ready" << std::endl;
        }

        // Log all output
        std::cout << "OD Process output: " << output << std::endl;

        // Continue reading - restart the async operation with a new buffer
        processObjectDetectionOutput();
      }
    });
}

/*
 * Launch Object Detection process
 */
void VideoSender::launchObjectDetection()
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  if (processPid > 0) {
    std::cerr << __FUNCTION__ << ": Process already exists, closing" << std::endl;
    kill(processPid, SIGTERM);
    int status;
    waitpid(processPid, &status, 0);
    processPid = -1;
  }

  ODdata[OB_VIDEO_PARAM_CONTINUE] = 1;

  // Create pipes for stdin, stdout, stderr
  int stdinPipe[2], stdoutPipe[2], stderrPipe[2];

  if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0) {
    std::cerr << "Failed to create pipes" << std::endl;
    return;
  }

  // Make the read end of stdin and the write ends of stdout/stderr non-blocking
  fcntl(stdinPipe[0], F_SETFL, O_NONBLOCK);
  fcntl(stdoutPipe[1], F_SETFL, O_NONBLOCK);
  fcntl(stderrPipe[1], F_SETFL, O_NONBLOCK);

  // Fork a child process
  processPid = fork();

  if (processPid < 0) {
    std::cerr << "Failed to fork process" << std::endl;
    close(stdinPipe[0]);
    close(stdinPipe[1]);
    close(stdoutPipe[0]);
    close(stdoutPipe[1]);
    close(stderrPipe[0]);
    close(stderrPipe[1]);
    return;
  }

  if (processPid == 0) {
    // Child process

    // Redirect stdin, stdout, stderr
    dup2(stdinPipe[0], STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);
    dup2(stderrPipe[1], STDERR_FILENO);

    // Close unused pipe ends
    close(stdinPipe[1]);
    close(stdoutPipe[0]);
    close(stderrPipe[0]);

    // Execute the script
    execl("/bin/sh", "sh", "-c", "./dlscript-dummy.py", nullptr);

    // If execl returns, there was an error
    std::cerr << "Failed to execute process" << std::endl;
    exit(1);
  }

  // Parent process

  // Close unused pipe ends
  close(stdinPipe[0]);
  close(stdoutPipe[1]);
  close(stderrPipe[1]);

  // Create ASIO stream descriptors for the pipes
  processStdin = std::make_unique<asio::posix::stream_descriptor>(eventLoop.context(), stdinPipe[1]);
  processStdout = std::make_unique<asio::posix::stream_descriptor>(eventLoop.context(), stdoutPipe[0]);
  processStderr = std::make_unique<asio::posix::stream_descriptor>(eventLoop.context(), stderrPipe[0]);

  // Start reading from stdout
  processObjectDetectionOutput();

  // Monitor for child process exit using signalfd
  auto waitForChildExit = [this]() {
    int status;
    pid_t result = waitpid(processPid, &status, WNOHANG);

    if (result > 0) {
      // Process has exited
      int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
      // Call the exit handler on the main thread
      asio::post(eventLoop.context(), [this, exitCode]() {
        handleObjectDetectionExit(exitCode);
      });
      return true;
    }

    return false;
  };

  // Start a timer to periodically check if child process has exited
  auto timer = std::make_shared<Timer>(eventLoop);
  timer->start(500, [timer, waitForChildExit]() {
    if (!waitForChildExit()) {
      // Child still running, continue monitoring
      timer->start(500, [timer, waitForChildExit]() {
        waitForChildExit();
      });
    }
  });
}

void VideoSender::emitVideo(std::vector<std::uint8_t> *data)
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  if (videoCallback) {
    videoCallback(data);
  } else {
    delete data; // Cleanup if no callback is registered
  }
}

GstFlowReturn VideoSender::newBufferCB(GstAppSink *sink, gpointer user_data)
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  VideoSender *vs = static_cast<VideoSender *>(user_data);

  // Get new video sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    std::cerr << __FUNCTION__ << ": Failed to get new sample" << std::endl;
    return GST_FLOW_OK;
  }

  // FIXME: zero copy?
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  std::vector<std::uint8_t> *data = nullptr;

  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    // Copy the data to a vector
    data = new std::vector<std::uint8_t>(map.data, map.data + map.size);
    vs->emitVideo(data);
    gst_buffer_unmap(buffer, &map);
  } else {
    std::cerr << "Error with gst_buffer_map" << std::endl;
  }

  gst_sample_unref(sample);

  return GST_FLOW_OK;
}

/*
 * Write camera frame to Object Detection process
 */
GstFlowReturn VideoSender::newBufferOBCB(GstAppSink *sink, gpointer user_data)
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  VideoSender *vs = static_cast<VideoSender *>(user_data);

  // Get new video sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    std::cerr << __FUNCTION__ << ": Failed to get new sample" << std::endl;
    return GST_FLOW_OK;
  }

  if (!vs->processReady) {
    std::cout << "ODprocess not ready yet, not sending frame" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  GstCaps *caps = gst_sample_get_caps(sample);
  if (caps == NULL) {
    std::cerr << __FUNCTION__ << ": Failed to get caps of the sample" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  gint width, height;
  GstStructure *gststruct = gst_caps_get_structure(caps, 0);
  gst_structure_get_int(gststruct, "width", &width);
  gst_structure_get_int(gststruct, "height", &height);

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;

  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    vs->ODdata[OB_VIDEO_PARAM_WD3] = width >> 3;
    vs->ODdata[OB_VIDEO_PARAM_HD3] = height >> 3;
    vs->ODdata[OB_VIDEO_PARAM_BPP] = map.size * 8 / (width * height);

    if (vs->processStdin && vs->processStdin->is_open()) {
      vs->processReady = false;

      // Write the header and frame data to the process
      asio::error_code ec;
      asio::write(*vs->processStdin, asio::buffer(vs->ODdata, sizeof(vs->ODdata)), ec);

      if (!ec) {
        asio::write(*vs->processStdin, asio::buffer(map.data, map.size), ec);
      }

      if (ec) {
        std::cerr << "Error writing to process: " << ec.message() << std::endl;
      }
    }

    gst_buffer_unmap(buffer, &map);
  } else {
    std::cerr << "Error with gst_buffer_map" << std::endl;
  }

  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

void VideoSender::setVideoSource(int index)
{
  switch (index) {
  case 0: // v4l2src
    videoSource = hardware->getCameraSrc();
    break;
  case 1: // videotestsrc
    videoSource = "videotestsrc";
    break;
  default:
    std::cerr << __FUNCTION__ << ": Unknown video source index: " << index << std::endl;
  }
}

void VideoSender::setBitrate(int bitrate)
{
  std::cout << "In " << __FUNCTION__ << ", bitrate: " << bitrate << std::endl;

  if (!encoder) {
    if (pipeline) {
      std::cerr << "Pipeline found, but no encoder?" << std::endl;
    }
    return;
  }

  int tmpbitrate = bitrate;
  if (!hardware->bitrateInKilobits()) {
    tmpbitrate *= 1024;
  }

  std::cout << "In " << __FUNCTION__ << ", setting bitrate: " << tmpbitrate << std::endl;
  if (hardware->getHardwareName() == "tegrak1" ||
      hardware->getHardwareName() == "tegrax1") {
    g_object_set(G_OBJECT(encoder), "target-bitrate", tmpbitrate, NULL);
  } else {
    g_object_set(G_OBJECT(encoder), "bitrate", tmpbitrate, NULL);
  }
}

void VideoSender::setVideoQuality(std::uint16_t q)
{
  quality = q;

  if (quality < sizeof(video_quality_bitrate) / sizeof(video_quality_bitrate[0])) {
    bitrate = video_quality_bitrate[quality];
  } else {
    std::cerr << __FUNCTION__ << ": Unknown quality: " << quality << std::endl;
    bitrate = video_quality_bitrate[0];
  }

  setBitrate(bitrate);
}

void VideoSender::setVideoCallback(VideoCallback callback)
{
  videoCallback = callback;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
