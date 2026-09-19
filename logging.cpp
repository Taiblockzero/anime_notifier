#include "logging.h"

#include <QDateTime>
#include <QDebug>
#include <cstdio>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

namespace logging {

#ifdef Q_OS_WIN
static void enableWindowsAnsi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    if (hErr != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hErr, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hErr, dwMode);
        }
    }
}
#endif

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    const char *color = "";
    const char *reset = "\033[0m";
    const char *level = "DEBUG";
    FILE *stream = stdout;

    switch (type) {
    case QtDebugMsg:
        color = "\033[36m"; // Cyan
        level = "DEBUG";
        break;

    case QtInfoMsg:
        color = "\033[32m"; // Green
        level = "INFO ";
        break;

    case QtWarningMsg:
        color = "\033[33m"; // Yellow
        level = "WARN ";
        stream = stderr;
        break;

    case QtCriticalMsg:
        color = "\033[31;1m"; // Bold Red
        level = "CRIT ";
        stream = stderr;
        break;

    case QtFatalMsg:
        color = "\033[35;1m"; // Bold Magenta
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
        fprintf(stream, "%s[%s] [%s]%s %s \033[90m(%s:%u)\033[0m\n",
                color, timestamp.toLocal8Bit().constData(), level, reset,
                msg.toLocal8Bit().constData(), fileName, context.line);
    } else {
        fprintf(stream, "%s[%s] [%s]%s %s\n",
                color, timestamp.toLocal8Bit().constData(), level, reset,
                msg.toLocal8Bit().constData());
    }

    fflush(stream);
}

void installLogging() {
#ifdef Q_OS_WIN
    enableWindowsAnsi();
#endif
    qInstallMessageHandler(messageHandler);
}

} // namespace logging