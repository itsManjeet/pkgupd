#include "common.h"


#include <algorithm>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP (autoremove) {
    os << "Cleanup unneeded packages from system" << std::endl;
}

PKGUPD_MODULE (autoremove) {
    CHECK_ARGS(0);

    auto is_required = [&](std::string id) -> bool {
        for (auto const &[id, info]: engine->list_installed()) {
            if (std::find(info.depends.begin(), info.depends.end(),
                          id) != info.depends.end()) {
                return true;
            }
        }
        return false;
    };

    PROCESS("checking for unneeded packages");
    std::vector<InstalledMetaInfo> installed_meta_infos;
    for (auto const &[id, installed_meta_info]: engine->list_installed()) {
        if (!installed_meta_info.dependency) {
            continue;
        }
        if (!is_required(id)) {
            installed_meta_infos.push_back(installed_meta_info);
        }
    }

    if (installed_meta_infos.size() == 0) {
        INFO("System is already cleaned");
        return 0;
    }

    INFO("Found " << BLUE(to_string(installed_meta_infos.size()))
                  << " unneeded packages");
    for (auto const &installed_meta_info: installed_meta_infos) {
        std::cout << "- " << installed_meta_info.id << std::endl;
    }

    if (!ask_user("You you want to remove the above unneeded packages", config)) {
        ERROR("User cancelled the operation");
        return 1;
    }
    for (auto const &installed_meta_info: installed_meta_infos) {
        PROCESS("uninstalling " << BLUE(installed_meta_info.id));
        engine->uninstall(installed_meta_info);
    }

    PROCESS("executing triggers")
    engine->triggers(installed_meta_infos);

    return 0;
}
