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

#include "Sysroot.h"

#include "defines.hxx"
#include <algorithm>
#include <fstream>
#include <utility>

Sysroot::Sysroot(std::string osname, std::filesystem::path root_dir)
        : osname_{std::move(osname)}, root_dir_{std::move(root_dir)} {
    boot_dir_ = root_dir_ / "boot";
    deployment_dir_ = root_dir_ / "system" / "deploy";

    reload_deployments();
    std::ifstream reader("/proc/mount");
    for (std::string line; std::getline(reader, line);) {
        std::stringstream ss(line);
        std::string source, target, filesystem, options;
        ss >> source >> target >> filesystem >> options;
        if (target == (root_dir_ / "usr").string()) {
            active_deployment_ = source;
            break;
        }
    }
}

void Sysroot::reload_deployments() {
    deployments_.clear();
    if (!std::filesystem::exists(deployment_dir_)) {
        throw std::runtime_error("no deployment found");
    }
    for (auto const& image :
            std::filesystem::directory_iterator(deployment_dir_)) {
        if (image.is_directory()) continue;
        if (image.path().has_extension() && image.path().extension() == ".tmp")
            continue;
        try {
            auto deployment =
                    SysImage(SysImage::Type::Image, image.path(), false,
                            (!active_deployment_.empty() &&
                                    active_deployment_ == image.path()));
            if (std::filesystem::exists(boot_dir_ /
                                        std::to_string(deployment.version()) /
                                        "kernel") &&
                    std::filesystem::exists(
                            boot_dir_ / std::to_string(deployment.version()) /
                            "initrd")) {
                deployment.set_deployed();
            }
            deployments_.emplace_back(deployment);
        } catch (const std::exception& exception) {
            ERROR("skipping " << image << ": " << exception.what());
        }
    }
    std::sort(deployments_.begin(), deployments_.end(),
            [](const SysImage& a, const SysImage& b) -> bool {
                return a.version() < a.version();
            });
}

void Sysroot::deploy(const SysImage& sys_image) {
    DEBUG("VERSION: " << sys_image.version());

    PROCESS("Checking kernel");
    auto kernel_list = sys_image.list_files("lib/modules/");
    if (kernel_list.empty()) {
        throw std::runtime_error("no kernel image found");
    }
    auto kernel_version = kernel_list.front();
    DEBUG("VERSION: " << kernel_version);

    auto deploy_boot_dir = boot_dir_ / std::to_string(sys_image.version());
    std::filesystem::create_directories(deploy_boot_dir);
    {
        PROCESS("Installing kernel")
        std::ofstream ks(deploy_boot_dir / "kernel", std::ios_base::binary);
        sys_image.extract("lib/modules/" + kernel_version + "/bzImage", ks);
    }
    {
        PROCESS("Installing initial ramdisk")
        std::ofstream is(deploy_boot_dir / "initrd", std::ios_base::binary);
        sys_image.extract("lib/modules/" + kernel_version + "/initramfs", is);
    }
    {
        PROCESS("Installing system image")
        auto target_path = deployment_dir_ / sys_image.path().filename();
        if (target_path != sys_image.path()) {
            std::filesystem::copy_file(sys_image.path(), target_path,
                    std::filesystem::copy_options::update_existing);
        }
    }
}

void Sysroot::generate_boot_config(std::initializer_list<std::string> kargs) {
    auto grub_config_path = boot_dir_ / "grub" / "grub-entries.cfg";
    std::ofstream grub_writer(grub_config_path);
    if (!grub_writer.is_open()) {
        throw std::runtime_error("failed to open " + grub_config_path.string());
    }
    std::string kargs_str;
    for (auto const& i : kargs) {
        if (i.starts_with("rd.image=")) continue;
        kargs_str += " " + i;
    }

    auto boot_loader_entries = boot_dir_ / "loader" / "entries";
    bool gen_boot_loader_specifications =
            std::filesystem::exists(boot_loader_entries);

    if (gen_boot_loader_specifications) {
        for (auto const& i :
                std::filesystem::directory_iterator(boot_loader_entries)) {
            if (i.is_regular_file()) {
                auto filename = i.path().filename();
                if (filename.has_extension() &&
                        filename.extension() == ".conf" &&
                        filename.string().starts_with(osname_ + "-")) {
                    std::filesystem::remove(i.path());
                }
            }
        }
    }
    int count = 1;
    for (auto const& deployment : deployments_) {
        grub_writer << "menuentry '" << osname_ << " " << deployment.version()
                    << "' {\n"
                    << "   linux /boot/" << deployment.version()
                    << "/kernel rd.image=" << deployment.version() << kargs_str
                    << "\n"
                    << "   initrd /boot/" << deployment.version() << "/initrd\n"
                    << "}\n";
        if (gen_boot_loader_specifications) {
            std::ofstream entry_writer(
                    boot_loader_entries /
                    (osname_ + "-" + std::to_string(count++) + ".conf"));
            entry_writer << "title " << osname_ << " " << deployment.version()
                         << '\n'
                         << "version " << deployment.version() << '\n'
                         << "options rd.image=" << deployment.version()
                         << kargs_str << '\n'
                         << "linux /boot/" << deployment.version() << "/kernel"
                         << '\n'
                         << "initrd /boot/" << deployment.version()
                         << "/initrd\n";
        }
    }
}
