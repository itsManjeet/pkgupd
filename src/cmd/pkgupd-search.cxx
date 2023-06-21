#include "../repository.hxx"
#include "../system-database.hxx"

using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(search) { os << "Search package from repository" << endl; }

PKGUPD_MODULE(search) {
    CHECK_ARGS(1);

    auto package_id = args[0];
    auto repository = Repository(config);

    int found = 0;
    for (auto const &p: repository.get()) {
        if (p.first.find(package_id) != string::npos) {
            found++;
            cout << GREEN(p.second->id()) << ": " << BLUE(p.second->version())
                 << "\n  " << BOLD(p.second->about()) << '\n'
                 << endl;
        }
    }

    if (found == 0) {
        ERROR("no package found with name '" << package_id << "'");
    }

    return 0;
}