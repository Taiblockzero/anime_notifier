#include "broadcastUtils.h"
#include "qnetworkaccessmanager.h"

#include <QCoreApplication>
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

bool sendPushNotification(const cfg::Config &conf, QNetworkAccessManager &manager, const PushNotification &notification) {
    if (conf.pushbulletToken.isEmpty()) {
        qWarning() << "Missing PUSHBULLET_TOKEN!";
    }

    QNetworkRequest request(QUrl("https://api.pushbullet.com/v2/pushes"));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("Access-Token", conf.pushbulletToken.toUtf8());

    QJsonObject obj;
    if (notification.type == PushTypes::note)
        obj["type"] = "note";
    obj["title"] = notification.title;
    obj["body"] = notification.body;

    QJsonDocument doc(obj);

    QNetworkReply *reply = manager.post(request, doc.toJson());

    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Successfully pushed notification";
        } else {
            qWarning() << "Pushbullet error:" << reply->errorString();
        }

        reply->deleteLater();
        QCoreApplication::quit();
    });

    return true;
}

} // namespace broadcastUtils