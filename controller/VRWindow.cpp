/*
 * Copyright 2020 Tuomas Kulve, <tuomas@kulve.fi>
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

/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * BSD License Usage
 *
 * Partly copied from:
 * https://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl
 */

// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
//
// Partly copied from:
// https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/

#include <QOpenGLWindow>
#include <QScreen>
#include <QPainter>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QStaticText>
#include <QKeyEvent>

#include "VRWindow.h"
#include "VRRenderer.h"

static bool xr_result(XrInstance instance, XrResult result, const char* format, ...)
{
	if (XR_SUCCEEDED(result))
		return true;

	char resultString[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(instance, result, resultString);

	size_t len1 = strlen(format);
	size_t len2 = strlen(resultString) + 1;
	char formatRes[len1 + len2 + 4]; // + " []\n"
	sprintf(formatRes, "%s [%s]\n", format, resultString);

	va_list args;
	va_start(args, format);
	vprintf(formatRes, args);
	va_end(args);
	return false;
}

static bool
isExtensionSupported(char* extensionName,
                     XrExtensionProperties* instanceExtensionProperties,
                     uint32_t instanceExtensionCount)
{
	for (uint32_t supportedIndex = 0; supportedIndex < instanceExtensionCount;
	     supportedIndex++) {
		if (!strcmp(extensionName,
		            instanceExtensionProperties[supportedIndex].extensionName)) {
			return true;
		}
	}
	return false;
}


static QPainterPath painterPathForTriangle()
{
  static const QPointF bottomLeft(-1.0, -1.0);
  static const QPointF top(0.0, 1.0);
  static const QPointF bottomRight(1.0, -1.0);

  QPainterPath path(bottomLeft);
  path.lineTo(top);
  path.lineTo(bottomRight);
  path.closeSubpath();
  return path;
}

// Use NoPartialUpdate. This means that all the rendering goes directly to
// the window surface, no additional framebuffer object stands in the
// middle. This is fine since we will clear the entire framebuffer on each
// paint. Under the hood this means that the behavior is equivalent to the
// manual makeCurrent - perform OpenGL calls - swapBuffers loop that is
// typical in pure QWindow-based applications.
VRWindow::VRWindow()
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
    , m_renderer("./background.frag")
    , m_text_layout("The triangle and this text is rendered with QPainter")
    , m_animate(true)
{
    m_view.lookAt(QVector3D(3,1,1),
                  QVector3D(0,0,0),
                  QVector3D(0,1,0));

    QLinearGradient gradient(QPointF(-1,-1), QPointF(1,1));
    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(1, Qt::green);

    m_brush = QBrush(gradient);

    connect(this, &QVRWindow::frameSwapped,
            this, QOverload<>::of(&QPaintDeviceWindow::update));
}

bool VRWindow::init()
{
	XrResult result;

	// --- Make sure runtime supports the OpenGL extension

	// xrEnumerate*() functions are usually called once with CapacityInput = 0.
	// The function will write the required amount into CountOutput. We then have
	// to allocate an array to hold CountOutput elements and call the function
	// with CountOutput as CapacityInput.
	uint32_t extensionCount = 0;
	result =
	    xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);

	/* TODO: instance null will not be able to convert XrResult to string */
	if (!xr_result(NULL, result,
	               "Failed to enumerate number of extension properties"))
		return false;

	printf("Runtime supports %d extensions\n", extensionCount);

	XrExtensionProperties extensionProperties[extensionCount];
	for (uint16_t i = 0; i < extensionCount; i++) {
		// we usually have to fill in the type (for validation) and set
		// next to NULL (or a pointer to an extension specific struct)
		extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		extensionProperties[i].next = NULL;
	}

	result = xrEnumerateInstanceExtensionProperties(
	    NULL, extensionCount, &extensionCount, extensionProperties);
	if (!xr_result(NULL, result, "Failed to enumerate extension properties"))
		return false;

	if (!isExtensionSupported(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
	                          extensionProperties, extensionCount)) {
		printf("Runtime does not support OpenGL extension!\n");
		return false;
	}

	printf("Runtime supports required extension %s\n",
	       XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

	// --- Enumerate API layers
	bool lunargCoreValidationSupported = false;
	uint32_t apiLayerCount;
	xrEnumerateApiLayerProperties(0, &apiLayerCount, NULL);
	printf("Loader found %d api layers%s", apiLayerCount,
	       apiLayerCount == 0 ? "\n" : ": ");
	XrApiLayerProperties apiLayerProperties[apiLayerCount];
	memset(apiLayerProperties, 0, apiLayerCount * sizeof(XrApiLayerProperties));

	for (uint32_t i = 0; i < apiLayerCount; i++) {
		apiLayerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
		apiLayerProperties[i].next = NULL;
	}
	xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount,
	                              apiLayerProperties);
	for (uint32_t i = 0; i < apiLayerCount; i++) {
		if (strcmp(apiLayerProperties[i].layerName,
		                  "XR_APILAYER_LUNARG_core_validation") == 0) {
			lunargCoreValidationSupported = true;
		}
		printf("%s%s", apiLayerProperties[i].layerName,
		       i < apiLayerCount - 1 ? ", " : "\n");
	}

	// --- Create XrInstance
	const char* const enabledExtensions[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};

	XrInstanceCreateInfo instanceCreateInfo = {
	    .type = XR_TYPE_INSTANCE_CREATE_INFO,
	    .next = NULL,
	    .createFlags = 0,
	    .enabledExtensionCount = 1,
	    .enabledExtensionNames = enabledExtensions,
	    .enabledApiLayerCount = 0,
	    .enabledApiLayerNames = NULL,
	    .applicationInfo =
	        {
	            .applicationName = "Pleco OpenXR Controller",
	            .engineName = "",
	            .applicationVersion = 1,
	            .engineVersion = 0,
	            .apiVersion = XR_CURRENT_API_VERSION,
	        },
	};

	if (lunargCoreValidationSupported && useCoreValidationLayer) {
		instanceCreateInfo.enabledApiLayerCount = 1;
		const char* const enabledApiLayers[] = {
		    "XR_APILAYER_LUNARG_core_validation"};
		instanceCreateInfo.enabledApiLayerNames = enabledApiLayers;
	}

	result = xrCreateInstance(&instanceCreateInfo, &instance);
	if (!xr_result(NULL, result, "Failed to create XR instance."))
		return false;

	// Checking instance properties is optional!
	{
		XrInstanceProperties instanceProperties = {
		    .type = XR_TYPE_INSTANCE_PROPERTIES,
		    .next = NULL,
		};

		result = xrGetInstanceProperties(instance, &instanceProperties);
		if (!xr_result(NULL, result, "Failed to get instance info"))
			return false;

		printf("Runtime Name: %s\n", instanceProperties.runtimeName);
		printf("Runtime Version: %d.%d.%d\n",
		       XR_VERSION_MAJOR(instanceProperties.runtimeVersion),
		       XR_VERSION_MINOR(instanceProperties.runtimeVersion),
		       XR_VERSION_PATCH(instanceProperties.runtimeVersion));
	}

	// --- Create XrSystem
	XrSystemGetInfo systemGetInfo = {.type = XR_TYPE_SYSTEM_GET_INFO,
	                                 .formFactor =
	                                     XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
	                                 .next = NULL};

	XrSystemId systemId;
	result = xrGetSystem(instance, &systemGetInfo, &systemId);
	if (!xr_result(instance, result,
	               "Failed to get system for HMD form factor."))
		return false;

	printf("Successfully got XrSystem %lu for HMD form factor\n", systemId);

	// checking system properties is optional!
	{
		XrSystemProperties systemProperties = {
		    .type = XR_TYPE_SYSTEM_PROPERTIES,
		    .next = NULL,
		    .graphicsProperties = {0},
		    .trackingProperties = {0},
		};

		result = xrGetSystemProperties(instance, systemId, &systemProperties);
		if (!xr_result(instance, result, "Failed to get System properties"))
			return false;

		printf("System properties for system %lu: \"%s\", vendor ID %d\n",
		       systemProperties.systemId, systemProperties.systemName,
		       systemProperties.vendorId);
		printf("\tMax layers          : %d\n",
		       systemProperties.graphicsProperties.maxLayerCount);
		printf("\tMax swapchain height: %d\n",
		       systemProperties.graphicsProperties.maxSwapchainImageHeight);
		printf("\tMax swapchain width : %d\n",
		       systemProperties.graphicsProperties.maxSwapchainImageWidth);
		printf("\tOrientation Tracking: %d\n",
		       systemProperties.trackingProperties.orientationTracking);
		printf("\tPosition Tracking   : %d\n",
		       systemProperties.trackingProperties.positionTracking);
	}

	// --- Enumerate and set up Views
	uint32_t viewConfigurationCount;
	result = xrEnumerateViewConfigurations(instance, systemId, 0,
	                                       &viewConfigurationCount, NULL);
	if (!xr_result(instance, result,
	               "Failed to get view configuration count"))
		return false;

	printf("Runtime supports %d view configurations\n", viewConfigurationCount);

	XrViewConfigurationType viewConfigurations[viewConfigurationCount];
	result = xrEnumerateViewConfigurations(
	    instance, systemId, viewConfigurationCount, &viewConfigurationCount,
	    viewConfigurations);
	if (!xr_result(instance, result,
	               "Failed to enumerate view configurations!"))
		return false;

	XrViewConfigurationType stereoViewConfigType =
	    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	/* Checking if the runtime supports the view configuration we want to use is
	 * optional! If stereoViewConfigType.type is unset after the loop, the runtime
	 * does not support Stereo VR. */
	{
		XrViewConfigurationProperties stereoViewConfigProperties = {0};
		for (uint32_t i = 0; i < viewConfigurationCount; ++i) {
			XrViewConfigurationProperties properties = {
			    .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES, .next = NULL};

			result = xrGetViewConfigurationProperties(
			    instance, systemId, viewConfigurations[i], &properties);
			if (!xr_result(instance, result,
			               "Failed to get view configuration info %d!", i))
				return false;

			if (viewConfigurations[i] == stereoViewConfigType &&
			    /* just to verify */ properties.viewConfigurationType ==
			        stereoViewConfigType) {
				printf("Runtime supports our VR view configuration, yay!\n");
				stereoViewConfigProperties = properties;
			} else {
				printf(
				    "Runtime supports a view configuration we are not interested in: "
				    "%d\n",
				    properties.viewConfigurationType);
			}
		}
		if (stereoViewConfigProperties.type !=
		    XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
			printf("Couldn't get VR View Configuration from Runtime!\n");
			return false;
		}

		printf("VR View Configuration:\n");
		printf("\tview configuratio type: %d\n",
		       stereoViewConfigProperties.viewConfigurationType);
		printf("\tFOV mutable           : %s\n",
		       stereoViewConfigProperties.fovMutable ? "yes" : "no");
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId,
	                                           stereoViewConfigType, 0,
	                                           &view_count, NULL);
	if (!xr_result(instance, result,
	               "Failed to get view configuration view count!"))
		return false;

	configuration_views =
	    malloc(sizeof(XrViewConfigurationView) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		configuration_views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
		configuration_views[i].next = NULL;
	}

	result = xrEnumerateViewConfigurationViews(
	    instance, systemId, stereoViewConfigType, view_count,
	    &view_count, configuration_views);
	if (!xr_result(instance, result,
	               "Failed to enumerate view configuration views!"))
		return false;

	printf("View count: %d\n", view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		printf("View %d:\n", i);
		printf("\tResolution       : Recommended %dx%d, Max: %dx%d\n",
		       configuration_views[0].recommendedImageRectWidth,
		       configuration_views[0].recommendedImageRectHeight,
		       configuration_views[0].maxImageRectWidth,
		       configuration_views[0].maxImageRectHeight);
		printf("\tSwapchain Samples: Recommended: %d, Max: %d)\n",
		       configuration_views[0].recommendedSwapchainSampleCount,
		       configuration_views[0].maxSwapchainSampleCount);
	}

	// For all graphics APIs, it's required to make the
	// "xrGet...GraphicsRequirements" call before creating a session. The
	// information retrieved by the OpenGL version of this call isn't very useful.
	// Other APIs have more useful requirements.
	{
		XrGraphicsRequirementsOpenGLKHR opengl_reqs = {
		    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, .next = NULL};

		PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
		result = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
		                               (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);
		result = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &opengl_reqs);
		if (!xr_result(instance, result,
		               "Failed to get OpenGL graphics requirements!"))
			return false;

		XrVersion desired_opengl_version = XR_MAKE_VERSION(4, 5, 0);
		if (desired_opengl_version > opengl_reqs.maxApiVersionSupported ||
		    desired_opengl_version < opengl_reqs.minApiVersionSupported) {
			printf(
			    "We want OpenGL %d.%d.%d, but runtime only supports OpenGL "
			    "%d.%d.%d - %d.%d.%d!\n",
			    XR_VERSION_MAJOR(desired_opengl_version),
			    XR_VERSION_MINOR(desired_opengl_version),
			    XR_VERSION_PATCH(desired_opengl_version),
			    XR_VERSION_MAJOR(opengl_reqs.minApiVersionSupported),
			    XR_VERSION_MINOR(opengl_reqs.minApiVersionSupported),
			    XR_VERSION_PATCH(opengl_reqs.minApiVersionSupported),
			    XR_VERSION_MAJOR(opengl_reqs.maxApiVersionSupported),
			    XR_VERSION_MINOR(opengl_reqs.maxApiVersionSupported),
			    XR_VERSION_PATCH(opengl_reqs.maxApiVersionSupported));
			return false;
		}
	}

	// --- Create session

	graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR){
	    .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
	};

 	// HACK? OpenXR wants us to report these values, so "work around" SDL a
	// bit and get the underlying glx stuff. Does this still work when e.g.
	// SDL switches to xcb?
  graphics_binding_gl.xDisplay = XOpenDisplay(NULL);
  graphics_binding_gl.glxDrawable = glXGetCurrentDrawable();
  graphics_binding_gl.glxContext = glXGetCurrentContext();

	// Set up rendering (compile shaders, ...)
  /* TUOMAS:done lazily in draw
	if (initGlImpl() != 0) {
		printf("OpenGl setup failed!\n");
		return false;
	}
  */

	XrSessionCreateInfo session_create_info = {.type =
	                                               XR_TYPE_SESSION_CREATE_INFO,
	                                           .next = &graphics_binding_gl,
	                                           .systemId = systemId};


	result =
	    xrCreateSession(instance, &session_create_info, &session);
	if (!xr_result(instance, result, "Failed to create session"))
		return false;

	printf("Successfully created a session with OpenGL!\n");

	const GLubyte* renderer_string = glGetString(GL_RENDERER);
	const GLubyte* version_string = glGetString(GL_VERSION);
	printf("Using OpenGL version: %s\n", version_string);
	printf("Using OpenGL Renderer: %s\n", renderer_string);

	// --- Check supported reference spaces
	// we don't *need* to check the supported reference spaces if we're confident
	// the runtime will support whatever we use
	{
		uint32_t referenceSpacesCount;
		result = xrEnumerateReferenceSpaces(session, 0, &referenceSpacesCount,
		                                    NULL);
		if (!xr_result(instance, result,
		               "Getting number of reference spaces failed!"))
			return false;

		XrReferenceSpaceType referenceSpaces[referenceSpacesCount];
		for (uint32_t i = 0; i < referenceSpacesCount; i++)
			referenceSpaces[i] = XR_REFERENCE_SPACE_TYPE_VIEW;
		result = xrEnumerateReferenceSpaces(session, referenceSpacesCount,
		                                    &referenceSpacesCount, referenceSpaces);
		if (!xr_result(instance, result,
		               "Enumerating reference spaces failed!"))
			return false;

		bool stageSpaceSupported = false;
		bool localSpaceSupported = false;
		printf("Runtime supports %d reference spaces: ", referenceSpacesCount);
		for (uint32_t i = 0; i < referenceSpacesCount; i++) {
			if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL) {
				printf("XR_REFERENCE_SPACE_TYPE_LOCAL%s",
				       i == referenceSpacesCount - 1 ? "\n" : ", ");
				localSpaceSupported = true;
			} else if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
				printf("XR_REFERENCE_SPACE_TYPE_STAGE%s",
				       i == referenceSpacesCount - 1 ? "\n" : ", ");
				stageSpaceSupported = true;
			} else if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_VIEW) {
				printf("XR_REFERENCE_SPACE_TYPE_VIEW%s",
				       i == referenceSpacesCount - 1 ? "\n" : ", ");
			} else {
				printf("some other space with type %u%s", referenceSpaces[i],
				       i == referenceSpacesCount - 1 ? "\n" : ", ");
			}
		}

		if (/* !stageSpaceSupported || */ !localSpaceSupported) {
			printf(
			    "runtime does not support required spaces! stage: %s, "
			    "local: %s\n",
			    stageSpaceSupported ? "supported" : "NOT SUPPORTED",
			    localSpaceSupported ? "supported" : "NOT SUPPORTED");
			return false;
		}
	}

	XrPosef identityPose = {.orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
	                        .position = {.x = 0, .y = 0, .z = 0}};

	XrReferenceSpaceCreateInfo localSpaceCreateInfo = {
	    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
	    .next = NULL,
	    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
	    .poseInReferenceSpace = identityPose};

	result = xrCreateReferenceSpace(session, &localSpaceCreateInfo,
	                                &local_space);
	if (!xr_result(instance, result, "Failed to create local space!"))
		return false;

	// --- Begin session
	XrSessionBeginInfo sessionBeginInfo = {.type = XR_TYPE_SESSION_BEGIN_INFO,
	                                       .next = NULL,
	                                       .primaryViewConfigurationType =
	                                           stereoViewConfigType};
	result = xrBeginSession(session, &sessionBeginInfo);
	if (!xr_result(instance, result, "Failed to begin session!"))
		return false;
	printf("Session started!\n");



	// --- Create Swapchains
	uint32_t swapchainFormatCount;
	result = xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount,
	                                     NULL);
	if (!xr_result(instance, result,
	               "Failed to get number of supported swapchain formats"))
		return false;

	printf("Runtime supports %d swapchain formats\n", swapchainFormatCount);
	int64_t swapchainFormats[swapchainFormatCount];
	result = xrEnumerateSwapchainFormats(session, swapchainFormatCount,
	                                     &swapchainFormatCount, swapchainFormats);
	if (!xr_result(instance, result,
	               "Failed to enumerate swapchain formats"))
		return false;

	// TODO: Determine which format we want to use instead of using the first one
	int64_t swapchainFormatToUse = swapchainFormats[0];

	/* First create swapchains and query the length for each swapchain. */
	swapchains = malloc(sizeof(XrSwapchain) * view_count);

	uint32_t swapchainLength[view_count];

	for (uint32_t i = 0; i < view_count; i++) {
		XrSwapchainCreateInfo swapchainCreateInfo = {
		    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
		    .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
		                  XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
		    .createFlags = 0,
		    .format = swapchainFormatToUse,
		    .sampleCount = 1,
		    .width = configuration_views[i].recommendedImageRectWidth,
		    .height = configuration_views[i].recommendedImageRectHeight,
		    .faceCount = 1,
		    .arraySize = 1,
		    .mipCount = 1,
		    .next = NULL,
		};

		result = xrCreateSwapchain(session, &swapchainCreateInfo,
		                           &swapchains[i]);
		if (!xr_result(instance, result, "Failed to create swapchain %d!", i))
			return false;

		result = xrEnumerateSwapchainImages(swapchains[i], 0,
		                                    &swapchainLength[i], NULL);
		if (!xr_result(instance, result, "Failed to enumerate swapchains"))
			return false;
	}

	// allocate one array of images and framebuffers per view
	images = malloc(sizeof(XrSwapchainImageOpenGLKHR*) * view_count);
	framebuffers = malloc(sizeof(GLuint*) * view_count);

	for (uint32_t i = 0; i < view_count; i++) {
		// allocate array of images and framebuffers for this view
		images[i] =
		    malloc(sizeof(XrSwapchainImageOpenGLKHR) * swapchainLength[i]);
		framebuffers[i] = malloc(sizeof(GLuint) * swapchainLength[i]);

		// get OpenGL image ids from runtime
		for (uint32_t j = 0; j < swapchainLength[i]; j++) {
			images[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
			images[i][j].next = NULL;
		}
		result = xrEnumerateSwapchainImages(
		    swapchains[i], swapchainLength[i], &swapchainLength[i],
		    (XrSwapchainImageBaseHeader*)images[i]);
		if (!xr_result(instance, result,
		               "Failed to enumerate swapchain images"))
			return false;

		// framebuffers are not managed or mandated by OpenXR, it's just how we
		// happen to render into textures in this example
		genFramebuffers(swapchainLength[i], framebuffers[i]);
	}

	// TODO: use swapchain and depth extension to submit depth textures too.
	glGenTextures(1, &depthbuffer);
	glBindTexture(GL_TEXTURE_2D, depthbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
	             configuration_views[0].recommendedImageRectWidth,
	             configuration_views[0].recommendedImageRectHeight, 0,
	             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);

	// projectionLayers struct reused for every frame
	projectionLayer = {
	    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
	    .next = NULL,
	    .layerFlags = 0,
	    .space = local_space,
	    .viewCount = view_count,
	    // views is const and can't be changed, has to be created new every time
	    .views = NULL,
	};

	quadLayer = {
	    .type = XR_TYPE_COMPOSITION_LAYER_QUAD,
	    .next = NULL,
	    .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
	    .space = local_space,
	    .eyeVisibility = XR_EYE_VISIBILITY_BOTH,
	    .pose = {.orientation = {.x = 0.f, .y = 0.f, .z = 0.f, .w = 1.f},
	             .position = {.x = 1.f, .y = 1.f, .z = -3.f}},
	    .size = {.width = 1.f, .height = .5f}};

  update();
  
	return true;
}

