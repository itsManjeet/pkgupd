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

#include "SysImage.h"

#include "Executor.h"
#include <algorithm>
#include <bits/ranges_algo.h>
#include <limits.h>
#include <utility>

SysImage::SysImage(
        Type type, std::filesystem::path path, bool is_deployed, bool is_active)
        : type_{type}, path_{std::move(path)}, is_deployed_{is_deployed},
          is_active_{is_active} {
    // auto [status, output] = Executor("/bin/unsquashfs")
    //                                 .arg("-cat")
    //                                 .arg("--quiet")
    //                                 .arg("--no-progress")
    //                                 .arg(path_)
    //                                 .arg(".rlxos-version")
    //                                 .output();
    // if (status != 0) {
    //     throw std::runtime_error("invalid deployment image '" + path.string()
    //     +
    //                              "' missing .rlxos-version file");
    // }
    // cache_ = output.erase(output.find_last_not_of('\n') + 1);

    std::stringstream os_release;
    extract("lib/os-release", os_release);
    for (std::string line; std::getline(os_release, line);) {
        auto idx = line.find_first_of('=');
        if (idx == std::string::npos) continue;
        auto key = line.substr(0, idx);
        auto value = line.substr(idx + 1);

        if (key.empty() || value.empty()) continue;
        // Don't be over-smart to inline this
        if (value[0] == '"') value = value.substr(1);
        if (value.back() == '"') value = value.substr(value.length() - 1);
        release_info_[key] = value;
    }

    if (auto version = release_info_.find("VERSION");
            (version == release_info_.end() || version->second.empty())) {
        throw std::runtime_error("no version in os-release");
    } else {
        try {
            this->version_ = std::stoi(version->second);
        } catch (...) { this->version_ = 0; }
    }
}

std::vector<std::string> SysImage::list_files(
        const std::filesystem::path& p, bool recursive) const {
    auto files = std::vector<std::string>();
    size_t depth = INT_MAX;
    if (!recursive) {
        depth = std::count(p.string().begin(), p.string().end(), '/') + 1;
    }
    auto [status, output] = Executor("/bin/unsquashfs")
                                    .arg("-ls")
                                    .arg("-max-depth")
                                    .arg(std::to_string(depth))
                                    .arg(path_)
                                    .arg(p)
                                    .output();
    if (status != 0) {
        throw std::runtime_error("failed to list files '" + output +
                                 "': " + std::to_string(status));
    }
    std::stringstream ss(output);
    auto path_prefix = std::filesystem::path("squashfs-root") / p;
    for (std::string line; std::getline(ss, line);) {
        if (line.empty()) continue;
        if (line.starts_with(path_prefix.string())) {
            line = line.substr(path_prefix.string().length());
            files.emplace_back(line);
        }
    }

    return files;
}

void SysImage::extract(const std::filesystem::path& p, std::ostream& os) const {
    auto status = Executor("/bin/unsquashfs")
                          .arg("-cat")
                          .arg(path_)
                          .arg(p)
                          .start()
                          .wait(&os);
    if (status != 0) {
        throw std::runtime_error("failed to extract file '" + p.string() + "'");
    }
}
