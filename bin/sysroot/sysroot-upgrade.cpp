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

#include "../../src/SysUpgrade.h"
#include "sysroot_common.h"

PKGUPD_SYSROOT_MODULE_HELP(upgrade) { os << "Upgrade sysroot" << std::endl; }

PKGUPD_SYSROOT_MODULE(upgrade) {
    auto upgrader = SysUpgrade(sysroot);

    PROCESS("Checking for updates")
    auto deployment = std::find_if(sysroot->deployments.begin(),
            sysroot->deployments.end(),
            [](const Deployment& deployment) -> bool {
                return deployment.is_active;
            });
    ;
    if (deployment == sysroot->deployments.end()) {
        ERROR("no booted deployment found");
        return 1;
    }

    for (auto const& i : config->node["include"]) {
        if (std::find_if(deployment->extensions.begin(),
                    deployment->extensions.end(), [i](auto const& id) -> bool {
                        return id.first == i.as<std::string>();
                    }) == deployment->extensions.end())
            deployment->extensions.emplace_back(i.as<std::string>(), "");
    }

    deployment->channel =
            config->get<std::string>("channel", deployment->channel);

    DEBUG("CHANNEL    : " << deployment->channel);
    DEBUG("EXTENSIONS : " << deployment->extensions.size());
    auto updateInfo =
            upgrader.update(*deployment, config->get<std::string>("server", ""),
                    config->get("dry-run", false));
    if (updateInfo) {
        if (config->get("dry-run", false)) {
            INFO("New updates available");
            std::cout << updateInfo->changelog << std::endl;
        }
    } else {
        INFO("No updates available");
        return 1;
    }

    return 0;
}