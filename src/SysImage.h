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

#ifndef PKGUPD_SYSIMAGE_H
#define PKGUPD_SYSIMAGE_H

#include <filesystem>
#include <map>
#include <utility>
#include <vector>

class SysImage {
public:
    enum class Type {
        Image,
        Unknown,
    };

private:
    Type type_{Type::Unknown};
    std::filesystem::path path_;
    int version_;
    bool is_active_{false};
    bool is_deployed_{false};
    std::map<std::string, std::string> release_info_;

public:
    SysImage(Type type, std::filesystem::path path, bool is_deployed = false,
            bool is_active = false);

    [[nodiscard]] Type type() const { return type_; }

    [[nodiscard]] std::filesystem::path const& path() const { return path_; }

    [[nodiscard]] int version() const { return version_; }

    [[nodiscard]] bool is_active() const { return is_active_; }

    [[nodiscard]] bool is_deployed() const { return is_deployed_; }

    void set_deployed(bool d = true) { is_deployed_ = d; }

    [[nodiscard]] std::vector<std::string> list_files(
            const std::filesystem::path& p, bool recursive = false) const;

    void extract(const std::filesystem::path& p, std::ostream& os) const;
};

#endif
