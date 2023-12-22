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

#include "../../src/json.h"
#include "ignite_common.h"

#include <fstream>

PKGUPD_IGNITE_MODULE_HELP(meta) {
    os << "Generate meta information" << std::endl;
}

PKGUPD_IGNITE_MODULE(meta) {
    CHECK_ARGS(1);

    nlohmann::json data;

    for (auto [path, build_info] : ignite->get_pool()) {
        build_info.cache = ignite->hash(build_info);
        auto cache_file = ignite->cachefile(build_info);
        if (std::filesystem::exists(cache_file)) {
            auto depends = std::vector<std::string>();
            for(std::filesystem::path depend : build_info.depends) {
                depends.push_back(depend.replace_extension());
            }
            data.push_back({
                {"id", std::filesystem::path(path).replace_extension()},
                {"version", build_info.version},
                {"about", build_info.about},
                {"cache", build_info.cache},
                {"depends", depends},
                {"integration", build_info.integration},
                {"backup", build_info.backup},
            });
        }
    }

    std::ofstream writer(args[0]);
    writer << data;
    return 0;
}