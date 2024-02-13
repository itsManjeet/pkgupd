#include "common.h"
#include <algorithm>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(remove) {
    os << "Remove package from system" << endl
       << PADDING << " " << BOLD("Options:") << endl
       << PADDING << "  - system.packages=" << BOLD("<list>")
       << "    # Skip System packages for removal" << endl
       << endl;
}

PKGUPD_MODULE(remove) {
    CHECK_ARGS(1);
    engine->load_system_database();

    auto const iter = engine->list_installed().find(args[0]);
    if (iter == engine->list_installed().end()) {
        ERROR("no component installed with id " << args[0]);
        return 1;
    }
    auto installed_meta_info = iter->second;
    if (!ask_user("do you want to remove " + installed_meta_info.id, config)) {
        ERROR("User cancelled the operation");
        return 1;
    }

    engine->uninstall(installed_meta_info);

    engine->triggers({installed_meta_info});

    return 0;
}
