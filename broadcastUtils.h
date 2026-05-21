#pragma once

#include "configUtils.h"

#include <QDateTime>
#include <QNetworkReply>

namespace broadcastUtils {

// Returns specific date of the next weekday from parameter
QDate nextWeekdayDate(QString targetDay);

// Convert weekday from name to number ex. "Wednesday"->3
int weekdayNumFromName(const QString &weekdayName);

// Check if qdatetime is today
bool isToday(const QDateTime &datetime);

// Send push notification to phone
bool sendPushNotification(const cfg::Config &conf, QNetworkAccessManager &manager);

} // namespace broadcastUtils