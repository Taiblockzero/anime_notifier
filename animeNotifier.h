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
#include "searchChecker.h"
#include "searchRecord.h"

namespace notifier {

class AnimeJob : public QObject {
    Q_OBJECT

  public:
    AnimeJob(QNetworkAccessManager &manager, const db::SearchRecord &searchRec, const QString supabaseUrl,
             const QString supabaseServiceKey, QObject *parent = nullptr);

    void start();

  signals:
    void finished(); // tells notifier “move to next anime”

  private slots:
    void onSearchFinished();
    void onDetailFinished();
    void onDbNotificationFinished();

  private:
    QString jobTag() const;
    bool parseSearchReply();
    void requestDetail();
    bool parseDetailReply();
    void calculateLocalNotificationTime();
    /* Change notification sending time to Now if testing mode is on */
    void changeTestingModeNotifTime();
    bool communicateNotificationIsReady();
    void finishJob();
    /* Check if notification has been sent today already */
    bool notificationAlreadySentToday() const;
    /* Check if all conditions for sending a notification have been met */
    bool sendNotificationConditionsMet() const;
    void queueDatabaseNotification();

  private:
    QNetworkAccessManager &manager_;
    cfg::persistentNotificationHistory hist_;
    QString username_;
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
    db::SearchRecord searchRec_;
    QString supabaseUrl_;
    QString supabaseServiceKey_;

    QNetworkReply *searchReply_ = nullptr;
    QNetworkReply *detailReply_ = nullptr;
    QNetworkReply *notifyReply_ = nullptr;

    const QString URL_STR = "https://api.tenrai.org/v1/anime";

    bool testingMode_ = false;
};

class AnimeNotifier : public QObject {
    Q_OBJECT

  public:
    AnimeNotifier(const QString &supabaseUrl, const QString &supabaseKey);
    void start();

  private slots:
    void runNextJob();
    void onSearchesRetrieved(const QJsonArray &supabaseSearches);
    void processNextSearch();

  signals:
    void finished(int exitCode);

  private:
    QNetworkAccessManager manager_;
    std::size_t index_ = 0;
    const QString supabaseUrl_;
    const QString supabaseKey_;
    db::SearchChecker *checkerPtr_ = nullptr;
    QList<QJsonObject> searchesQueue_;

  private:
    /* Flattens config users to get from '1 user with many searches' -> '1 user with 1 search'.
       Done in order to get them ready to use with anime job(1 search per job). */
    void fillUserSearches();
};

} // namespace notifier