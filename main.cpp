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

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QNetworkAccessManager manager;

    int64_t malId = 0;
    QNetworkReply *replyInfo = nullptr;

    {
        // QUrl urlSearch("https://api.jikan.moe/v4/anime?q=youjo%20senki");
        QUrl urlSearch("https://api.jikan.moe/v4/anime?q=re%20zero%20season%204");
        QNetworkRequest requestSearch(urlSearch);

        QNetworkReply *replySearch = manager.get(requestSearch);

        QObject::connect(replySearch, &QNetworkReply::finished, [&]() {
            if (replySearch->error() == QNetworkReply::NoError) {
                // Search request success
                {
                    QByteArray response = replySearch->readAll();

                    // Parse JSON
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                    QJsonObject jsonObj = jsonDoc.object();

                    QJsonArray dataArr = jsonObj["data"].toArray();
                    QJsonObject firstObj = dataArr[0].toObject();
                    malId = firstObj["mal_id"].toInt();
                }
                qDebug() << "MAL ID:" << malId;

                // Use anime id from search request to get anime info with broadcast info
                // if (!malId)
                // {

                // }
                std::string urlInfoStr{"https://api.jikan.moe/v4/anime/" + std::to_string(malId)};
                QUrl urlInfo(urlInfoStr.c_str());
                QNetworkRequest requestInfo(urlInfo);
                replyInfo = manager.get(requestInfo);

                QObject::connect(replyInfo, &QNetworkReply::finished, [&]() {
                    if (replyInfo->error() == QNetworkReply::NoError) {
                        QByteArray response = replyInfo->readAll();

                        // Parse JSON
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
                        QJsonObject jsonObj = jsonDoc.object();

                        QJsonObject dataObj = jsonObj["data"].toObject();
                        qDebug() << "Airing:" << dataObj["airing"].toBool();
                        bool airing = dataObj["airing"].toBool();
                        if (!airing) {
                            qDebug() << "Anime not currently airing! Exiting...";
                            replyInfo->deleteLater();
                            QCoreApplication::quit();
                            return;
                        }

                        QJsonObject broadcastObj{dataObj["broadcast"].toObject()};
                        qDebug() << "Time: " << broadcastObj["time"].toString();
                        qDebug() << "Timezone: " << broadcastObj["timezone"].toString();
                    } else {
                        qDebug() << "Second request error:" << replyInfo->errorString();
                    }

                    replyInfo->deleteLater();
                    QCoreApplication::quit();
                });
            } else {
                qDebug() << "Error:" << replySearch->errorString();
                replySearch->deleteLater();
                QCoreApplication::quit();
            }

            replySearch->deleteLater();
        });
    }

    return QCoreApplication::exec();
}