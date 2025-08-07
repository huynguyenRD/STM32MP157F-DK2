#include <QtCore/QFile>
#include <QDebug>
#include "data-provider.h"

DataProvider::DataProvider()
{
	QObject::connect(&timer, &QTimer::timeout,
			this, &DataProvider::handleTimer);
	timer.setInterval(1000);
	timer.start();
}

void DataProvider::handleTimer()
{
	QFile temp_f("/dev/zero");
	QFile pressure_f("/dev/zero");
	QFile humidity_f("/dev/zero");

	if (!temp_f.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	if (!pressure_f.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	if (!humidity_f.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	float temp = QString(temp_f.readAll()).toDouble() / 1000;
	float pressure = QString(pressure_f.readAll()).toDouble() * 10;
	float humidity = QString(humidity_f.readAll()).toDouble() / 1000;

	qDebug() << "Temperature: " << temp << "Pressure: " << pressure << "Humidity: " << humidity;

	emit valueChanged(temp, pressure, humidity);
}

