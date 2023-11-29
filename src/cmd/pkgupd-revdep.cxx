#include "../configuration.hxx"
#include "../repository.hxx"
#include "../resolver.hxx"
#include "../system-database.hxx"

using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(revdep) {
    os << "List the reverse dependency of specified package" << std::endl;
}

PKGUPD_MODULE(revdep) {
    CHECK_ARGS(1);
    auto repository = Repository(config);
    auto package = repository.get(args[0].c_str());
    if (!package) {
        ERROR("missing required package " << args[0]);
        return 1;
    }
    for (auto const& [id, meta_info]: repository.get()) {
        if (find_if(meta_info.depends.begin(), meta_info.depends.end(),
                    [&](std::string id) -> bool { return id == package->id; }) !=
            meta_info.depends.end()) {
            std::cout << meta_info.id << std::endl;
        }
    }


    return 0;
}
