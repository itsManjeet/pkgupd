#include "common.h"
#include <algorithm>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(search) {
    os << "Search package from repository" << endl;
}

PKGUPD_MODULE(search) {
    CHECK_ARGS(1);

    engine->sync(false);
    int found = 0;
    for (auto const& [_, meta_info] : engine->list_remote()) {
        if (meta_info.id.contains(args[0]) ||
                meta_info.about.contains(args[0])) {
            cout << GREEN(meta_info.id) << ": " << BLUE(meta_info.version)
                 << "\n  " << BOLD(meta_info.about) << '\n'
                 << endl;
            found++;
        }
    }

    if (found == 0) {
        ERROR("no component found with name '" << args[0] << "'");
    }

    return 0;
}
