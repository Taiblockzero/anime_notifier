#include "logging.h"

#include <QDateTime>
#include <QDebug>
#include <cstdio>
#include <cstring>

namespace logging {

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    const char *level = "DEBUG";
    FILE *stream = stdout;

    switch (type) {
    case QtDebugMsg:
        level = "DEBUG";
        break;

    case QtInfoMsg:
        level = "INFO ";
        break;

    case QtWarningMsg:
        level = "WARN ";
        stream = stderr;
        break;

    case QtCriticalMsg:
        level = "CRIT ";
        stream = stderr;
        break;

    case QtFatalMsg:
        level = "FATAL";
        stream = stderr;
        break;
    }

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // Extract file basename if available
    const char *fileName = nullptr;
    if (context.file) {
        fileName = strrchr(context.file, '/');
        if (!fileName) {
            fileName = strrchr(context.file, '\\');
        }
        fileName = fileName ? fileName + 1 : context.file;
    }

    if ((type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) && fileName && fileName[0] != '\0') {
        fprintf(stream, "[%s] [%s] %s (%s:%u)\n",
                timestamp.toLocal8Bit().constData(), level,
                msg.toLocal8Bit().constData(), fileName, context.line);
    } else {
        fprintf(stream, "[%s] [%s] %s\n",
                timestamp.toLocal8Bit().constData(), level,
                msg.toLocal8Bit().constData());
    }

    fflush(stream);
}

void installLogging() {
    qInstallMessageHandler(messageHandler);
}

} // namespace logging