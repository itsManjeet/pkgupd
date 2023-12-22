#include "common.h"

#include <fstream>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(info) {
    os << "Display package information of specified package" << endl;
}

PKGUPD_MODULE(info) {
    CHECK_ARGS(1);
    engine->load_system_database();

    if (auto const installed_meta_info = engine->list_installed().find(args[0]);
            installed_meta_info != engine->list_installed().end()) {
        std::cout << installed_meta_info->second.str() << std::endl;
        return 0;
    }
    ERROR("no package installed with name " + args[0]);
    return 1;
}
