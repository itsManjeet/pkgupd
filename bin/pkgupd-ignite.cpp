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

#include "ignite/ignite_common.h"

#include <functional>
#include <cstring>

#define PKGUPD_IGNITE_MODULES_LIST \
    X(status)                      \
    X(build)

#define X(id) PKGUPD_IGNITE_MODULE(id);
PKGUPD_IGNITE_MODULES_LIST
#undef X

#define X(id) PKGUPD_IGNITE_MODULE_HELP(id);
PKGUPD_IGNITE_MODULES_LIST
#undef X

std::map<std::string, std::function<int(std::vector<std::string> const &, Engine *, Ignite *, Configuration *)>>
        IGNITE_MODULES = {
#define X(id) {#id, PKGUPD_IGNITE_##id},
        PKGUPD_IGNITE_MODULES_LIST
#undef X
};


PKGUPD_MODULE_HELP(ignite) {
    os << "Project Build tool" << std::endl;

    int size = 10;
#define X(id) \
  if (strlen(#id) > size) size = std::strlen(#id);
    PKGUPD_IGNITE_MODULES_LIST
#undef X

    std::cout << "Task:" << std::endl;
#define X(id)                                                        \
  std::cout << " - " << BLUE(#id) << std::string(size - std::strlen(#id), ' ') \
       << "    ";                                                    \
  PKGUPD_IGNITE_help_##id(std::cout, 2 + size + 4);
    PKGUPD_IGNITE_MODULES_LIST
#undef X
}

PKGUPD_MODULE(ignite) {
    if (args.empty()) {
        PKGUPD_help_ignite(std::cout, 0);
        return 1;
    }
    auto const &task = args[0];
    auto iter = IGNITE_MODULES.find(task);
    if (iter == IGNITE_MODULES.end()) {
        PKGUPD_help_ignite(std::cout, 0);
        return 1;
    }
    auto sub_args = std::vector<std::string>(args.begin() + 1, args.end());

    auto ignite = Ignite(*config, config->get<std::string>("ignite.source", std::filesystem::current_path()),
                         config->get<std::string>("ignite.cache", ""));
    ignite.load();

    return iter->second(sub_args, engine, &ignite, config);
}