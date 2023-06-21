#include "../common.hh"
#include "../configuration.hh"
#include "../system-database.hh"
#include "../triggerer.hh"
#include "../uninstaller/uninstaller.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(autoremove) {
    os << "Cleanup unneeded packages from system" << std::endl;
}

PKGUPD_MODULE(autoremove) {
    CHECK_ARGS(0);
    auto system_database = SystemDatabase(config);

    auto is_required = [&](std::string id) -> bool {
        for (auto const& i : system_database.get()) {
            if (std::find(i.second->depends().begin(), i.second->depends().end(),
                          id) != i.second->depends().end()) {
                return true;
            }
        }
        return false;
    };

    PROCESS("checking for unneeded packages");
    std::vector<std::pair<std::shared_ptr<InstalledPackageInfo>, std::vector<std::string>>>
        packages_to_remove;
    for (auto const& i : system_database.get()) {
        if (!i.second->isDependency()) {
            continue;
        }
        if (!is_required(i.first)) {
            std::vector<std::string> files;
            system_database.get_files(i.second, files);
            packages_to_remove.push_back({i.second, files});
        }
    }

    if (packages_to_remove.size() == 0) {
        INFO("System is already cleaned");
        return 0;
    }

    INFO("Found " << BLUE(to_string(packages_to_remove.size()))
                  << " unneeded packages");
    for (auto const& i : packages_to_remove) {
        std::cout << "- " << i.first->id() << std::endl;
    }

    if (!ask_user("You you want to remove the above unneeded packages", config)) {
        ERROR("User cancelled the operation");
        return 1;
    }

    auto uninstaller = Uninstaller(config);
    for (auto const& i : packages_to_remove) {
        PROCESS("uninstalling " << BLUE(i.first->id()));
        if (!uninstaller.uninstall(i.first, &system_database)) {
            ERROR("failed to remove " << i.first->id() << ", "
                                      << uninstaller.error());
        }
    }

    auto trigger = Triggerer();
    if (!trigger.trigger(packages_to_remove)) {
        ERROR("triggering failed " << trigger.error());
        return 1;
    }

    return 0;
}