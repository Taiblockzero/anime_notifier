#include "configUtils.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace config {

QJsonObject initialize(const QString &configFile) {
    QFile file(configFile);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open" << configFile;
        return {};
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qWarning() << "Invalid config format";
        return {};
    }

    return doc.object();
}

} // namespace config