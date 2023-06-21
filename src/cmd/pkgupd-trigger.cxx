#include "../triggerer.hxx"

using namespace rlxos::libpkgupd;

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(trigger) {
    os << "Execute required triggers and create required users & groups" << endl;
}

PKGUPD_MODULE(trigger) {
    auto triggerer = std::make_shared<Triggerer>();
    auto system_database = std::make_shared<SystemDatabase>(config);
    std::vector<std::pair<std::shared_ptr<InstalledPackageInfo>, std::vector<std::string>>>
            packages;
    for (auto const &i: system_database->get()) {
        std::vector<std::string> files;
        system_database->get_files(i.second, files);
        packages.push_back({i.second, files});
    }
    if (!triggerer->trigger(packages)) {
        cerr << "Error! " << triggerer->error() << endl;
    }
    return 0;
}