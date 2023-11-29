#include <ranges>

#include "Uninstaller.hxx"

using namespace rlxos::libpkgupd;

void Uninstaller::uninstall(const InstalledMetaInfo& installed_meta_info,
                            SystemDatabase* sys_db) {
    std::vector<std::string> package_files;

    auto root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
    sys_db->get_files(installed_meta_info, package_files);

    for (auto& package_file: std::ranges::reverse_view(package_files)) {
        auto filepath = std::filesystem::path(root_dir);
        if (package_file.find("./", 0) == 0) {
            filepath /= package_file.substr(2, package_file.length() - 2);
        } else {
            filepath /= package_file;
        }


        try {
            std::error_code error;
            if (std::filesystem::exists(filepath)) {
                if (std::filesystem::is_directory(filepath) &&
                    std::filesystem::is_empty(filepath)) {
                    std::filesystem::remove_all(filepath, error);
                } else if (!std::filesystem::is_directory(filepath)) {
                    std::filesystem::remove(filepath, error);
                }

                if (error) {
                    std::cerr << "FAILED TO REMOVE " << filepath << " : "
                            << error.message() << std::endl;
                }
            }
        } catch (std::exception const& exc) {
            std::cerr << "FAILED TO REMOVE " << filepath << " : " << exc.what()
                    << std::endl;
        }
    }

    sys_db->remove(installed_meta_info.id);
}
