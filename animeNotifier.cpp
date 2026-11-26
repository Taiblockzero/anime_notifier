#include "animeNotifier.h"

namespace notifier {
// ----------------- Anime Job ---------------------------
AnimeJob::AnimeJob(QNetworkAccessManager &manager, const db::SearchRecord &searchRec, const QString supabaseUrl,
                   const QString supabaseServiceKey, QObject *parent)
    : QObject(parent), manager_(manager), searchRec_(searchRec), supabaseUrl_(supabaseUrl),
      supabaseServiceKey_(supabaseServiceKey) {}

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
    // Process API reply
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

    // Prerequisites for checking conditions
    calculateLocalNotificationTime();
    // activate testing mode if needed to change notification time to now
    testingMode_ = false;
    changeTestingModeNotifTime();

    // Check conditions for sending notification
    if (!sendNotificationConditionsMet()) {
        finishJob();
        return;
    }

    // Send notification that anime episode is up
    qInfo().noquote() << jobTag() << "Episode is ready! Sending push notification...";
    bool requestedNotification = communicateNotificationIsReady();

    if (!requestedNotification) {
        qWarning().noquote() << jobTag() << "Failed to dispatch push notification request.";
        finishJob();
        return;
    }

    // Update latest notification sent date in persistent settings
    hist_.setLatestDate(malId_, notificationTime_.date());

    detailReply_->deleteLater();
    detailReply_ = nullptr;
}

void AnimeJob::onDbNotificationFinished() {
    notifyReply_->deleteLater();
    QNetworkReply *reply = notifyReply_;
    notifyReply_ = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        qCritical().noquote() << jobTag() << "Failed to queue notification in Supabase:" << reply->errorString()
                              << reply->readAll();
        finishJob();
        return;
    }

    // PostgREST returns true or false
    const QByteArray response = reply->readAll().trimmed();
    if (response == "true") {
        qInfo().noquote() << jobTag() << "Notification queued and last_notified_at updated.";
    } else {
        qInfo().noquote() << jobTag()
                          << "Skipped DB insert (already notified today or search record no longer exists).";
    }

    finishJob();
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

bool AnimeJob::communicateNotificationIsReady() {
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

bool AnimeJob::notificationAlreadySentToday() const {
    // Don't skip sending notifications when in testing mode
    if (testingMode_)
        return false;

    if (hist_.getLatestDate(malId_) == notificationTime_.date())
        return true;
    return false;
}

bool AnimeJob::sendNotificationConditionsMet() const {
    // is anime currently airing
    if (!airing_) {
        qInfo().noquote() << jobTag() << "Anime is not currently airing. Skipping.";
        return false;
    }

    // is notification already sent today
    if (notificationAlreadySentToday()) {
        qInfo().noquote() << jobTag()
                          << QString("Notification already sent today (%1). Skipping.")
                                 .arg(notificationTime_.date().toString("yyyy-MM-dd"));
        return false;
    }

    // should new episode notification be sent today
    if (!broadcastUtils::isToday(notificationTime_)) {
        qInfo().noquote() << jobTag()
                          << QString("Next episode date is %1 (not today). Skipping.")
                                 .arg(notificationTime_.date().toString("yyyy-MM-dd"));
        return false;
    }

    // has notification time passed
    if (notificationTime_ > QDateTime::currentDateTime()) {
        qInfo().noquote() << jobTag()
                          << QString("Episode airs today but is not ready yet (ready at %1). Skipping.")
                                 .arg(notificationTime_.time().toString("HH:mm"));
        return false;
    }

    return true;
}

void AnimeJob::queueDatabaseNotification() {
    const QUrl url(QString("%1/rest/v1/rpc/queue_anime_notification").arg(supabaseUrl_));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("apikey", supabaseServiceKey_.toUtf8());
    request.setRawHeader("Authorization", QString("Bearer %1").arg(supabaseServiceKey_).toUtf8());

    QJsonObject payload;
    payload["p_search_id"] = searchRec_.searchId;
    payload["p_user_id"] = searchRec_.userId.toString(QUuid::WithoutBraces);
    payload["p_title"] = QString("%1 — New Episode Ready").arg(animeTitle_);
    payload["p_body"] = QString("Episode aired at %1").arg(notificationTime_.toString("HH:mm"));

    notifyReply_ = manager_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(notifyReply_, &QNetworkReply::finished, this, &AnimeJob::onDbNotificationFinished);
}

// ----------------- Anime Notifier ----------------------
AnimeNotifier::AnimeNotifier(const QString &supabaseUrl, const QString &supabaseKey)
    : supabaseUrl_(supabaseUrl), supabaseKey_(supabaseKey) {}

void AnimeNotifier::start() {
    index_ = 0;
    checkerPtr_ = new db::SearchChecker(supabaseUrl_, supabaseKey_, this);
    connect(checkerPtr_, &db::SearchChecker::searchesRetrieved, this, &AnimeNotifier::onSearchesRetrieved);

    connect(checkerPtr_, &db::SearchChecker::finished, this, [this](int exitCode) {
        if (exitCode != 0) {
            qCritical() << "db::SearchChecker failed with exit code" << exitCode;
            emit finished(exitCode);
        }
    });

    qInfo() << "Starting AnimeNotifier. Triggering db::SearchChecker...";
    checkerPtr_->run();
}

void AnimeNotifier::onSearchesRetrieved(const QJsonArray &supabaseSearches) {
    if (supabaseSearches.empty()) {
        qWarning() << "No searches found in Supabase!";
        emit finished(1);
        return;
    }

    qInfo() << "Retrieved" << supabaseSearches.size() << "searches from Supabase";

    searchesQueue_.clear();
    for (const QJsonValue &searchVal : supabaseSearches) {
        if (searchVal.isObject()) {
            searchesQueue_.append(searchVal.toObject());
        }
    }

    processNextSearch();
}

void AnimeNotifier::processNextSearch() {
    if (searchesQueue_.empty()) {
        qInfo() << "Finished processing searches";
        emit finished(0);
        return;
    }

    QJsonObject currentSearch = searchesQueue_.takeFirst();
    db::SearchRecord currentRecord = db::fromJson(currentSearch);

    qInfo() << "Processing Search ID:" << currentRecord.searchId << "| Term:" << currentRecord.search
            << "| Last Notified:"
            << (currentRecord.lastNotifiedAt.isNull() ? "Never" : currentRecord.lastNotifiedAt.toString());

    // run Anime Job

    // TODO: Perform scrape/match check here.
    // Call processNextSearch() when this item finishes.
}

} // namespace notifier