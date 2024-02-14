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

#ifndef PKGUPD_SYSROOT_H
#define PKGUPD_SYSROOT_H

#include "Deployment.h"
#include <filesystem>
#include <optional>
#include <ostree.h>
#include <vector>

struct Sysroot {
    std::filesystem::path root_dir;
    std::vector<Deployment> deployments;
    std::string osname;
    OstreeSysroot* sysroot{nullptr};
    OstreeRepo* repo{nullptr};

    explicit Sysroot(std::string osname, std::filesystem::path root_dir = "/");

    virtual ~Sysroot();

    void reload_deployments();

    void generate_boot_config(std::initializer_list<std::string> kargs);
};

#endif