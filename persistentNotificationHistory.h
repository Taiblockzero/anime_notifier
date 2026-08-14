#pragma once

#include <QDate>
#include <qsettings.h>
#include <utility>
#include <vector>

namespace cfg {
class persistentNotificationHistory {
  public:
    persistentNotificationHistory() : settings_("Taiblockzero", "WeeklyAnimeNotifier") {}

    void setLatestDate(int malId, QDate date);

    // Get on which date a notification was last sent for a specific anime
    // Returns -1 if no notifications were yet sent
    QDate getLatestDate(int malId);

  private:
    QSettings settings_;
    const QString SETTING_PREFIX = "lastNotificationDate/";
};
} // namespace cfg
