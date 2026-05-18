#include "broadcastUtils.h"

bool BroadcastTime::changeTimezone(QString timezone) {
    if (timezone == this->timezone_) {
        qDebug() << "Same timezone, no timezone change needed";
        return false;
    }

    QString weekdaySingular = this->weekday_.chopped(1); // remove last 's' to make it singlular

    timezone_ = timezone;
}
