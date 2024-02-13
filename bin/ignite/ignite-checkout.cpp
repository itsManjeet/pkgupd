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

#include "../../src/ArchiveManager.h"
#include "ignite_common.h"

PKGUPD_IGNITE_MODULE_HELP(checkout) {
    os << "Checkout built cache" << std::endl;
}

PKGUPD_IGNITE_MODULE(checkout) {
    CHECK_ARGS(2);

    std::filesystem::create_directories(args[1]);

    std::vector<std::tuple<std::string, Builder::BuildInfo, bool>> status;
    ignite->resolve({args[0]}, status);

    auto [path, build_info, cached] = status.back();
    if (!cached) {
        ERROR(path << " not cached yet");
        return 1;
    }

    std::vector<std::string> files;
    ArchiveManager::extract(ignite->cachefile(build_info), args[1], files);

    return 0;
}