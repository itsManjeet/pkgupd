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

#include "common.h"

#include <fstream>

PKGUPD_MODULE(install);

PKGUPD_MODULE_HELP(build) {
    os << "Build the PKGUPD component from element file" << std::endl;
}

PKGUPD_MODULE(build) {
    CHECK_ARGS(1);
    std::string build_info_content;
    {
        std::ifstream reader(args[0]);

        build_info_content = std::string(
                (std::istreambuf_iterator<char>(reader)),
                (std::istreambuf_iterator<char>())
        );
    }

    auto c = config->get<std::string>("builder.config", "");
    if (!c.empty()) {
        std::ifstream reader(c);
        if (!reader.good()) throw std::runtime_error("failed to read configuration file '" + c + "'");
        config->update_from(std::string(
                (std::istreambuf_iterator<char>(reader)),
                (std::istreambuf_iterator<char>())
        ));
    }

    auto build_info = Builder::BuildInfo(build_info_content);
    if (config->get<bool>("build.depends", true)) {
        std::vector<std::string> to_install = build_info.depends;
        to_install.insert(to_install.begin(), build_info.build_time_depends.begin(),
                          build_info.build_time_depends.end());
        if (!config->node["installer.depends"]) {
            config->set("installer.depends", true);
        }
        if (PKGUPD_install(to_install, engine, config) != 0) {
            ERROR("failed to install build dependencies");
            return 1;
        }
    }


    auto package_path = engine->build(build_info);

    MESSAGE("READY", "your package is ready at " << package_path);
    return 0;
}
