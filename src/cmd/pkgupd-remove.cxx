#include "../common.hxx"
#include "../configuration.hxx"
#include "../Trigger/Trigger.hxx"
#include "../Uninstaller/Uninstaller.hxx"

using namespace rlxos::libpkgupd;

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

    auto pkg_id = args[0];

    std::vector<std::string> excludedPackages;

    // exclude all system image packages
    config->get("system.packages", excludedPackages);
    if (std::find(excludedPackages.begin(), excludedPackages.end(), pkg_id) !=
        excludedPackages.end()) {
        ERROR("can't remove package from system image");
        return 1;
    }

    auto system_database = SystemDatabase(config);

    auto pkg_info = system_database.get(pkg_id.c_str());
    if (!pkg_info) {
        ERROR("no package with name " << RED(pkg_id)
            << " is installed in the system");
        return 1;
    }

    if (!ask_user("do you want to remove " + pkg_info->id, config)) {
        ERROR("User cancelled the operation");
        return 1;
    }

    PROCESS("removing " << GREEN(pkg_info->id) << ":"
        << BLUE(pkg_info->version));
    auto uninstaller = std::make_shared<Uninstaller>(config);
    uninstaller->uninstall(*pkg_info, &system_database);

    if (!config->get(SKIP_TRIGGERS, false)) {
        auto triggerer = std::make_shared<Triggerer>();
        std::vector<std::string> files;
        system_database.get_files(*pkg_info, files);
        if (!triggerer->trigger(
            std::vector<
                std::pair<InstalledMetaInfo, std::vector<std::string>>>{
                {*pkg_info, files}
            })) {
            ERROR("failed to execute post removal triggers '" << triggerer->error());
            return 1;
        }
    }

    return 0;
}