void VRWindow::handle_xr_events()
{

	XrResult result;
  bool isStopping = false;
  bool poll_again = true;

	while (poll_again) {
    poll_again = true;
                      
		XrEventDataBuffer runtimeEvent = {.type = XR_TYPE_EVENT_DATA_BUFFER,
		                                  .next = NULL};
		XrResult pollResult = xrPollEvent(instance, &runtimeEvent);
		if (pollResult == XR_SUCCESS) {
			switch (runtimeEvent.type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost* event = (XrEventDataEventsLost*)&runtimeEvent;
				printf("EVENT: %d events data lost!\n", event->lostEventCount);
				// do we care if the runtime loses events?
				break;
			}
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending* event =
				    (XrEventDataInstanceLossPending*)&runtimeEvent;
				printf("EVENT: instance loss pending at %lu! Destroying instance.\n",
				       event->lossTime);
				// Handling this: spec says destroy instance
				// (can optionally recreate it)
				// TODO: must exit, running = false;
        isStopping = true;
				break;
			}
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				printf("EVENT: session state changed ");
				XrEventDataSessionStateChanged* event =
				    (XrEventDataSessionStateChanged*)&runtimeEvent;
				XrSessionState state = event->state;

				// it would be better to handle each state change
				isVisible = event->state <= XR_SESSION_STATE_FOCUSED;
				printf("to %d. Visible: %d", state, isVisible);
				if (event->state >= XR_SESSION_STATE_STOPPING) {
					printf("\nSession is in state stopping...");
					isStopping = true;
				}
				printf("\n");
				break;
			}
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				printf("EVENT: reference space change pending!\n");
				XrEventDataReferenceSpaceChangePending* event =
				    (XrEventDataReferenceSpaceChangePending*)&runtimeEvent;
				(void)event;
				// TODO: do something
				break;
			}
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				printf("EVENT: interaction profile changed!\n");
				XrEventDataInteractionProfileChanged* event =
				    (XrEventDataInteractionProfileChanged*)&runtimeEvent;
				(void)event;
				// TODO: do something
				break;
			}

			case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
				printf("EVENT: visibility mask changed!!\n");
				XrEventDataVisibilityMaskChangedKHR* event =
				    (XrEventDataVisibilityMaskChangedKHR*)&runtimeEvent;
				(void)event;
				// this event is from an extension
				break;
			}
			case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
				printf("EVENT: perf settings!\n");
				XrEventDataPerfSettingsEXT* event =
				    (XrEventDataPerfSettingsEXT*)&runtimeEvent;
				(void)event;
				// this event is from an extension
				break;
			}
			default: printf("Unhandled event type %d\n", runtimeEvent.type);
			}
		} else if (pollResult == XR_EVENT_UNAVAILABLE) {
      poll_again = false;
		} else {
			printf("Failed to poll events!\n");
      poll_again = false;
			break;
		}

		if (isStopping) {
			printf("Ending session...\n");
      poll_again = false;
      // TODO: End vs. RequestEnd?
			xrEndSession(session);
			break;
		}
  }
}

