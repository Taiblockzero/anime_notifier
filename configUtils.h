#pragma once

#include <QJsonObject>

#include <QString>

namespace config {

QJsonObject initialize(const QString &configFile = "config.json");

}