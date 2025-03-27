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
 * Parts copied from QT base examples under BSD license
 * https://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/
 */



#ifndef _VRWINDOW_H
#define _VRWINDOW_H

#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr.h>

#include <QOpenGLWidget>

#include "VRRenderer.h"

class VRWindow : public QOpenGLWindow
{
  Q_OBJECT

public:
  VRWindow();

protected:
  void paintGL() override;
  void resizeGL(int w, int h) override;
  void keyPressEvent(QKeyEvent *e) override;

private:
  void setAnimating(bool enabled);

  QMatrix4x4 m_window_normalised_matrix;
  QMatrix4x4 m_window_painter_matrix;
  QMatrix4x4 m_projection;
  QMatrix4x4 m_view;
  QMatrix4x4 m_model_triangle;
  QMatrix4x4 m_model_text;
  QBrush m_brush;

  VRRenderer m_renderer;
  QStaticText m_text_layout;
  bool m_animate;

	XrInstance instance;
	XrSession session;
	XrSpace local_space;
	XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
	// one array of images per view
	XrSwapchainImageOpenGLKHR** images;
	XrSwapchain* swapchains;
	// Each physical Display/Eye is described by a view
	uint32_t view_count;
	XrViewConfigurationView* configuration_views;
	GLuint** framebuffers;
	GLuint depthbuffer;

  XrCompositionLayerProjection projectionLayer;
	XrCompositionLayerQuad quadLayer;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
