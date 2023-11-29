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
    for (auto const& [id, meta_info]: repository.get()) {
        cout << GREEN(meta_info.id) << ": " << BLUE(meta_info.version)
                << "\n  " << BOLD(meta_info.about) << '\n'
                << endl;
        found++;
    }

    if (found == 0) {
        ERROR("no package found with name '" << package_id << "'");
    }

    return 0;
}
