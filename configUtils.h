#pragma once

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

#include <QString>

namespace cfg {

class Config {
  public:
    QString pushbulletToken;
    QStringList animeSearches;

    static Config load(QString filename) {
        Config c;

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Can't open config file";
            return c;
        }

        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        c.pushbulletToken = obj["pushbullet_token"].toString();
        // load anime searches
        const QJsonArray arr = obj["anime_searches"].toArray();
        for (const auto &search : arr) {
            c.animeSearches.append(search.toString());
        }

        return c;
    }

    bool save(QString filename);
};
} // namespace cfg