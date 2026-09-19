#include "animeNotifier.h"

namespace notifier {
// ----------------- Anime Job ---------------------------
AnimeJob::AnimeJob(QNetworkAccessManager &manager, const QString &username, const QString &search,
                   const QString &pushbulletToken, QObject *parent)
    : QObject(parent), manager_(manager), username_(username), search_(search), pushbulletToken_(pushbulletToken) {}

QString AnimeJob::jobTag() const {
    if (!animeTitle_.isEmpty()) {
        return QString("[%1 | '%2']").arg(username_, animeTitle_);
    }
    return QString("[%1 | '%2']").arg(username_, search_);
}

void AnimeJob::start() {
    QUrl url(URL_STR);
    QUrlQuery q;
    q.addQueryItem("q", search_);
    url.setQuery(q);
    qDebug().noquote() << jobTag() << "Querying MAL search API:" << url.toString();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "WeeklyAnimeNotifier/1.0");

    searchReply_ = manager_.get(request);
    connect(searchReply_, &QNetworkReply::finished, this, &AnimeJob::onSearchFinished);
}

void AnimeJob::onSearchFinished() {
    if (searchReply_->error() != QNetworkReply::NoError) {
        qCritical().noquote() << jobTag() << "Search network error:" << searchReply_->errorString()
                              << QString("(code: %1)").arg(searchReply_->error());
        finishJob();
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
    if (arr.isEmpty()) {
        qWarning().noquote() << jobTag() << "No matching anime found for search query.";
        return false;
    }
    auto first = arr.first().toObject();

    malId_ = first["mal_id"].toInt();
    animeTitle_ = first["title"].toString();

    qInfo().noquote() << jobTag() << QString("Matched: '%1' (MAL ID: %2)").arg(animeTitle_).arg(malId_);

    searchReply_->deleteLater();
    searchReply_ = nullptr;

    return malId_ != 0;
}

void AnimeJob::requestDetail() {
    QUrl url(URL_STR + "/" + QString::number(malId_));
    qDebug().noquote() << jobTag() << "Requesting anime details from:" << url.toString();
    detailReply_ = manager_.get(QNetworkRequest(url));

    connect(detailReply_, &QNetworkReply::finished, this, &AnimeJob::onDetailFinished);
}

void AnimeJob::onDetailFinished() {
    if (detailReply_->error() != QNetworkReply::NoError) {
        qCritical().noquote() << jobTag() << "Detail network error:" << detailReply_->errorString()
                              << QString("(code: %1)").arg(detailReply_->error());
        finishJob();
        return;
    }

    bool parsedDetail = parseDetailReply();
    if (!parsedDetail) {
        finishJob();
        return;
    }

    if (!airing_) {
        qInfo().noquote() << jobTag() << "Anime is not currently airing. Skipping.";
        finishJob();
        return;
    }

    calculateLocalNotificationTime();

    // activate testing mode to change notification time to now
    testingMode_ = false;
    changeTestingModeNotifTime();

    // Notification already sent today
    if (notificationAlreadySentToday()) {
        qInfo().noquote() << jobTag() << QString("Notification already sent today (%1). Skipping.")
                                .arg(notificationTime_.date().toString("yyyy-MM-dd"));
        finishJob();
        return;
    }

    // check if new episode notification should be sent today
    if (!broadcastUtils::isToday(notificationTime_)) {
        qInfo().noquote() << jobTag() << QString("Next episode date is %1 (not today). Skipping.")
                                .arg(notificationTime_.date().toString("yyyy-MM-dd"));
        finishJob();
        return;
    }

    // check if notification time has passed
    if (notificationTime_ > QDateTime::currentDateTime()) {
        qInfo().noquote() << jobTag() << QString("Episode airs today but is not ready yet (ready at %1). Skipping.")
                                .arg(notificationTime_.time().toString("HH:mm"));
        finishJob();
        return;
    }

    // notify that anime episode is up
    qInfo().noquote() << jobTag() << "Episode is ready! Sending push notification...";
    bool requestedNotification = sendAnimeIsUpNotification();

    if (!requestedNotification) {
        qWarning().noquote() << jobTag() << "Failed to dispatch push notification request.";
        finishJob();
        return;
    }

    // update latest notification sent date in persistent settings
    hist_.setLatestDate(malId_, notificationTime_.date());

    detailReply_->deleteLater();
    detailReply_ = nullptr;
}

bool AnimeJob::parseDetailReply() {
    QJsonParseError err;

    auto json = QJsonDocument::fromJson(detailReply_->readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning().noquote() << jobTag() << "JSON parsing error on anime detail reply:" << err.errorString();
        return false;
    }

    auto animeDataObj = json.object().value("data").toObject();
    if (animeDataObj.isEmpty()) {
        qWarning().noquote() << jobTag() << "Anime detail response has empty 'data' field.";
        return false;
    }

    airing_ = animeDataObj["airing"].toBool();

    auto broadcastObj = animeDataObj["broadcast"].toObject();
    jpnBroadcastInfo_ = broadcastObj["string"].toString();
    jpnDay_ = broadcastObj["day"].toString();
    jpnTime_ = broadcastObj["time"].toString();
    jpnTimezone_ = broadcastObj["timezone"].toString();

    if (jpnDay_.isEmpty() || jpnTime_.isEmpty() || jpnTimezone_.isEmpty()) {
        qWarning().noquote() << jobTag() << "Anime is missing broadcast schedule info (day/time/timezone).";
        return false;
    }

    return true;
}

void AnimeJob::calculateLocalNotificationTime() {
    QString weekdaySingular = jpnDay_.chopped(1); // remove last 's' to make it singular

    // get next broadcast time w/ specific date
    QDate nextBroadcastJp = broadcastUtils::nextWeekdayDate(weekdaySingular);
    QTimeZone tokyoTz(jpnTimezone_.toUtf8());
    QList splitTime = jpnTime_.split(':'); // ex. [10, 30]

    QDateTime jpBroadcast(nextBroadcastJp, QTime(splitTime[0].toInt(), splitTime[1].toInt()), tokyoTz);
    QDateTime localBroadcast = jpBroadcast.toLocalTime();

    // make notification time from airing time + buffer for the episode to be translated
    notificationTime_ = localBroadcast.addSecs(10800); // +3 hours

    qDebug().noquote() << jobTag() << QString("Broadcast: %1 (%2 at %3 %4) -> Local air: %5 -> Scheduled notification (+3h buffer): %6")
                                  .arg(jpnBroadcastInfo_, jpnDay_, jpnTime_, jpnTimezone_,
                                       localBroadcast.toString("yyyy-MM-dd HH:mm"),
                                       notificationTime_.toString("yyyy-MM-dd HH:mm"));
}

void AnimeJob::changeTestingModeNotifTime() {
    if (!testingMode_)
        return;

    notificationTime_.setDate(QDate::currentDate());
    notificationTime_.setTime(QTime::currentTime());
    qWarning().noquote() << jobTag()
                         << QString("TESTING MODE ACTIVE: Overrode notification time to now (%1)")
                                .arg(notificationTime_.toString("yyyy-MM-dd HH:mm:ss"));
}

bool AnimeJob::sendAnimeIsUpNotification() {
    broadcastUtils::PushNotification notification{.type = broadcastUtils::PushTypes::note,
                                                  .title = "'" + animeTitle_ + "' new episode released",
                                                  .body = "'" + animeTitle_ +
                                                          "' newest episode has broadcasted and is ready to watch"};

    QNetworkReply *pushbulletReply = broadcastUtils::sendPushNotification(pushbulletToken_, manager_, notification);

    if (nullptr == pushbulletReply) {
        qWarning().noquote() << jobTag() << "Failed to initialize Pushbullet request (empty token or invalid request).";
        return false;
    }

    qDebug().noquote() << jobTag() << "Dispatched Pushbullet notification request.";

    QObject::connect(pushbulletReply, &QNetworkReply::finished, [this, pushbulletReply]() {
        if (pushbulletReply->error() == QNetworkReply::NoError) {
            qInfo().noquote() << jobTag() << "Pushbullet notification delivered successfully.";
        } else {
            qWarning().noquote() << jobTag() << "Pushbullet notification failed:" << pushbulletReply->errorString();
        }

        pushbulletReply->deleteLater();
        emit finished();
    });

    return true;
}

void AnimeJob::finishJob() {
    if (searchReply_) {
        searchReply_->deleteLater();
        searchReply_ = nullptr;
    }

    if (detailReply_) {
        detailReply_->deleteLater();
        detailReply_ = nullptr;
    }

    emit finished();
}

bool AnimeJob::notificationAlreadySentToday() {
    // Don't skip sending notifications when in testing mode
    if (testingMode_)
        return false;

    if (hist_.getLatestDate(malId_) == notificationTime_.date())
        return true;
    return false;
}

// ----------------- Anime Notifier ----------------------
void AnimeNotifier::start() {
    conf_ = cfg::Config::load("config.json");

    users_ = conf_.users;
    index_ = 0;

    fillUserSearches();

    qInfo().noquote() << QString("Loaded configuration: %1 user(s), %2 total anime search(es).")
                             .arg(users_.size())
                             .arg(userSearches_.size());

    runNextJob();
}

void AnimeNotifier::runNextJob() {
    if (index_ >= userSearches_.size()) {
        qInfo().noquote() << QString("All %1 anime search job(s) completed.").arg(userSearches_.size());
        QCoreApplication::quit();
        return;
    }

    UserSearchTask userSearch = userSearches_[index_++];

    qInfo().noquote() << QString("[%1/%2] Starting check for user '%3' (search: '%4')")
                             .arg(index_)
                             .arg(userSearches_.size())
                             .arg(userSearch.username_, userSearch.animeSearch_);

    auto *job = new AnimeJob(manager_, userSearch.username_, userSearch.animeSearch_, userSearch.pushbulletToken_, this);

    connect(job, &AnimeJob::finished, this, [this, userSearch]() {
        qDebug().noquote() << QString("[%1 | '%2'] Job completed.")
                                  .arg(userSearch.username_, userSearch.animeSearch_);
        QTimer::singleShot(1000, this, &AnimeNotifier::runNextJob);
    });

    job->start();
}

void AnimeNotifier::fillUserSearches() {
    if (users_.empty()) {
        qWarning().noquote() << "User list is empty! Check config.json.";
        return;
    }

    for (const auto &user : users_) {
        for (const auto &search : user.animeSearches_) {
            UserSearchTask tempSearchTask{
                .username_ = user.username_, .pushbulletToken_ = user.pushbulletToken_, .animeSearch_ = search};
            userSearches_.push_back(tempSearchTask);
        }
    }
}

} // namespace notifier