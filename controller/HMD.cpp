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
 * Parts copied from https://github.com/OpenHMD/OpenHMD/tree/master/examples/
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

#include "HMD.h"

/*
 * Constructor for the HMD
 */
HMD::HMD(void):
  ctx(nullptr),
  hmd(nullptr),
  hmd_w(0),
  hmd_h(0),
  ipd(),
  viewport_scale(),
  distortion_coeffs(),
  aberr_scale(),
  sep(),
  left_lens_center(),
  right_lens_center()
{
  // Do something
}



/*
 * Destructor for the HMD
 */
HMD::~HMD()
{
  ohmd_ctx_destroy(ctx);
}



/*
 * Init HMD. Returns false on failure
 */
bool HMD::init()
{

  int major, minor, patch;
  ohmd_get_version(&major, &minor, &patch);
  qDebug() << "OpenHMD version: " << major <<  "." << minor  << "." << patch;

  ctx = ohmd_ctx_create();
  int num_devices = ohmd_ctx_probe(ctx);
  if (num_devices < 0){
    qDebug() <<"failed to probe devices: " << ohmd_ctx_get_error(ctx);
    return false;
  }

  qDebug() << "Detected OpenHMD devices: " << num_devices;

  // Print device information
  for(int i = 0; i < num_devices; i++){
    int device_class = 0, device_flags = 0;
    const char* device_class_s[] = {"HMD", "Controller", "Generic Tracker", "Unknown"};

    ohmd_list_geti(ctx, i, OHMD_DEVICE_CLASS, &device_class);
    ohmd_list_geti(ctx, i, OHMD_DEVICE_FLAGS, &device_flags);

    printf("device %d\n", i);
    printf("  vendor:  %s\n", ohmd_list_gets(ctx, i, OHMD_VENDOR));
    printf("  product: %s\n", ohmd_list_gets(ctx, i, OHMD_PRODUCT));
    printf("  path:    %s\n", ohmd_list_gets(ctx, i, OHMD_PATH));
    printf("  class:   %s\n", device_class_s[device_class > OHMD_DEVICE_CLASS_GENERIC_TRACKER ? 4 : device_class]);
    printf("  flags:   %02x\n",  device_flags);
    printf("    null device:         %s\n", device_flags & OHMD_DEVICE_FLAGS_NULL_DEVICE ? "yes" : "no");
    printf("    rotational tracking: %s\n", device_flags & OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING ? "yes" : "no");
    printf("    positional tracking: %s\n", device_flags & OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING ? "yes" : "no");
    printf("    left controller:     %s\n", device_flags & OHMD_DEVICE_FLAGS_LEFT_CONTROLLER ? "yes" : "no");
    printf("    right controller:    %s\n\n", device_flags & OHMD_DEVICE_FLAGS_RIGHT_CONTROLLER ? "yes" : "no");
  }

  ohmd_device_settings* settings = ohmd_device_settings_create(ctx);
  int auto_update = 1;
  ohmd_device_settings_seti(settings, OHMD_IDS_AUTOMATIC_UPDATE, &auto_update);

  ohmd_device* hmd = ohmd_list_open_device_s(ctx, 0, settings);
  if (!hmd){
    qDebug() <<"failed to open device: " << ohmd_ctx_get_error(ctx);
    return 1;
  }

  ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
  ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);
  ohmd_device_getf(hmd, OHMD_EYE_IPD, &ipd);

  //viewport is half the screen
  ohmd_device_getf(hmd, OHMD_SCREEN_HORIZONTAL_SIZE, &(viewport_scale[0]));
  viewport_scale[0] /= 2.0f;
  ohmd_device_getf(hmd, OHMD_SCREEN_VERTICAL_SIZE, &(viewport_scale[1]));
  //distortion coefficients
  ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(distortion_coeffs[0]));
  ohmd_device_getf(hmd, OHMD_UNIVERSAL_ABERRATION_K, &(aberr_scale[0]));
  //calculate lens centers (assuming the eye separation is the distance between the lens centers)
  ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);
  ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(left_lens_center[1]));
  ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(right_lens_center[1]));
  left_lens_center[0] = viewport_scale[0] - sep/2.0f;
  right_lens_center[0] = sep/2.0f;
  //assume calibration was for lens view to which ever edge of screen is further away from lens center
  //float warp_scale = (left_lens_center[0] > right_lens_center[0]) ? left_lens_center[0] : right_lens_center[0];
  //float warp_adj = 1.0f;
  ohmd_device_settings_destroy(settings);

  return true;
}

/*
 * Reset orientation and position
 */
bool HMD::reset_orientation()
{
  float zero[] = {0, 0, 0, 1};
  ohmd_device_setf(hmd, OHMD_ROTATION_QUAT, zero);
  ohmd_device_setf(hmd, OHMD_POSITION_VECTOR, zero);
  return true;
}

bool HMD::get_modelview_matrix_left(float matrix[16])
{
  ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_MODELVIEW_MATRIX, matrix);
  return true;
}

bool HMD::get_modelview_matrix_right(float matrix[16])
{
  ohmd_device_getf(hmd, OHMD_RIGHT_EYE_GL_MODELVIEW_MATRIX, matrix);
  return true;
}

bool HMD::get_projection_matrix_left(float matrix[16])
{
  ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_PROJECTION_MATRIX, matrix);
  return true;
}

bool HMD::get_projection_matrix_right(float matrix[16])
{
  ohmd_device_getf(hmd, OHMD_RIGHT_EYE_GL_PROJECTION_MATRIX, matrix);
  return true;
}

bool HMD::get_distortion_vertex_src(const char **vertex)
{
  ohmd_gets(OHMD_GLSL_DISTORTION_VERT_SRC, vertex);
  return true;
}

bool HMD::get_distortion_fragment_src(const char **fragment)
{
  ohmd_gets(OHMD_GLSL_DISTORTION_FRAG_SRC, fragment);
  return true;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