bool VRWindow::next_xr_frame() {

  // --- Wait for our turn to do head-pose dependent computation and render a
  // frame
  XrFrameState frameState = {.type = XR_TYPE_FRAME_STATE, .next = NULL};
  XrFrameWaitInfo frameWaitInfo = {.type = XR_TYPE_FRAME_WAIT_INFO,
                                   .next = NULL};
  result = xrWaitFrame(session, &frameWaitInfo, &frameState);
  if (!xr_result(instance, result,
                 "xrWaitFrame() was not successful, exiting..."))
    return false;

  // --- Create projection matrices and view matrices for each eye
  XrViewLocateInfo viewLocateInfo = {
                                     .type = XR_TYPE_VIEW_LOCATE_INFO,
                                     .next = NULL,
                                     .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                     .displayTime = frameState.predictedDisplayTime,
                                     .space = local_space};
  
  XrView views[view_count];
  for (uint32_t i = 0; i < view_count; i++) {
    views[i].type = XR_TYPE_VIEW;
    views[i].next = NULL;
  };

  XrViewState viewState = {.type = XR_TYPE_VIEW_STATE, .next = NULL};
  uint32_t viewCountOutput;
  result = xrLocateViews(session, &viewLocateInfo, &viewState,
                         view_count, &viewCountOutput, views);
  if (!xr_result(instance, result, "Could not locate views"))
    return false;

  // --- Begin frame
  XrFrameBeginInfo frameBeginInfo = {.type = XR_TYPE_FRAME_BEGIN_INFO,
                                     .next = NULL};

  result = xrBeginFrame(session, &frameBeginInfo);
  if (!xr_result(instance, result, "failed to begin frame!"))
    return false;

  XrCompositionLayerProjectionView projection_views[view_count];

  // render each eye and fill projection_views with the result
  for (uint32_t i = 0; i < view_count; i++) {
    XrMatrix4x4f projectionMatrix;
    XrMatrix4x4f_CreateProjectionFov(&projectionMatrix, GRAPHICS_OPENGL,
                                     views[i].fov, 0.05f, 100.0f);

    const XrVector3f uniformScale = {.x = 1.f, .y = 1.f, .z = 1.f};

    XrMatrix4x4f viewMatrix;
    XrMatrix4x4f_CreateTranslationRotationScaleRotate(
			    &viewMatrix, &views[i].pose.position, &views[i].pose.orientation,
			    &uniformScale);

    // Calculates the inverse of a rigid body transform.
    XrMatrix4x4f inverseViewMatrix;
    XrMatrix4x4f_InvertRigidBody(&inverseViewMatrix, &viewMatrix);

    XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
			    .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, .next = NULL};
    uint32_t bufferIndex;
    result = xrAcquireSwapchainImage(
			    swapchains[i], &swapchainImageAcquireInfo, &bufferIndex);
    if (!xr_result(instance, result,
                   "failed to acquire swapchain image!"))
      return false;

    XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
			    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
			    .next = NULL,
			    .timeout = 1000};
    result =
      xrWaitSwapchainImage(swapchains[i], &swapchainImageWaitInfo);
    if (!xr_result(instance, result,
                   "failed to wait for swapchain image!"))
      return false;

    projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
    projection_views[i].next = NULL;
    projection_views[i].pose = views[i].pose;
    projection_views[i].fov = views[i].fov;
    projection_views[i].subImage.swapchain = swapchains[i];
    projection_views[i].subImage.imageArrayIndex = 0;
    projection_views[i].subImage.imageRect.offset.x = 0;
    projection_views[i].subImage.imageRect.offset.y = 0;
    projection_views[i].subImage.imageRect.extent.width =
      configuration_views[i].recommendedImageRectWidth;
    projection_views[i].subImage.imageRect.extent.height =
      configuration_views[i].recommendedImageRectHeight;

    m_renderer.draw(configuration_views[i].recommendedImageRectWidth,
                configuration_views[i].recommendedImageRectHeight,
                projectionMatrix, inverseViewMatrix, &spaceLocation[0], &spaceLocation[1],
                framebuffers[i][bufferIndex], depthbuffer,
                images[i][bufferIndex], i,
                frameState.predictedDisplayTime);
    //renderFrame();
    glFinish();
    XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
			    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, .next = NULL};
    result = xrReleaseSwapchainImage(swapchains[i],
                                     &swapchainImageReleaseInfo);
    if (!xr_result(instance, result,
                   "failed to release swapchain image!"))
      return false;
  }

  projectionLayer.views = projection_views;

  // show left eye image on the quad layer
  quadLayer.subImage = projection_views[0].subImage;

  float aspect = projection_views[0].subImage.imageRect.extent.width /
    projection_views[0].subImage.imageRect.extent.width;
  // 1 pixel = 1cm = 0.001m
  quadLayer.size.width =
    projection_views[0].subImage.imageRect.extent.width * 0.001f;
  quadLayer.size.height = quadLayer.size.width / aspect;

  const XrCompositionLayerBaseHeader* const submittedLayers[] = {
		    (const XrCompositionLayerBaseHeader* const) & projectionLayer,
		    (const XrCompositionLayerBaseHeader* const) & quadLayer};
  XrFrameEndInfo frameEndInfo = {
		    .type = XR_TYPE_FRAME_END_INFO,
		    .displayTime = frameState.predictedDisplayTime,
		    .layerCount = sizeof(submittedLayers) / sizeof(submittedLayers[0]),
		    .layers = submittedLayers,
		    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
		    .next = NULL};
  result = xrEndFrame(session, &frameEndInfo);
  if (!xr_result(instance, result, "failed to end frame!"))
    return false;

  return true;
}

void VRWindow::paintGL()
{
  handle_xr_events();
  next_xr_frame();
}

void VRWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_P) { // pause
        m_animate = !m_animate;
        setAnimating(m_animate);
    }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
