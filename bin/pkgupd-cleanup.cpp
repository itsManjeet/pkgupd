#include "common.h"

PKGUPD_MODULE_HELP(cleanup) {
    os << "clean pkgupd cache including:" << std::endl
       << PADDING << "  - package cache" << std::endl
       << PADDING << "  - source files " << std::endl
       << PADDING << "  - recipe data" << std::endl;
}

PKGUPD_MODULE(cleanup) {
    auto pkg_dir = config->get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) + "/packages";
    auto src_dir = config->get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) + "/sources";
    if (std::filesystem::exists(pkg_dir) &&
        ask_user("Do you want to clean package directory '" + pkg_dir + "'",
                 config)) {
        std::error_code error;
        PROCESS("cleaning source cache");
        std::filesystem::remove(pkg_dir, error);
        if (error) {
            ERROR("failed to remove '" + pkg_dir + "'");
        }
        std::filesystem::create_directories(pkg_dir, error);
        if (error) {
            ERROR("failed to create new directory '" + pkg_dir + "'");
        }
    }

    if (std::filesystem::exists(src_dir) &&
        ask_user("Do you want to clean sources directory '" + src_dir + "'",
                 config)) {
        std::error_code error;
        PROCESS("cleanin source cache");
        std::filesystem::remove(src_dir, error);
        if (error) {
            ERROR("failed to remove '" + pkg_dir + "'");
        }
        std::filesystem::create_directories(src_dir, error);
        if (error) {
            ERROR("failed to create new directory '" + src_dir + "'");
        }
    }

    return 0;
}
