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

PKGUPD_SYSROOT_MODULE_HELP(list) { os << "list all deployments" << std::endl; }

PKGUPD_SYSROOT_MODULE(list) {
    sysroot->reload_deployments();
    for (auto const& i : sysroot->deployments) {
        std::cout << (i.is_active ? "*" : "-") << " " << sysroot->osname << " "
                  << i.channel << '\n'
                  << "  revision  : " << i.revision << '\n'
                  << "  refspec   : " << i.refspec << std::endl;
        if (!i.extensions.empty()) {
            std::cout << "  extensons : " << i.extensions.size() << '\n';
            for (auto const& [id, revision] : i.extensions) {
                std::cout << "   - " << id << " : " << revision << '\n';
            }
        }
        std::cout << std::endl;
    }
    return 0;
}