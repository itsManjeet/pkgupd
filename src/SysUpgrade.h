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

#ifndef PKGUPD_SYSUPGRADE_H
#define PKGUPD_SYSUPGRADE_H

#include "Sysroot.h"
#include <optional>

class SysUpgrade {
public:
    struct UpdateInfo {
        int version;
        std::string changelog;
        std::string cache;
        std::string url;
    };
private:
    Sysroot *sysroot{nullptr};

public:
    explicit SysUpgrade(Sysroot *sysroot) : sysroot{sysroot} {

    }

    void create_sign(const std::filesystem::path &path, const std::filesystem::path &sign_path);

    std::optional<UpdateInfo> update(const std::string &url, bool dry_run = false);

    int compare(const std::filesystem::path &file, const std::filesystem::path &sign, std::vector<int>& blocks);
};


#endif //PKGUPD_SYSUPGRADE_H
