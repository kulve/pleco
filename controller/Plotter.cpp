#include "Plotter.h"

#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QWidget>

Plotter::Plotter(QWidget *parent):
  QWidget(parent), data()
{
}



Plotter::~Plotter(void)
{
}



void Plotter::push(int value)
{
  qDebug() << "in" << __FUNCTION__;

  // FIXME: use static ring buffer?
  
  // Add new value to vector
  data.push_back(value);

  // Remove extra data from the start of the vector
  if (data.size() - this->width() > 0) {
	data.pop_front();
  }
  
  //this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  // Repaint the widget when appropriate
  this->update();
}



void Plotter::paintEvent(QPaintEvent *event)
{
  qDebug() << "in" << __FUNCTION__;

  QPen pen(Qt::black, 1, Qt::SolidLine);

  QPainter painter(this);

  painter.setPen(pen);

  // Transparent gray background
  painter.fillRect(this->rect(), QColor(64, 64, 64, 200));

  // Find out the smallest and the biggest values
  int smallest = 9999999;
  int biggest = -9999999;
  for (int i = 0; i < data.size(); i++) {
	if (data.at(i) < smallest) {
	  smallest = data.at(i);
	}
	if (data.at(i) > biggest) {
	  biggest = data.at(i);
	}
  }

  // The range between the smallest and the biggest values
  int range = biggest - smallest;
  
  // Calcute offset to get only positive y-coordinates
  int offset_y = 0;
  if (smallest < 0) {
	offset_y = -1 * smallest;
  }

  // Calculate the scale factor with 20% extra around the values
  double scale = (range * 1.20) / (double)this->height();

  // Create the points to draw
  QPoint points[data.size()];
  int start_x = this->width() - data.size();
  for (int i = 0; i < data.size(); i++) {
	points[i].setX(start_x + i);
	points[i].setY((int)((data.at(i) + offset_y) * scale));
  }

  painter.drawPoints(points, data.size());
}


QSize Plotter::sizeHint(void) const
{
  qDebug() << "in" << __FUNCTION__;
  return QSize(200, 50);
}
