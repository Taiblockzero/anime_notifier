#pragma once

#include "searchChecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrlQuery>

namespace db {

class SearchChecker : public QObject {
    Q_OBJECT

  public:
    explicit SearchChecker(const QString &supabaseUrl, const QString &serviceRoleKey, QObject *parent = nullptr);
    ~SearchChecker() override = default;

    // Triggers the single-pass fetch from Supabase
    void run();

  signals:
    // Emitted when active searches are fetched successfully
    void searchesRetrieved(const QJsonArray &searches);

    // Emitted when completed or on error to gracefully exit app
    void finished(int exitCode);

  private slots:
    void onSearchesReplyFinished(QNetworkReply *reply);

  private:
    QNetworkRequest createServerRequest(const QUrl &url) const;

    QNetworkAccessManager *networkManager_;
    QString supabaseUrl_;
    QString serviceRoleKey_;
};
} // namespace db