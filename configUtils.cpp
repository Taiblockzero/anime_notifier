#include "configUtils.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace cfg {

bool Config::save(QString filename) {
    QJsonObject obj;

    // Add new members here
    obj["pushbullet_token"] = pushbulletToken_;

    QJsonDocument doc(obj);

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to write config";
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

} // namespace cfg