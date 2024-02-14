#include "unlocked_common.h"
#include <iostream>

using namespace std;

PKGUPD_UNLOCKED_MODULE_HELP(trigger) {
    os << "Execute required triggers and create required users & groups"
       << endl;
}

PKGUPD_UNLOCKED_MODULE(trigger) {
    engine->triggers();
    return 0;
}
