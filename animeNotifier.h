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
class AnimeNotifier {
  public:
    void start();

  private:
    QNetworkAccessManager manager_;
    QNetworkReply *replySearch_ = nullptr;
    QNetworkReply *replyInfo_ = nullptr;
    int64_t malId_ = 0;
    QString animeTitle_;
    cfg::Config conf_;
};

} // namespace notifier