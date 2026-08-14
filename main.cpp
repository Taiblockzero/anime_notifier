#include "animeNotifier.h"
#include "logging.h"

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    logging::installLogging();

    notifier::AnimeNotifier malNotifier;
    malNotifier.start();

    return QCoreApplication::exec();
}