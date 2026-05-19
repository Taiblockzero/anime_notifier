#include "broadcastUtils.h"

#include <QNetworkRequest>

namespace broadcastUtils {

QDate nextWeekdayDate(QString targetDay) {
    QDate today = QDate::currentDate();
    // convert weekday to number
    int targetDayNum = weekdayNumFromName(targetDay);

    // create next date
    int daysToAdd = (targetDayNum - today.dayOfWeek() + 7) % 7;

    return today.addDays(daysToAdd);
}

int weekdayNumFromName(const QString &weekdayName) {
    QString weekdayNameLower = weekdayName.toLower();

    static const QMap<QString, int> days = {{"monday", 1}, {"tuesday", 2},  {"wednesday", 3}, {"thursday", 4},
                                            {"friday", 5}, {"saturday", 6}, {"sunday", 7}};

    return days.value(weekdayNameLower);
}

bool isToday(const QDateTime &datetime) { return datetime.date() == QDate::currentDate(); }

} // namespace broadcastUtils