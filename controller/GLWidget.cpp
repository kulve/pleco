/****************************************************************************
 **
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** Commercial Usage
 ** Licensees holding valid Qt Commercial licenses may use this file in
 ** accordance with the Qt Commercial License Agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Nokia.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Nokia gives you certain additional
 ** rights.  These rights are described in the Nokia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
 ** If you have questions regarding the use of this file, please contact
 ** Nokia at qt-info@nokia.com.
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include <QtGui>
#include <QtOpenGL>

#include <math.h>

#include "GLWidget.h"
#include "qtlogo.h"


#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

GLWidget::GLWidget(QWidget *parent)
  : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
  logo = 0;
  yawRot = 0;
  pitchRot = 0;
  rollRot = 0;

  qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
  qtPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
  return QSize(400, 400);
}

static void qNormalizeAngle(int &angle)
{
  while (angle < -180)
	angle = 180;
  while (angle > 180)
	angle = 180;
}

void GLWidget::setYawRotation(int angle)
{
  qNormalizeAngle(angle);
  if (angle != yawRot) {
	yawRot = angle;
	emit yawRotationChanged(angle);
	updateGL();
  }
}

void GLWidget::setPitchRotation(int angle)
{
  qNormalizeAngle(angle);
  if (angle != pitchRot) {
	pitchRot = angle;
	emit pitchRotationChanged(angle);
	updateGL();
  }
}

void GLWidget::setRollRotation(int angle)
{
  qNormalizeAngle(angle);
  if (angle != rollRot) {
	rollRot = angle;
	emit rollRotationChanged(angle);
	updateGL();
  }
}

void GLWidget::initializeGL()
{
  qglClearColor(qtPurple.dark());

  logo = new QtLogo(this, 64);
  logo->setColor(qtGreen.dark());

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_MULTISAMPLE);
  static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

void GLWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  glTranslatef(0.0, 0.0, -10.0);

  qDebug() << "FOO:" << yawRot << pitchRot << rollRot;

  glRotatef(-rollRot , 0.0, 0.0, 1.0);
  glRotatef( pitchRot, 1.0, 0.0, 0.0);
  glRotatef(-yawRot,   0.0, 1.0, 0.0);  

  logo->draw();
}

void GLWidget::resizeGL(int width, int height)
{
  int side = qMin(width, height);
  glViewport((width - side) / 2, (height - side) / 2, side, side);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
#ifdef QT_OPENGL_ES_1
  glOrthof(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
#else
  glOrtho(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
#endif
  glMatrixMode(GL_MODELVIEW);
}
