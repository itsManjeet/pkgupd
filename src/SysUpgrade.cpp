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

#include "SysUpgrade.h"

#include <fstream>
#include "Downloader.h"
#include "Executor.h"

#include "picosha2.h"

#include <random>

#define BLOCK_SIZE 4096

std::string random_string(int length) {
    const std::string VALID_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, VALID_CHARS.size() - 1);

    std::string buffer;
    buffer.reserve(length);

    for (int i = 0; i < length; ++i) {
        buffer += VALID_CHARS[dis(gen)];
    }

    return buffer;
}

std::optional<SysUpgrade::UpdateInfo> SysUpgrade::update(const std::string &url, bool dry_run) {
    auto response = Downloader(url + "/update")
            .get_json();
    auto latest_installed = sysroot->deployments().front();
    if (response.contains("release")) {
        throw std::runtime_error("invalid response");
    }
    auto temp_file = std::filesystem::temp_directory_path() / random_string(10);

    auto latest_available_version = response["release"].get<int>();
    if (latest_installed.version() > latest_available_version) {
        auto update_info = UpdateInfo{
                .version = latest_available_version,
                .changelog = response["changelog"].get<std::string>(),
                .cache = response["cache"].get<std::string>(),
                .url = response["url"].get<std::string>(),
        };

        if (!dry_run) {
            Executor("/bin/zsync")
                    .arg("-i").arg(latest_installed.path())
                    .arg("-o").arg(temp_file)
                    .arg(update_info.url + ".zsync")
                    .execute();
            sysroot->deploy(SysImage(SysImage::Type::Image, temp_file));
            sysroot->reload_deployments();
            sysroot->generate_boot_config({});
        }
        return update_info;
    }
    return std::nullopt;
}
