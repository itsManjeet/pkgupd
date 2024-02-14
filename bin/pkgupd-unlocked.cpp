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

#include "unlocked/unlocked_common.h"
#include <cstring>
#include <functional>

#define PKGUPD_UNLOCKED_MODULES_LIST                                           \
    X(install)                                                                 \
    X(remove)                                                                  \
    X(sync)                                                                    \
    X(info)                                                                    \
    X(search)                                                                  \
    X(update)                                                                  \
    X(depends)                                                                 \
    X(trigger)                                                                 \
    X(owner)                                                                   \
    X(build)                                                                   \
    X(cleanup)                                                                 \
    X(cachefile)                                                               \
    X(autoremove)

#define X(id) PKGUPD_UNLOCKED_MODULE(id);
PKGUPD_UNLOCKED_MODULES_LIST
#undef X

#define X(id) PKGUPD_UNLOCKED_MODULE_HELP(id);
PKGUPD_UNLOCKED_MODULES_LIST
#undef X

std::map<std::string, std::function<int(std::vector<std::string> const&,
                              Engine*, Configuration*)>>
        UNLOCKED_MODULES = {
#define X(id) {#id, PKGUPD_UNLOCKED_##id},
                PKGUPD_UNLOCKED_MODULES_LIST
#undef X
};

PKGUPD_MODULE_HELP(unlocked) {
    os << "System root manager" << std::endl;

    if (padding > 0) return;
    int size = 10;
#define X(id)                                                                  \
    if (strlen(#id) > size) size = std::strlen(#id);
    PKGUPD_UNLOCKED_MODULES_LIST
#undef X

    std::cout << "SUB Task:" << std::endl;
#define X(id)                                                                  \
    std::cout << " - " << BLUE(#id)                                            \
              << std::string(size - std::strlen(#id), ' ') << "    ";          \
    PKGUPD_UNLOCKED_help_##id(std::cout, 2 + size + 4);
    PKGUPD_UNLOCKED_MODULES_LIST
#undef X
}

PKGUPD_MODULE(unlocked) {
    if (args.empty()) {
        PKGUPD_help_unlocked(std::cout, 0);
        return 1;
    }
    auto const& task = args[0];
    auto iter = UNLOCKED_MODULES.find(task);
    if (iter == UNLOCKED_MODULES.end()) {
        PKGUPD_help_unlocked(std::cout, 0);
        return 1;
    }
    auto sub_args = std::vector<std::string>(args.begin() + 1, args.end());

    if (unshare(CLONE_NEWNS) != 0) {
        throw std::runtime_error(
                "failed to setup namespaces: " + std::string(strerror(errno)));
    }
    auto unlocked = Engine(*config);

    return iter->second(sub_args, &unlocked, config);
}