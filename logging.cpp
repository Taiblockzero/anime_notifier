#include "logging.h"

#include <QDateTime>
#include <QDebug>
#include <cstdio>

namespace logging {

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(context);

    QString level;

    switch (type) {
    case QtDebugMsg:
        level = "DEBUG";
        break;

    case QtInfoMsg:
        level = "INFO";
        break;

    case QtWarningMsg:
        level = "WARN";
        break;

    case QtCriticalMsg:
        level = "CRITICAL";
        break;

    case QtFatalMsg:
        level = "FATAL";
        break;
    }

    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");

    // make sure qDebug goes to stdout, qCritical goes to stderr and so on
    FILE *stream = (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) ? stderr : stdout;

    fprintf(stream, "[%s] [%s] %s\n", timestamp.toLocal8Bit().constData(), level.toLocal8Bit().constData(),
            msg.toLocal8Bit().constData());

    fflush(stream);
}

void installLogging() { qInstallMessageHandler(messageHandler); }

} // namespace logging