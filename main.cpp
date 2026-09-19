#include "animeNotifier.h"
#include "logging.h"

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    logging::installLogging();
    logging::printSeparator();
    qInfo().noquote() << "Starting Anime Notifier run";

    notifier::AnimeNotifier malNotifier;
    malNotifier.start();

    int exitCode = QCoreApplication::exec();

    qInfo().noquote() << "Anime Notifier run finished";
    logging::printSeparator();
    fprintf(stdout, "\n");
    fflush(stdout);

    return exitCode;
}