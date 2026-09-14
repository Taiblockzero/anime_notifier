#pragma once

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QString>

namespace cfg {

struct ConfigUser {
    QString username_;
    QString pushbulletToken_;
    QStringList animeSearches_;
};

class Config {
  public:
    std::vector<ConfigUser> users;

    static Config load(QString filename) {
        Config c;

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Can't open config file, looking in '" << filename << "'";
            return c;
        }

        // parse config JSON
        QJsonObject rootObj = QJsonDocument::fromJson(file.readAll()).object();
        QJsonArray usersArr = rootObj.value("users").toArray();
        for (const auto &userVal : std::as_const(usersArr)) {
            ConfigUser tempUser;
            QJsonObject userObj = userVal.toObject();

            tempUser.username_ = userObj["username"].toString();
            tempUser.pushbulletToken_ = userObj["pushbullet_token"].toString();
            const QJsonArray searchesArr = userObj["anime_searches"].toArray();
            for (const auto &search : searchesArr) {
                tempUser.animeSearches_.append(search.toString());
            }

            c.users.push_back(tempUser);
        }

        return c;
    }
};
} // namespace cfg