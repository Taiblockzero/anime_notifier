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
        qCritical() << "Qt error:" << searchReply_->error();
        qCritical() << "Error string:" << searchReply_->errorString();

        searchReply_->deleteLater();
        emit finished();
        return;
    }

    bool parsedSearch = parseSearchReply();

    if (!parsedSearch) {
        emit finished();
        return;
    }

    requestDetail();
}

bool AnimeJob::parseSearchReply() {
    auto json = QJsonDocument::fromJson(searchReply_->readAll()).object();
    auto arr = json["data"].toArray();
    auto first = arr[0].toObject();

    malId_ = first["mal_id"].toInt();
    animeTitle_ = first["title"].toString();

    searchReply_->deleteLater();
    searchReply_ = nullptr;

    if (!malId_)
        return false;
    return true;
}

void AnimeJob::requestDetail() {
    QUrl url(URL_STR + "/" + QString::number(malId_));
    detailReply_ = manager_.get(QNetworkRequest(url));

    connect(detailReply_, &QNetworkReply::finished, this, &AnimeJob::onDetailFinished);
}

void AnimeJob::onDetailFinished() {
    if (detailReply_->error() != QNetworkReply::NoError) {
        qCritical() << "Qt error:" << detailReply_->error();
        qCritical() << "Error string:" << detailReply_->errorString();

        detailReply_->deleteLater();
        emit finished();
        return;
    }

    parseDetailReply();

    if (!airing_) {
        qCritical() << "Anime not currently airing!";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    calculateLocalNotificationTime();

    // activate testing mode to change notification time to now
    activateTestingMode(true);

    // check if new episode notification should be sent today
    if (!broadcastUtils::isToday(notificationTime_)) {
        qDebug() << "Notification day is not today";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    qDebug() << "Notification time(local):" << notificationTime_.toString();
    // check if notification time has passed
    if (notificationTime_ > QDateTime::currentDateTime()) {
        qDebug() << "The new episode is not ready to watch yet";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    // notify that anime episode is up
    bool sentNotification = sendAnimeIsUpNotification();

    if (!sentNotification) {
        qDebug() << "Failed sending push notification api request";
        detailReply_->deleteLater();
        detailReply_ = nullptr;
        emit finished();
        return;
    }

    detailReply_->deleteLater();
    detailReply_ = nullptr;
}

void AnimeJob::parseDetailReply() {
    auto json = QJsonDocument::fromJson(detailReply_->readAll()).object();
    auto animeDataObj = json["data"].toObject();

    airing_ = animeDataObj["airing"].toBool();
    qDebug() << "Airing:" << airing_;

    auto broadcastObj = animeDataObj["broadcast"].toObject();
    jpnBroadcastInfo_ = broadcastObj["string"].toString();
    jpnDay_ = broadcastObj["day"].toString();
    jpnTime_ = broadcastObj["time"].toString();
    jpnTimezone_ = broadcastObj["timezone"].toString();
    qDebug() << "Broadcast: " << jpnBroadcastInfo_;
    qDebug() << "Day: " << jpnDay_;
    qDebug() << "Time: " << jpnTime_;
    qDebug() << "Timezone: " << jpnTimezone_;
}

void AnimeJob::calculateLocalNotificationTime() {
    QString weekdaySingular = jpnDay_.chopped(1); // remove last 's' to make it singular

    // get next broadcast time w/ specific date
    QDate nextBroadcastJp = broadcastUtils::nextWeekdayDate(weekdaySingular);
    QTimeZone tokyoTz(jpnTimezone_.toUtf8());
    QList splitTime = jpnTime_.split(':'); // ex. [10, 30]

    QDateTime jpBroadcast(nextBroadcastJp, QTime(splitTime[0].toInt(), splitTime[1].toInt()), tokyoTz);
    QDateTime localBroadcast = jpBroadcast.toLocalTime();

    qDebug() << "Local time:" << localBroadcast.toString();

    // make notification time from airing time + buffer for the episode to be translated
    notificationTime_ = localBroadcast.addSecs(10800); // +3 hours
}

void AnimeJob::activateTestingMode(bool activate) {
    if (!activate)
        return;

    notificationTime_.setDate(QDate::currentDate());
    notificationTime_.setTime(QTime::currentTime());
    qDebug() << "Changed notification time to" << notificationTime_.toString()
             << "for testing purposes, remember to DELETE!!!";
}

bool AnimeJob::sendAnimeIsUpNotification() {
    broadcastUtils::PushNotification notification{.type = broadcastUtils::PushTypes::note,
                                                  .title = "'" + animeTitle_ + "' new episode released",
                                                  .body = "'" + animeTitle_ +
                                                          "' newest episode has broadcasted and is ready to watch"};

    QNetworkReply *pushbulletReply = broadcastUtils::sendPushNotification(pushbulletToken_, manager_, notification);

    if (nullptr == pushbulletReply) {
        qDebug() << "Failed sending push notification";
        return false;
    }

    qDebug() << "Requested push notification";

    QObject::connect(pushbulletReply, &QNetworkReply::finished, [this, pushbulletReply]() {
        if (pushbulletReply->error() == QNetworkReply::NoError) {
            qDebug() << "Successfully pushed notification";
        } else {
            qWarning() << "Pushbullet error:" << pushbulletReply->errorString();
        }

        pushbulletReply->deleteLater();
        emit finished();
    });

    return true;
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
        QCoreApplication::quit();
        return;
    }

    QString search = animeList_[index_++];

    qDebug() << "Creating job for:" << search;

    auto *job = new AnimeJob(manager_, search, conf_.pushbulletToken_, this);

    connect(job, &AnimeJob::finished, this, [this, search]() {
        qDebug() << "Finished job:" << search;
        QTimer::singleShot(1000, this, &AnimeNotifier::runNextJob);
    });

    job->start();
}

} // namespace notifier