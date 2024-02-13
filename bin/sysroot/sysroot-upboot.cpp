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

PKGUPD_SYSROOT_MODULE_HELP(upboot) {
    os << "list all deployments" << std::endl;
}

PKGUPD_SYSROOT_MODULE(upboot) {
    sysroot->reload_deployments();
    for (auto const& i : sysroot->deployments()) {
        std::cout << (i.is_active() ? "*" : "-") << " " << sysroot->osname()
                  << "  version  : " << i.version() << '\n'
                  << "  deployed : " << (i.is_deployed() ? "YES" : "NO")
                  << '\n';
        if (!i.is_deployed()) { sysroot->deploy(i); }
    }
    PROCESS("Updating bootloader configuration");
    sysroot->generate_boot_config({});
    return 0;
}