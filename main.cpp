#include "animeNotifier.h"
#include "logging.h"

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    logging::installLogging();
    logging::printSeparator();
    qInfo().noquote() << "Starting Anime Notifier run";

    notifier::AnimeNotifier malNotifier;
    // Use singleShot to avoid endless running bug when config is invalid (put this task in Qt event queue)
    QTimer::singleShot(0, &malNotifier, &notifier::AnimeNotifier::start);

    int exitCode = QCoreApplication::exec();

    qInfo().noquote() << "Anime Notifier run finished";
    logging::printSeparator();
    fprintf(stdout, "\n");
    fflush(stdout);

    return exitCode;
}