#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QUuid>
#include <QtTypes>

namespace db {

struct SearchRecord {
    qint64 searchId{0};
    QString search;
    QDateTime createdAt;
    QUuid userId;
    bool isActive{true};
    QDateTime lastNotifiedAt;
};

static SearchRecord fromJson(const QJsonObject &json) {
    SearchRecord res;
    res.searchId = json.value("search_id").toInteger();
    res.search = json.value("search").toString();
    res.createdAt = QDateTime::fromString(json.value("created_at").toString(), Qt::ISODateWithMs);
    res.userId = QUuid::fromString(json.value("user_id").toString());
    res.isActive = json.value("is_active").toBool();

    const QJsonValue lastNotifiedVal = json.value("last_notified_at");
    if (!lastNotifiedVal.isNull() && !lastNotifiedVal.isUndefined()) {
        res.lastNotifiedAt = QDateTime::fromString(lastNotifiedVal.toString(), Qt::ISODateWithMs);
    }
}

} // namespace db