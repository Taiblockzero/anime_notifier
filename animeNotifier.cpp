#include "animeNotifier.h"

namespace notifier {
// ----------------- Anime Job ---------------------------
AnimeJob::AnimeJob(QNetworkAccessManager &manager, const QString &search, const QString &pushbulletToken,
                   QObject *parent)
    : QObject(parent), manager_(manager), search_(search), pushbulletToken_(pushbulletToken) {}

void AnimeJob::start() {
    QUrl url(URL_STR);
    QUrlQuery q;
    q.addQueryItem("q", search_);
    url.setQuery(q);
    qDebug() << "Encoded Url:" << url.toEncoded();

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::UserAgentHeader, "WeeklyAnimeNotifier/1.0");

    searchReply_ = manager_.get(request);

    connect(searchReply_, &QNetworkReply::finished, this, &AnimeJob::onSearchFinished);
}

void AnimeJob::onSearchFinished() {
    if (searchReply_->error() != QNetworkReply::NoError) {
        qDebug() << "Qt error:" << searchReply_->error();
        qDebug() << "Error string:" << searchReply_->errorString();
        qDebug() << "HTTP status:" << searchReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        QByteArray body = searchReply_->readAll();
        qDebug() << "Response body:" << body;

        qWarning() << searchReply_->errorString();
        searchReply_->deleteLater();
        emit finished();
        return;
    }

    auto json = QJsonDocument::fromJson(searchReply_->readAll()).object();
    auto arr = json["data"].toArray();
    auto first = arr[0].toObject();

    malId_ = first["mal_id"].toInt();
    animeTitle_ = first["title"].toString();

    searchReply_->deleteLater();
    searchReply_ = nullptr;

    if (!malId_) {
        emit finished();
        return;
    }

    QUrl url(URL_STR + "/" + QString::number(malId_));
    detailReply_ = manager_.get(QNetworkRequest(url));

    connect(detailReply_, &QNetworkReply::finished, this, &AnimeJob::onDetailFinished);
}

void AnimeJob::onDetailFinished() {
    if (detailReply_->error() != QNetworkReply::NoError) {
        detailReply_->deleteLater();
        emit finished();
        return;
    }

    auto json = QJsonDocument::fromJson(detailReply_->readAll()).object();
    auto animeDataObj = json["data"].toObject();

    bool airing = animeDataObj["airing"].toBool();

    if (!airing) {
        qDebug() << "Anime not currently airing! Exiting...";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    // Parse JSON
    QJsonObject broadcastObj{animeDataObj["broadcast"].toObject()};
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
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    qDebug() << "Notification time(local):" << notificationTime.toString();
    // check if notification time has passed
    if (notificationTime > QDateTime::currentDateTime()) {
        qDebug() << "The new episode is not ready to watch yet";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    // notify that anime episode is up
    broadcastUtils::PushNotification notification{.type = broadcastUtils::PushTypes::note,
                                                  .title = "'" + animeTitle_ + "' new episode released",
                                                  .body = "'" + animeTitle_ +
                                                          "' newest episode has broadcasted and is ready to watch"};

    if (!broadcastUtils::sendPushNotification(pushbulletToken_, manager_, notification)) {
        qDebug() << "Failed sending push notification";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }
    qDebug() << "Requested push notification";

    detailReply_->deleteLater();
    detailReply_ = nullptr;

    emit finished();
}

// ----------------- Anime Notifier ----------------------
void AnimeNotifier::start() {
    conf_ = cfg::Config::load("config.json");

    animeList_ = conf_.animeSearches_;
    index_ = 0;

    runNextJob();
}

void AnimeNotifier::runNextJob() {
    qDebug() << "runNextJob index:" << index_;

    if (index_ >= animeList_.size()) {
        qDebug() << "Done with anime search list";
        return;
    }

    QString search = animeList_[index_++];

    qDebug() << "Creating job for:" << search;

    auto *job = new AnimeJob(manager_, search, conf_.pushbulletToken_, this);

    connect(job, &AnimeJob::finished, this, [this, search]() {
        qDebug() << "Finished job:" << search;
        QTimer::singleShot(1000, this, &AnimeNotifier::runNextJob);
        // runNextJob();
    });

    job->start();
}

} // namespace notifier