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
#include "../../src/Executor.h"

#include <fstream>

PKGUPD_IGNITE_MODULE_HELP(meta) {
    os << "Generate meta information" << std::endl;
}

PKGUPD_IGNITE_MODULE(meta) {
    CHECK_ARGS(1);

    nlohmann::json data;

    auto target_path = std::filesystem::path(args[0]).parent_path();
    std::filesystem::remove_all(target_path / "apps");
    std::filesystem::create_directories(target_path / "apps");

    for (auto [path, build_info] : ignite->get_pool()) {
        build_info.cache = ignite->hash(build_info);
        auto cache_file = ignite->cachefile(build_info);
        if (std::filesystem::exists(cache_file)) {
            auto depends = std::vector<std::string>();
            for (std::filesystem::path depend : build_info.depends) {
                depends.push_back(depend.replace_extension());
            }

            auto type = build_info.config.get<std::string>("type", "component");

            data.push_back({
                {"id", std::filesystem::path(path).replace_extension()},
                {"version", build_info.version},
                {"about", build_info.about},
                {"cache", build_info.cache},
                {"depends", depends},
                {"type", type},
                {"integration", build_info.integration},
                {"backup", build_info.backup},
            });
            if (type == "app") {
                PROCESS("Adding App " << data.back()["id"].get<std::string>());
                Executor("/bin/tar")
                    .arg("-xf")
                    .arg(cache_file)
                    .arg("-C")
                    .arg(target_path / "apps")
                    .execute();
            }
        }
    }

    std::ofstream writer(args[0]);
    writer << data;
    return 0;
}