#include "../common.hxx"
#include "../Installer/installer.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"
#include "../utils/utils.hxx"

using namespace rlxos::libpkgupd;

#include <filesystem>

namespace fs = std::filesystem;

#include <iostream>

using namespace std;

PKGUPD_MODULE(sync);

PKGUPD_MODULE_HELP(update) {
    os << "Update non-system packages of system" << endl
            << PADDING << " " << BOLD("Options:") << endl
            << PADDING << "  - system.packages=" << BOLD("<list>")
            << "    # Skip all system packages" << endl
            << PADDING << "  - update.exclude=" << BOLD("<list>")
            << "    # Specify package to exclude from update" << endl
            << endl;
}

PKGUPD_MODULE(update) {
    std::shared_ptr<SystemDatabase> system_database =
            std::make_shared<SystemDatabase>(config);

    std::shared_ptr<Repository> repository = std::make_shared<Repository>(config);
    std::shared_ptr<Installer> installer = std::make_shared<Installer>(config);

    std::vector<std::string> packages_id;
    std::vector<MetaInfo> outdated_packages;
    std::vector<std::string> excludedPackages;

    // exclude all system image packages
    config->get("system.packages", excludedPackages);
    config->get("update.exclude", excludedPackages);

    if (int status = PKGUPD_sync({}, config); status != 0) {
        return status;
    }

    repository->init();

    PROCESS("checking system updates");
    for (auto const& [id, installed_meta_info]: system_database->get()) {
        if (std::find(excludedPackages.begin(), excludedPackages.end(), id) !=
            excludedPackages.end()) {
            continue;
        }

        auto repository_info = repository->get(id);
        if (!repository_info) {
            INFO("missing repository information for " << id << ", skipping");
            continue;
        }
        if (installed_meta_info.cache != repository_info->cache) {
            INFO(("updates for ") << installed_meta_info.id << " "
                << installed_meta_info.version << ":" << installed_meta_info.cache << "->"
                << repository_info->version << ":" << repository_info->cache);
            outdated_packages.push_back(*repository_info);
        }
    }

    if (outdated_packages.size() == 0) {
        cout << BOLD("system is upto date") << endl;
        return 0;
    }

    INFO("found " << outdated_packages.size() << " update(s)");
    if (!ask_user("Do you want to continue", config)) {
        ERROR("user cancelled the operation");
        return 1;
    }

    config->node()["force"] = true;
    config->node()["mode.ask"] = false;
    config->node()["is-updating"] = true;
    installer->install(outdated_packages, repository.get(),
                       system_database.get());

    cout << BOLD("successfully") << " " << BLUE("updated") << " "
            << GREEN(outdated_packages.size()) << BOLD(" package(s)") << endl;

    return 0;
}
