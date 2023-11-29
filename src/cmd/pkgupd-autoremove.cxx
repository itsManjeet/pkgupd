#include "../common.hxx"
#include "../configuration.hxx"
#include "../system-database.hxx"
#include "../Trigger/Trigger.hxx"
#include "../Uninstaller/Uninstaller.hxx"

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
        for (auto const& [id, info]: system_database.get()) {
            if (std::find(info.depends.begin(), info.depends.end(),
                          id) != info.depends.end()) {
                return true;
            }
        }
        return false;
    };

    PROCESS("checking for unneeded packages");
    std::vector<std::pair<InstalledMetaInfo, std::vector<std::string>>>
            packages_to_remove;
    for (auto const& [id, installed_meta_info]: system_database.get()) {
        if (!installed_meta_info.dependency) {
            continue;
        }
        if (!is_required(id)) {
            std::vector<std::string> files;
            system_database.get_files(installed_meta_info, files);
            packages_to_remove.push_back({installed_meta_info, files});
        }
    }

    if (packages_to_remove.size() == 0) {
        INFO("System is already cleaned");
        return 0;
    }

    INFO("Found " << BLUE(to_string(packages_to_remove.size()))
        << " unneeded packages");
    for (auto const& [installed_meta_info, files]: packages_to_remove) {
        std::cout << "- " << installed_meta_info.id << std::endl;
    }

    if (!ask_user("You you want to remove the above unneeded packages", config)) {
        ERROR("User cancelled the operation");
        return 1;
    }

    auto uninstaller = Uninstaller(config);
    for (auto const& [installed_meta_info, files]: packages_to_remove) {
        PROCESS("uninstalling " << BLUE(installed_meta_info.id));
        try {
            uninstaller.uninstall(installed_meta_info, &system_database);
        } catch (const std::exception& exception) {
            ERROR("failed to remove " << installed_meta_info.id << ", "
                << exception.what());
        }
    }

    auto trigger = Triggerer();
    if (!trigger.trigger(packages_to_remove)) {
        ERROR("triggering failed " << trigger.error());
        return 1;
    }

    return 0;
}
