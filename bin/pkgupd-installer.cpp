#include "common.h"


PKGUPD_MODULE_HELP(install) {
    os << "Install package from repository" << std::endl
       << PADDING << " " << BOLD("Options:") << std::endl
       << PADDING << "  - installer.depends=" << BOLD("<bool>")
       << "   # Enable or disable dependency management" << std::endl
       << PADDING << "  - force=" << BOLD("<bool>")
       << "               # Force installation of already installed package"
       << std::endl
       << PADDING << "  - installer.triggers=" << BOLD("<bool>")
       << "  # Skip triggers during installation, also include user/group "
          "creation"
       << std::endl
       << PADDING << "  - downloader.force=" << BOLD("<bool>")
       << "    # Force redownload of specified package" << std::endl
       << std::endl;
}

PKGUPD_MODULE(install) {
    engine->sync(false);

    std::vector<MetaInfo> meta_infos;
    if (config->get("installer.depends", true)) {
        PROCESS("calculating dependencies")
        engine->resolve(args, meta_infos);
        if (meta_infos.empty()) {
            INFO("dependencies already satisfied");
            return 0;
        }
    } else {
        for (auto const &id: args) {
            meta_infos.emplace_back(engine->get_remote_meta_info(id));
        }
    }


    if (meta_infos.size() > 1) {
        int count = 1;
        for (auto const &meta_info: meta_infos) {
            std::cout << count++ << ". " << meta_info.id << std::endl;
        }
        if (!ask_user("Do you want to install " + std::to_string(meta_infos.size()) + " package(s)", config)) {
            return 0;
        }
    }

    for (auto const &meta_info: meta_infos) {
        PROCESS("downlading " << meta_info.id);
        if (auto const cache_path = engine->download(meta_info, config->get("downloader.force", false)); !
                std::filesystem::exists(cache_path)) {
            throw std::runtime_error("failed to download cache " + cache_path.string());
        }
    }


    std::vector<std::string> deprecated_files;
    std::vector<InstalledMetaInfo> installed_meta_infos;
    for (auto const &meta_info: meta_infos) {
        PROCESS("installing " << meta_info.id);
        installed_meta_infos.emplace_back(engine->install(meta_info, deprecated_files));
    }

    try {
        engine->triggers(installed_meta_infos);
    } catch (const std::exception &exception) {
        ERROR(exception.what());
    }


    return 0;
}
