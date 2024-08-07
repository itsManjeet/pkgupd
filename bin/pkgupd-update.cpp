#include "common.h"
#include <filesystem>
#include <ranges>

namespace fs = std::filesystem;

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(update) {
    os << "Update non-system packages of system" << endl
       << PADDING << " " << BOLD("Options:") << endl
       << PADDING << "  - system.packages=" << BOLD("<list>")
       << "    # Skip all system packages" << endl
       << PADDING << "  - update.exclude=" << BOLD("<list>")
       << "    # Specify package to exclude from update" << endl
       << endl;
}

PKGUPD_MODULE(update) {
    engine->load_system_database();

    PROCESS("syncing remote");
    engine->sync(true);

    std::stringstream changelog;

    PROCESS("checking outdated components")
    std::vector<MetaInfo> outdated_meta_infos;
    for (auto const& [_, installed_meta_info] : engine->list_installed()) {
        try {
            auto remote_meta_info =
                    engine->get_remote_meta_info(installed_meta_info.id);
            if (remote_meta_info.cache != installed_meta_info.cache) {
                changelog << remote_meta_info.id << " "
                          << installed_meta_info.version << ":"
                          << installed_meta_info.cache << " => "
                          << remote_meta_info.version << ":"
                          << remote_meta_info.cache;
                outdated_meta_infos.emplace_back(remote_meta_info);
            }
        } catch (const std::exception& exception) { ERROR(exception.what()); }
    }

    PROCESS("resolving dependencies")
    std::vector<MetaInfo> in_dependency_order;
    engine->resolve(outdated_meta_infos, in_dependency_order);

    if (in_dependency_order.empty()) {
        MESSAGE("SUCCESS", "system is upto date");
        return 0;
    }

    std::cout << changelog.str() << '\n'
              << in_dependency_order.size() << " component(s) is/are out dated";
    if (!ask_user("you do want to update the above components", config)) {
        return 0;
    }

    for (auto const& meta_info : in_dependency_order) {
        PROCESS("downlading " << meta_info.id);
        if (auto const cache_path = engine->download(
                    meta_info, config->get("downloader.force", false));
                !std::filesystem::exists(cache_path)) {
            throw std::runtime_error(
                    "failed to download cache " + cache_path.string());
        }
    }

    std::vector<std::string> deprecated_files;
    std::vector<InstalledMetaInfo> installed_meta_infos;
    for (auto const& meta_info : in_dependency_order) {
        PROCESS("installing " << meta_info.id);
        installed_meta_infos.emplace_back(
                engine->install(meta_info, deprecated_files));
    }

    engine->triggers(installed_meta_infos);

    for (auto file : std::ranges::reverse_view(deprecated_files)) {
        std::error_code error;
        if (file.starts_with("./")) file = file.substr(2);
        if (file.empty()) continue;
        auto path_to_remove = std::filesystem::path(config->get<std::string>(
                                      "dir.root", "/")) /
                              file;
        if (std::filesystem::is_directory(path_to_remove) &&
                !std::filesystem::is_empty(path_to_remove)) {
            continue;
        } else {
            DEBUG("removing " << path_to_remove.string());
            std::filesystem::remove(path_to_remove, error);
        }
        if (error) { ERROR("failed to remove deprecated file " << file); }
    }

    MESSAGE("SUCCESS", "System updated successfully");

    return 0;
}
