/*
 * Copyright (c) 2023 Manjeet Singh <itsmanjeet1998@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "unlocked_common.h"
#include <fstream>

PKGUPD_UNLOCKED_MODULE(install);

PKGUPD_UNLOCKED_MODULE_HELP(build) {
    os << "Build the PKGUPD component from element file" << std::endl;
}

PKGUPD_UNLOCKED_MODULE(build) {
    CHECK_ARGS(1);

    auto c = config->get<std::string>("builder.config", "");
    if (!c.empty()) {
        PROCESS("Using builder configuration " << c);
        config->update_from_file(c);
        DEBUG("CONFIGURATION: " << config->node);
    }

    engine->load_system_database();
    engine->sync();

    auto build_info = Builder::BuildInfo(args[0]);
    if (config->get<bool>("build.depends", true)) {
        std::vector<std::string> to_install = build_info.depends;
        to_install.insert(to_install.begin(),
                build_info.build_time_depends.begin(),
                build_info.build_time_depends.end());
        if (!config->node["installer.depends"]) {
            config->set("installer.depends", true);
        }
        for (auto i = to_install.begin(); i != to_install.end();) {
            if (engine->list_installed().find(*i) !=
                    engine->list_installed().end()) {
                i = to_install.erase(i);
            } else {
                i++;
            }
        }
        if (PKGUPD_UNLOCKED_install(to_install, engine, config) != 0) {
            ERROR("failed to install build dependencies");
            return 1;
        }
    }

    build_info.resolve(*config);
    build_info.cache = engine->hash(build_info);

    PROCESS("BUILDING " << build_info.id);
    auto package_path = engine->build(build_info);

    MESSAGE("READY", "your package is ready at " << package_path);
    return 0;
}
