#include "animeNotifier.h"

namespace notifier {

void AnimeNotifier::start() {
    // Load config
    conf_ = cfg::Config::load("config.json");

    // Loop thru anime searches list
    for (const auto &search : std::as_const(conf_.animeSearches_)) {
        auto searchReply = createSearchRequest(search);

        // Send anime search request
        QObject::connect(searchReply, &QNetworkReply::finished, [this]() {
            if (searchReply->error() == QNetworkReply::NoError) {
                // Search request success
                {
                    QByteArray response = searchReply->readAll();

                    // Parse JSON
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                    QJsonObject jsonObj = jsonDoc.object();

                    QJsonArray dataArr = jsonObj["data"].toArray();
                    QJsonObject firstObj = dataArr[0].toObject();
                    malId_ = firstObj["mal_id"].toInt();
                    animeTitle_ = firstObj["title"].toString();
                }
                qDebug();
                qDebug() << "Current time:" << QDateTime::currentDateTime().toString();
                qDebug() << "MAL ID:" << malId_;
                qDebug() << "Title:" << animeTitle_;

                if (!malId_) {
                    qDebug() << "MAL ID not found";
                    searchReply->deleteLater();
                    QCoreApplication::quit();
                    return;
                }

                // Use anime id from search request to get anime info with broadcast info
                QString urlInfoStr{"https://api.jikan.moe/v4/anime/" + QString::number(malId_)};
                QUrl urlInfo(urlInfoStr);
                QNetworkRequest requestInfo(urlInfo);
                replyInfo_ = manager_.get(requestInfo);

                QObject::connect(replyInfo_, &QNetworkReply::finished, [this]() {
                    if (replyInfo_->error() == QNetworkReply::NoError) {
                        QByteArray response = replyInfo_->readAll();

                        // Parse JSON
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                        QJsonObject jsonObj = jsonDoc.object();

                        QJsonObject dataObj = jsonObj["data"].toObject();
                        qDebug() << "Airing:" << dataObj["airing"].toBool();
                        bool airing = dataObj["airing"].toBool();
                        if (!airing) {
                            qDebug() << "Anime not currently airing! Exiting...";
                            replyInfo_->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        QJsonObject broadcastObj{dataObj["broadcast"].toObject()};
                        QString jpnBroadcastInfo = broadcastObj["string"].toString();
                        QString jpnDay = broadcastObj["day"].toString();
                        QString jpnTime = broadcastObj["time"].toString();
                        QString jpnTimezone = broadcastObj["timezone"].toString();
                        qDebug() << "Broadcast: " << jpnBroadcastInfo;
                        qDebug() << "Day: " << jpnDay;
                        qDebug() << "Time: " << jpnTime;
                        qDebug() << "Timezone: " << jpnTimezone;

                        QString weekdaySingular = jpnDay.chopped(1); // remove last 's' to make it singlular
                        // get next broadcast time w/ specific date
                        QDate nextBroadcastJp = broadcastUtils::nextWeekdayDate(weekdaySingular);
                        QTimeZone tokyoTz(jpnTimezone.toUtf8());
                        QList splitTime = jpnTime.split(':'); // ex. [10, 30]

                        QDateTime jpBroadcast(nextBroadcastJp, QTime(splitTime[0].toInt(), splitTime[1].toInt()),
                                              tokyoTz);
                        QDateTime localBroadcast = jpBroadcast.toLocalTime();

                        qDebug() << "Local time:" << localBroadcast.toString();

                        // make notification time from airing time + buffer for the episode to be translated
                        QDateTime notificationTime = localBroadcast.addSecs(10800); // +3 hours

                        // FOR TESTING - DELETE
                        notificationTime.setDate(QDate::currentDate());
                        notificationTime.setTime(QTime::currentTime());
                        qDebug() << "Changed notification time to" << notificationTime.toString()
                                 << "for testing purposes, remember to DELETE!!!";

                        // check if new episode notification should be sent today
                        if (!broadcastUtils::isToday(notificationTime)) {
                            qDebug() << "Notification day is not today";
                            replyInfo_->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        qDebug() << "Notification time(local):" << notificationTime.toString();
                        // check if notification time has passed
                        if (notificationTime > QDateTime::currentDateTime()) {
                            qDebug() << "The new episode is not ready to watch yet";
                            replyInfo_->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        // notify that anime episode is up
                        broadcastUtils::PushNotification notification{
                            .type = broadcastUtils::PushTypes::note,
                            .title = "'" + animeTitle_ + "' new episode released",
                            .body = "'" + animeTitle_ + "' newest episode has broadcasted and is ready to watch"};

                        if (!broadcastUtils::sendPushNotification(conf_.pushbulletToken_, manager_, notification)) {
                            qDebug() << "Failed sending push notification";
                            replyInfo_->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }
                        qDebug() << "Requested push notification";

                    } else {
                        qDebug() << "Anime info request error:" << replyInfo_->errorString();
                    }

                    replyInfo_->deleteLater();
                });
            } else {
                qDebug() << "Error:" << searchReply->errorString();
                searchReply->deleteLater();
                QCoreApplication::quit();
            }

            searchReply->deleteLater();
        });
    }
}

QNetworkReply *AnimeNotifier::createSearchRequest(const QString &search) {
    const QString API_URL = "https://api.jikan.moe/v4/anime";
    auto searchUrl = combineUrlWithQuery(API_URL, search);

    QNetworkRequest requestSearch(searchUrl);
    auto searchReply = manager_.get(requestSearch);

    return searchReply;
}

QUrl AnimeNotifier::combineUrlWithQuery(const QString &urlStr, const QString &queryItem) {
    QUrl url(urlStr);
    QUrlQuery query;
    query.addQueryItem("q", queryItem);
    url.setQuery(query);

    return url;
}
} // namespace notifier