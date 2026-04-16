#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNetworkAccessManager manager;

    QUrl url("https://api.jikan.moe/v4/anime?q=youjo%20senki");
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();

            // Parse JSON
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            QJsonArray dataArr = jsonObj["data"].toArray();
            QJsonObject firstObj = dataArr[0].toObject();
            qDebug() << "MAL ID:" << firstObj["mal_id"].toInt();
        } else {
            qDebug() << "Error:" << reply->errorString();
        }

        reply->deleteLater();

        QCoreApplication::quit();
    });

    return QCoreApplication::exec();
}