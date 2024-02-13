#include "common.h"
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(sync) {
    os << "Sync local database from server repository" << endl;
}

PKGUPD_MODULE(sync) {
    PROCESS("SYNCING REPOSITORY");
    engine->sync(true);
    return 0;
}
