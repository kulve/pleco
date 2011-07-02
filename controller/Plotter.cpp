#include "Plotter.h"

#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QWidget>
#include <QLine>

Plotter::Plotter(QWidget *parent):
  QWidget(parent), data()
{
}



Plotter::~Plotter(void)
{
}



void Plotter::push(int value)
{
  //qDebug() << "in" << __FUNCTION__ << "value:" << value;

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



void Plotter::paintEvent(QPaintEvent *)
{
  //qDebug() << "in" << __FUNCTION__;

  QPen black_pen(Qt::black, 1, Qt::SolidLine);
  QPen red_pen(Qt::red, 1, Qt::SolidLine);
  QPen white_pen(Qt::white, 1, Qt::SolidLine);

  QPainter painter(this);

  painter.setPen(black_pen);

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
  if (range < 10) {
	range = 10;
  }
  int extra = this->height() * 0.20; // 20% extra around the values

  // If range is really small, center the values in the graph
  if (range < this->height()) {
	extra = this->height() - range;
  }

  // Calcute offset to get only positive y-coordinates
  int offset_y = 0;
  offset_y = -1 * smallest;

  // Calculate the scale factor with extra around the values
  double scale = (this->height() - extra) / (double)(range);
  if (scale > 1) {
	scale = 1;
  }

  //qDebug() << __FUNCTION__ << ": min/max/range/extra/scale" << smallest << biggest << range << extra << scale;
  //qDebug() << __FUNCTION__ << ": width/height" << this->width() << this->height();

  // Create the points to draw
  QVector<QLine> lines;
  int currentValue = 0;
  int start_x = this->width() - data.size();
  for (int i = 1; i < data.size(); i++) {
	
	int x1 = start_x + i - 1;
	int x2 = start_x + i;
	int y1 = (this->height() - (int)(extra/2)) - ((int)((data.at(i-1) + offset_y) * scale));
	int y2 = (this->height() - (int)(extra/2)) - ((int)((data.at(i) + offset_y) * scale));
	lines.push_back(QLine(x1, y1, x2, y2));
	currentValue = data.at(i);
	
  }
  
  // Draw zero line
  if (smallest <= 0 && biggest >= 0) {
	painter.setPen(red_pen);
	int zero_y = (this->height() - (int)(extra/2)) - ((int)((0 + offset_y) * scale));
	painter.drawLine(0, zero_y, this->width(), zero_y);
	painter.setPen(black_pen);
  }
  
  // Draw the QLines
  painter.drawLines(lines);
  
  // Draw the min/max numbers
  // FIXME: use the height of the text instead of 10
  painter.drawText(2, 10, QString::number(biggest == -9999999 ? 0 : biggest));
  painter.drawText(2, this->height(), QString::number(smallest == 9999999 ? 0 : smallest));

  // Draw the latest value
  painter.setPen(white_pen);
  // FIXME: use the width of the text instead of 20
  painter.drawText(this->width() - 20, 10, QString::number(currentValue));
}


QSize Plotter::sizeHint(void) const
{
  qDebug() << "in" << __FUNCTION__;
  return QSize(200, 50);
}
