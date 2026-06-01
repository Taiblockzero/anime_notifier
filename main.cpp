#include "animeNotifier.h"

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    notifier::AnimeNotifier malNotifier;
    malNotifier.start();

    return QCoreApplication::exec();
}