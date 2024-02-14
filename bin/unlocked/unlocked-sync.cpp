#include "unlocked_common.h"
#include <iostream>

using namespace std;

PKGUPD_UNLOCKED_MODULE_HELP(sync) {
    os << "Sync local database from server repository" << endl;
}

PKGUPD_UNLOCKED_MODULE(sync) {
    PROCESS("SYNCING REPOSITORY");
    engine->sync(true);
    return 0;
}
