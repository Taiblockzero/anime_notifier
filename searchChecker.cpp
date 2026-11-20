#include "searchChecker.h"
#include <QDebug>

namespace db {

SearchChecker::SearchChecker(const QString &supabaseUrl, const QString &serviceRoleKey, QObject *parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)), supabaseUrl_(supabaseUrl),
      serviceRoleKey_(serviceRoleKey) {}

QNetworkRequest SearchChecker::createServerRequest(const QUrl &url) const {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("apikey", serviceRoleKey_.toUtf8());
    request.setRawHeader("Authorization", QString("Bearer %1").arg(serviceRoleKey_).toUtf8());
    return request;
}

void SearchChecker::run() {
    QUrl url(QString("%1/rest/v1/searches").arg(supabaseUrl_));
    QUrlQuery query;

    // Retrieve search_id, user_id (UUID), and search string
    query.addQueryItem("select", "search_id,user_id,search");
    query.addQueryItem("is_active", "eq.true");
    query.addQueryItem("order", "created_at.desc");
    url.setQuery(query);

    QNetworkRequest request = createServerRequest(url);
    QNetworkReply *reply = networkManager_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onSearchesReplyFinished(reply); });
}

void SearchChecker::onSearchesReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network Error fetching searches:" << reply->errorString();
        emit finished(1); // Exit code 1 (failure)
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    if (!jsonDoc.isArray()) {
        qWarning() << "Invalid JSON response from Supabase.";
        emit finished(1);
        return;
    }

    QJsonArray searches = jsonDoc.array();

    // Hand off the searches array containing user_id, search_id, and search string
    emit searchesRetrieved(searches);
}
} // namespace db