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

#include "ignite_common.h"

PKGUPD_IGNITE_MODULE_HELP(build) {
    os << "Build the element into container" << std::endl;
}


PKGUPD_IGNITE_MODULE(build) {
    CHECK_ARGS(1);

    std::vector<std::tuple<std::string, Builder::BuildInfo, bool>> status;
    ignite->resolve({args[0]}, status);

    for (auto &[path, build_info, cached]: status) {
        if (!cached) {
            PROCESS("Building " << build_info.id)
            build_info.resolve(*config);
            ignite->build(build_info, engine);
        }
    }

    return 0;
}