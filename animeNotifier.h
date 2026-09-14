#pragma once

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

#include "broadcastUtils.h"
#include "configUtils.h"
#include "persistentNotificationHistory.h"

namespace notifier {

class AnimeJob : public QObject {
    Q_OBJECT

  public:
    AnimeJob(QNetworkAccessManager &manager, const QString &search, const QString &pushbulletToken,
             QObject *parent = nullptr);

    void start();

  signals:
    void finished(); // tells notifier “move to next anime”

  private slots:
    void onSearchFinished();
    void onDetailFinished();

  private:
    bool parseSearchReply();
    void requestDetail();
    bool parseDetailReply();
    void calculateLocalNotificationTime();
    /* Change notification sending time to Now if testing mode is on */
    void changeTestingModeNotifTime();
    bool sendAnimeIsUpNotification();
    void finishJob();
    /* Check if notification has been sent today already */
    bool notificationAlreadySentToday();

  private:
    QNetworkAccessManager &manager_;
    cfg::persistentNotificationHistory hist_;
    QString search_;

    int malId_ = 0;
    QString animeTitle_;
    QString pushbulletToken_;
    bool airing_ = false;
    QString jpnBroadcastInfo_;
    QString jpnDay_;
    QString jpnTime_;
    QString jpnTimezone_;
    QDateTime notificationTime_;

    QNetworkReply *searchReply_ = nullptr;
    QNetworkReply *detailReply_ = nullptr;

    const QString URL_STR = "https://api.tenrai.org/v1/anime";

    bool testingMode_ = false;
};

struct UserSearchTask {
    QString username_;
    QString pushbulletToken_;
    QString animeSearch_;
};

class AnimeNotifier : public QObject {
    Q_OBJECT

  public:
    void start();

  private slots:
    void runNextJob();

  private:
    QNetworkAccessManager manager_;
    std::vector<cfg::ConfigUser> users_;
    std::vector<UserSearchTask> userSearches_;
    std::size_t index_ = 0;
    cfg::Config conf_;

  private:
    /* Flattens config users to get from '1 user with many searches' -> '1 user with 1 search'.
       Done in order to get them ready to use with anime job(1 search per job). */
    void fillUserSearches();
};

} // namespace notifier