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

#ifndef PKGUPD_BUILDER_H
#define PKGUPD_BUILDER_H

#include <utility>
#include <filesystem>
#include "MetaInfo.h"


class Builder {
public:
    struct BuildInfo : MetaInfo {
        std::vector<std::string> build_time_depends, sources;

        BuildInfo() = default;

        explicit BuildInfo(const std::string &input);

    };

    struct Compiler {
        std::string file;
        std::string script;
    };

private:
    const Configuration &config;
    const BuildInfo &build_info;
    std::map<std::string, Compiler> compilers;

public:
    explicit Builder(const Configuration &config, const BuildInfo &build_info);

    std::optional<std::filesystem::path>
    prepare_sources(const std::filesystem::path &source_dir, const std::filesystem::path &build_root);

    Compiler get_compiler(const std::filesystem::path &build_root);

    void compile_source(const std::filesystem::path &build_root, const std::filesystem::path &install_root);

    void pack(const std::filesystem::path &install_root, const std::filesystem::path &package);
};


#endif //PKGUPD_BUILDER_H
