#include "persistentNotificationHistory.h"

namespace cfg {

void persistentNotificationHistory::setLatestDate(int malId, QDate date) {
    settings_.setValue(SETTING_PREFIX + QString::number(malId), date);
}

QDate persistentNotificationHistory::getLatestDate(int malId) {
    QVariant value = settings_.value(SETTING_PREFIX + QString::number(malId));
    if (!value.isValid())
        return QDate();

    return value.toDate();
}
} // namespace cfg