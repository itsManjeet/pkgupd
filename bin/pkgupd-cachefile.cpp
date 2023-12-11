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

PKGUPD_MODULE_HELP(cachefile) {
    os << "print the cache file name" << std::endl;
}

PKGUPD_MODULE(cachefile) {
    CHECK_ARGS(1);

    engine->sync();

    std::ifstream reader(args[0]);
    if (!reader.good()) {
        throw std::runtime_error("failed to read build build_info");
    }

    std::string content(
            (std::istreambuf_iterator<char>(reader)),
            (std::istreambuf_iterator<char>())
    );

    std::cout << engine->cache_file(Builder::BuildInfo(content)).string() << std::endl;
    return 0;
}