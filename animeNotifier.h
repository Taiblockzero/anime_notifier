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

  private:
    QNetworkAccessManager &manager_;
    QString search_;

    int malId_ = 0;
    QString animeTitle_;
    QString pushbulletToken_;

    QNetworkReply *searchReply_ = nullptr;
    QNetworkReply *detailReply_ = nullptr;

    const QString URL_STR = "https://api.tenrai.org/v1/anime";
};

class AnimeNotifier : public QObject {
    Q_OBJECT

  public:
    void start();

  private slots:
    void runNextJob();

  private:
    QNetworkAccessManager manager_;
    QStringList animeList_;
    int index_ = 0;
    cfg::Config conf_;
};

} // namespace notifier