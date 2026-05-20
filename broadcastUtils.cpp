#include "broadcastUtils.h"

#include <QJsonObject>
#include <QNetworkRequest>

namespace broadcastUtils {

QDate nextWeekdayDate(QString targetDay) {
    QDate today = QDate::currentDate();
    // convert weekday to number
    int targetDayNum = weekdayNumFromName(targetDay);

    // create next date
    int daysToAdd = (targetDayNum - today.dayOfWeek() + 7) % 7;

    return today.addDays(daysToAdd);
}

int weekdayNumFromName(const QString &weekdayName) {
    QString weekdayNameLower = weekdayName.toLower();

    static const QMap<QString, int> days = {{"monday", 1}, {"tuesday", 2},  {"wednesday", 3}, {"thursday", 4},
                                            {"friday", 5}, {"saturday", 6}, {"sunday", 7}};

    return days.value(weekdayNameLower);
}

bool isToday(const QDateTime &datetime) { return datetime.date() == QDate::currentDate(); }

bool sendPushNotification() {
    QNetworkRequest request(QUrl("https://api.pushbullet.com/v2/pushes"));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString token = qEnvironmentVariable("PUSHBULLET_TOKEN");

    if (token.isEmpty()) {
        qWarning() << "Missing PUSHBULLET_TOKEN!";
    }
    request.setRawHeader("Access-Token", token.toUtf8());

    QJsonObject obj;
    obj["type"] = "note";
    obj["title"] = "Anime Alert";
    obj["body"] = "One Piece airs today!";

    QJsonDocument doc(obj);

    // manager.post(request, doc.toJson());
}

} // namespace broadcastUtils