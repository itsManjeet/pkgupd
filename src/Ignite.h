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

#ifndef PKGUPD_IGNITE_H
#define PKGUPD_IGNITE_H

#include "Builder.h"
#include "Container.h"
#include "Engine.h"

class Ignite {
    std::map<std::string, Builder::BuildInfo> pool;
    std::filesystem::path project_path, cache_path;

    Configuration& config;

public:
    using State = std::tuple<std::string, Builder::BuildInfo, bool>;

    explicit Ignite(Configuration& config, std::filesystem::path project_path,
            std::filesystem::path cache_path = {});

    void load();

    [[nodiscard]] std::filesystem::path const& get_cache_path() const {
        return cache_path;
    }

    std::string hash(const Builder::BuildInfo& build_info);

    std::filesystem::path cache_file(const Builder::BuildInfo& build_info);

    [[nodiscard]] std::map<std::string, Builder::BuildInfo> const&
    get_pool() const {
        return pool;
    }

    void resolve(const std::vector<std::string>& id, std::vector<State>& output,
            bool devel = true, bool include_depends = true,
            bool include_extra = true);

    enum class ContainerType {
        Build,
        Shell,
    };

    Container setup_container(const Builder::BuildInfo& build_info,
            Engine* engine,
            ContainerType container_type = ContainerType::Shell);

    void integrate(Container& container, const Builder::BuildInfo& build_info,
            const std::filesystem::path& root = {});

    void build(const Builder::BuildInfo& build_info, Engine* engine);
};

#endif // PKGUPD_IGNITE_H
