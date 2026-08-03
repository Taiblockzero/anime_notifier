#pragma once

#include "configUtils.h"

#include <QDateTime>
#include <QNetworkReply>

namespace broadcastUtils {

enum class PushTypes { note };

struct PushNotification {
    PushTypes type;
    QString title;
    QString body;
};

const QString PUSHBULLET_URL = "https://api.pushbullet.com/v2/pushes";

// Returns specific date of the next weekday from parameter
QDate nextWeekdayDate(QString targetDay);

// Convert weekday from name to number ex. "Wednesday"->3
int weekdayNumFromName(const QString &weekdayName);

// Check if qdatetime is today
bool isToday(const QDateTime &datetime);

// Send push notification to phone
// Sends request to pushbullet and returns reply for caller to handle
QNetworkReply *sendPushNotification(const QString &pushbulletToken, QNetworkAccessManager &manager,
                                    const PushNotification &notification);

} // namespace broadcastUtils