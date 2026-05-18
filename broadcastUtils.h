#pragma once

#include <QCoreApplication>
#include <QTimeZone>

class BroadcastTime {
    QString weekday_;
    QString time_;
    QString timezone_;

  public:
    BroadcastTime(QString weekday, QString time, QString timezone) : weekday_(weekday), time_(time), timezone_(timezone) {}

    // change to specific timezone
    bool changeTimezone(QString timezone);
};