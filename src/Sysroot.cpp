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

#include "Error.h"
#include "defines.hxx"
#include <algorithm>
#include <glib.h>
#include <utility>

Sysroot::Sysroot(std::string osname, std::filesystem::path root_dir)
        : root_dir{std::move(root_dir)}, osname{std::move(osname)} {
    GError* error = nullptr;
    const std::unique_ptr<GFile, decltype(&g_object_unref)> file(
            g_file_new_for_path(this->root_dir.c_str()), g_object_unref);
    sysroot = ostree_sysroot_new(file.get());
    ostree_sysroot_initialize_with_mount_namespace(sysroot, nullptr, &error);

    if (!ostree_sysroot_load(sysroot, nullptr, &error)) { throw Error(error); }

    if (!ostree_sysroot_get_repo(sysroot, &repo, nullptr, &error)) {
        throw Error(error);
    }

    reload_deployments();
}

Sysroot::~Sysroot() {
    ostree_sysroot_unload(sysroot);

    if (repo) g_object_unref(repo);
    repo = nullptr;

    // if (sysroot) g_object_unref(sysroot);
    // sysroot = nullptr;
}

void Sysroot::reload_deployments() {
    deployments.clear();

    std::unique_ptr<OstreeDeployment, decltype(&g_object_unref)>
            booted_deployment(ostree_sysroot_get_booted_deployment(sysroot),
                    g_object_unref);

    if (booted_deployment == nullptr) {
        throw std::runtime_error("no booted deployment found");
    }

    std::unique_ptr<GPtrArray, void (*)(GPtrArray*)> deployment_list(
            ostree_sysroot_get_deployments(sysroot),
            +[](GPtrArray* array) -> void { g_ptr_array_free(array, true); });
    for (auto i = 0; i < deployment_list->len; i++) {
        deployments.push_back(Deployment(
                reinterpret_cast<OstreeDeployment*>(deployment_list->pdata[i]),
                repo));

        if (ostree_deployment_equal(
                    deployments.back().backend, booted_deployment.get())) {
            deployments.back().is_active = true;
        }
    }
}
void Sysroot::generate_boot_config(std::initializer_list<std::string> kargs) {}
