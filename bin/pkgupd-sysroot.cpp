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

#include "sysroot/sysroot_common.h"
#include <cstring>
#include <functional>

#define PKGUPD_SYSROOT_MODULES_LIST                                            \
    X(list)                                                                    \
    X(upboot)

#define X(id) PKGUPD_SYSROOT_MODULE(id);
PKGUPD_SYSROOT_MODULES_LIST
#undef X

#define X(id) PKGUPD_SYSROOT_MODULE_HELP(id);
PKGUPD_SYSROOT_MODULES_LIST
#undef X

std::map<std::string, std::function<int(std::vector<std::string> const&,
                              Engine*, Sysroot*, Configuration*)>>
        SYSROOT_MODULES = {
#define X(id) {#id, PKGUPD_SYSROOT_##id},
                PKGUPD_SYSROOT_MODULES_LIST
#undef X
};

PKGUPD_MODULE_HELP(sysroot) {
    os << "System root manager" << std::endl;

    if (padding > 0) return;
    int size = 10;
#define X(id)                                                                  \
    if (strlen(#id) > size) size = std::strlen(#id);
    PKGUPD_SYSROOT_MODULES_LIST
#undef X

    std::cout << "SUB Task:" << std::endl;
#define X(id)                                                                  \
    std::cout << " - " << BLUE(#id)                                            \
              << std::string(size - std::strlen(#id), ' ') << "    ";          \
    PKGUPD_SYSROOT_help_##id(std::cout, 2 + size + 4);
    PKGUPD_SYSROOT_MODULES_LIST
#undef X
}

PKGUPD_MODULE(sysroot) {
    if (args.empty()) {
        PKGUPD_help_sysroot(std::cout, 0);
        return 1;
    }
    auto const& task = args[0];
    auto iter = SYSROOT_MODULES.find(task);
    if (iter == SYSROOT_MODULES.end()) {
        PKGUPD_help_sysroot(std::cout, 0);
        return 1;
    }
    auto sub_args = std::vector<std::string>(args.begin() + 1, args.end());

    auto sysroot = Sysroot(config->get<std::string>("osname", "rlxos"),
            config->get<std::string>("sysroot", "/"));

    return iter->second(sub_args, engine, &sysroot, config);
}