#include "broadcastUtils.h"
#include "qnetworkaccessmanager.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

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

QNetworkReply *sendPushNotification(const QString &pushbulletToken, QNetworkAccessManager &manager,
                                    const PushNotification &notification) {
    if (pushbulletToken.isEmpty()) {
        qWarning() << "Missing PUSHBULLET_TOKEN!";
        return nullptr;
    }

    QNetworkRequest request((QUrl(PUSHBULLET_URL)));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("Access-Token", pushbulletToken.toUtf8());

    QJsonObject obj;
    if (notification.type == PushTypes::note)
        obj["type"] = "note";
    obj["title"] = notification.title;
    obj["body"] = notification.body;

    QJsonDocument doc(obj);

    return manager.post(request, doc.toJson());
}

} // namespace broadcastUtils