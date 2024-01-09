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

#include <filesystem>
#include <vector>
#include <optional>
#include <map>
#include "SysImage.h"

class Sysroot {
private:
    std::filesystem::path boot_dir_, root_dir_;
    std::vector<SysImage> deployments_;
    std::string active_deployment_;
    std::string osname_;

public:
    explicit Sysroot(std::string osname, std::filesystem::path root_dir = "/");

    void reload_deployments();

    void deploy(const SysImage &deployment);

    [[nodiscard]] std::vector<SysImage> const &deployments() const { return deployments_; }

    void generate_boot_config(std::initializer_list<std::string> kargs);

    const std::string &osname() const { return osname_; }

};

#endif