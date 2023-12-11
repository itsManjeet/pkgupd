#include "common.h"

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(depends) {
    os << "List all the dependent packages required" << endl
       << PADDING << " " << BOLD("Options:") << endl
       << PADDING << "  - depends.all=" << BOLD("<bool>")
       << "    # List all dependent packages including already installed packages"
       << endl
       << endl;
}

PKGUPD_MODULE(depends) {
    std::vector<MetaInfo> meta_infos;
    PROCESS("loading repository");
    engine->sync(false);

    PROCESS("calculating dependency");
    engine->resolve(args, meta_infos);
    for (auto const &i: meta_infos) {
        cout << i.id << endl;
    }


    return 0;
}
