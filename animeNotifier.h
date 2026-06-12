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
class AnimeNotifier : public QObject {
    Q_OBJECT

  public:
    explicit AnimeNotifier(QObject *parent = nullptr);

    void start();

  private slots:
    void onFoundAnimeInfo();
    void onFoundAnimeDetails();

  private:
    QNetworkReply *createSearchRequest(const QString &search);
    QUrl combineUrlWithQuery(const QString &urlStr, const QString &queryItem);

  private:
    QNetworkAccessManager manager_;
    QNetworkReply *replyInfo_ = nullptr;
    int64_t malId_ = 0;
    QString animeTitle_;
    cfg::Config conf_;
};

} // namespace notifier