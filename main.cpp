#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QtTest>
#include <iostream>

#include "broadcastUtils.h"
#include "configUtils.h"

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    cfg::Config conf = cfg::Config::load("config.json");

    QNetworkAccessManager manager;

    int64_t malId = 0;
    QString animeTitle;
    QNetworkReply *replyInfo = nullptr;
    {
        QString search;
        qDebug() << "Type something:";
        QTextStream in(stdin);
        search = in.readLine();
        // QString search = "re zero season 4";

        QUrl urlSearch("https://api.jikan.moe/v4/anime");
        QUrlQuery query;
        query.addQueryItem("q", search);
        urlSearch.setQuery(query);

        QNetworkRequest requestSearch(urlSearch);

        QNetworkReply *replySearch = manager.get(requestSearch);

        QObject::connect(replySearch, &QNetworkReply::finished, [&, search, replySearch]() {
            if (replySearch->error() == QNetworkReply::NoError) {
                // Search request success
                {
                    QByteArray response = replySearch->readAll();

                    // Parse JSON
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                    QJsonObject jsonObj = jsonDoc.object();

                    QJsonArray dataArr = jsonObj["data"].toArray();
                    QJsonObject firstObj = dataArr[0].toObject();
                    malId = firstObj["mal_id"].toInt();
                    animeTitle = firstObj["title"].toString();
                }
                qDebug() << "MAL ID:" << malId;
                qDebug() << "Title:" << animeTitle;

                if (!malId) {
                    qDebug() << "MAL ID not found";
                    replySearch->deleteLater();
                    QCoreApplication::quit();
                    return;
                }

                // Use anime id from search request to get anime info with broadcast info
                QString urlInfoStr{"https://api.jikan.moe/v4/anime/" + QString::number(malId)};
                QUrl urlInfo(urlInfoStr);
                QNetworkRequest requestInfo(urlInfo);
                replyInfo = manager.get(requestInfo);

                QObject::connect(replyInfo, &QNetworkReply::finished, [&, search]() {
                    if (replyInfo->error() == QNetworkReply::NoError) {
                        QByteArray response = replyInfo->readAll();

                        // Parse JSON
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                        QJsonObject jsonObj = jsonDoc.object();

                        QJsonObject dataObj = jsonObj["data"].toObject();
                        qDebug() << "Airing:" << dataObj["airing"].toBool();
                        bool airing = dataObj["airing"].toBool();
                        if (!airing) {
                            qDebug() << "Anime not currently airing! Exiting...";
                            replyInfo->deleteLater();
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

                        QDateTime jpBroadcast(nextBroadcastJp, QTime(splitTime[0].toInt(), splitTime[1].toInt()), tokyoTz);
                        QDateTime localBroadcast = jpBroadcast.toLocalTime();

                        qDebug() << "Local time:" << localBroadcast.toString();

                        // DELETE
                        localBroadcast.setDate(QDate(2026, 5, 22));
                        localBroadcast.setTime(QTime(10, 15));

                        // check if new episode comes out today
                        if (!broadcastUtils::isToday(localBroadcast)) {
                            qDebug() << "Anime is not airing today";
                            replyInfo->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        // make notification time from airing time + buffer for the episode to be translated
                        QDateTime notificationTime = localBroadcast.addSecs(10800); // +3 hours
                        qDebug() << "Notification time(local):" << notificationTime.toString();
                        // check if notification time has passed
                        if (notificationTime > QDateTime::currentDateTime()) {
                            qDebug() << "The new episode is not ready to watch yet";
                            replyInfo->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        // notify that anime episode is up
                        broadcastUtils::PushNotification notification{.type = broadcastUtils::PushTypes::note,
                                                                      .title = search + " is up",
                                                                      .body = search + " has broadcasted and is ready to watch"};

                        if (!broadcastUtils::sendPushNotification(conf, manager, notification)) {
                            qDebug() << "Failed sending push notification";
                            replyInfo->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }
                        qDebug() << "Requested push notification";

                    } else {
                        qDebug() << "Anime info request error:" << replyInfo->errorString();
                    }

                    replyInfo->deleteLater();
                });
            } else {
                qDebug() << "Error:" << replySearch->errorString();
                replySearch->deleteLater();
                QCoreApplication::quit();
            }

            replySearch->deleteLater();
        });
    }

    return QCoreApplication::exec();
}