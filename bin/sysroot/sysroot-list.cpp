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

#include "sysroot_common.h"

static std::string truncate(const std::string& str, int size = 6) {
    return str.size() > size ? str.substr(0, size) : str;
}

PKGUPD_SYSROOT_MODULE_HELP(list) { os << "list all deployments" << std::endl; }

PKGUPD_SYSROOT_MODULE(list) {
    sysroot->reload_deployments();
    for (auto const& i : sysroot->deployments) {
        std::cout << (i.is_active ? "*" : "-") << " " << sysroot->osname
                  << " : " << truncate(i.revision) << '\n'
                  << "  channel   : " << i.channel << '\n'
                  << "  revision  : " << truncate(i.base_revision) << '\n'
                  << "  refspec   : " << i.refspec << std::endl;
        if (!i.extensions.empty()) {
            std::cout << "  extensons : " << i.extensions.size() << " ";
            for (auto const& [id, revision] : i.extensions) {
                std::cout << "[" << id << "(" << truncate(revision) << ")] ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}