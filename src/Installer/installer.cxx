#include "installer.hxx"

#include <cstring>

using namespace rlxos::libpkgupd;

#include "../ArchiveManager/ArchiveManager.hxx"
#include "../common.hxx"
#include "../downloader.hxx"
#include "../repository.hxx"
#include "../resolver.hxx"
#include "../Trigger/Trigger.hxx"

void Installer::install(
    std::vector<std::pair<std::string, MetaInfo>> const& pkgs,
    SystemDatabase* sys_db) {
    std::vector<std::pair<InstalledMetaInfo, std::vector<std::string>>>
            packages;

    std::filesystem::path root_dir =
            mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

    if (access(root_dir.c_str(), W_OK) != 0) {
        throw std::runtime_error("no write permission on root directory = '" + root_dir.string() +
                                 "', " + std::string(strerror(errno)));
    }
    for (auto const& [package_path, meta_info]: pkgs) {
        if (!std::filesystem::exists(package_path)) {
            throw std::runtime_error("package file '" + package_path + "' not exists");
        }
        PROCESS(
            "installing " << meta_info.id << " " << meta_info.version << " " << meta_info.cache
            << (true ? " as dependency" : ""));

        std::vector<std::string> backups;
        mConfig->get("backup", backups);

        backups.insert(backups.end(), meta_info.backup.begin(),
                       meta_info.backup.end());

        if (!mConfig->get("no-backup", false)) {
            for (auto const& i: backups) {
                std::filesystem::path backup_path = root_dir / i;
                if (std::filesystem::exists(backup_path)) {
                    std::error_code error;
                    std::filesystem::copy(
                        backup_path, (backup_path.string() + ".old"),
                        std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::overwrite_existing,
                        error);
                    if (error) {
                        throw std::runtime_error("failed to backup file " + backup_path.string());
                    }
                }
            }
        }
        auto package_meta_info = ArchiveManager::info(package_path);

        std::vector<std::string> files;
        ArchiveManager::list(package_path, files);

        root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

        ArchiveManager::extract(package_path, root_dir, files);

        if (!mConfig->get("no-backup", false)) {
            for (auto const& i: backups) {
                std::filesystem::path backup_path = root_dir / i;
                if (std::filesystem::exists(backup_path) &&
                    std::filesystem::exists((backup_path.string() + ".old"))) {
                    std::error_code error;
                    std::filesystem::copy(
                        backup_path, (backup_path.string() + ".new"),
                        std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::overwrite_existing,
                        error);
                    if (error) {
                        throw std::runtime_error("failed to add new backup file " + backup_path.string());
                    }

                    std::filesystem::rename((backup_path.string() + ".old"), backup_path,
                                            error);
                    if (error) {
                        throw std::runtime_error("failed to recover backup file " + backup_path.string() +
                                                 ", " + error.message());
                    }
                }
            }
        }
        // TODO: handle is_dependency;
        bool is_dependency = false;

        auto old_package_info = sys_db->get(meta_info.id);
        if (old_package_info.has_value()) {
            is_dependency = old_package_info->dependency;

            std::vector<std::string> old_files;
            try {
                sys_db->get_files(*old_package_info, old_files);
            } catch (...) {
            }
            if (!old_files.empty()) {
                PROCESS("cleaning old packages")
                for (auto i = old_files.rbegin(); i != old_files.rend(); ++i) {
                    std::string file = *i;
                    if (file.length()) {
                        file = file.substr(2);
                    }
                    if (std::filesystem::exists(root_dir / file) &&
                        std::find(files.begin(), files.end(), "./" + file) ==
                        files.end()) {
                        if (file.find("./bin", 0) == 0 || file.find("./lib", 0) == 0 ||
                            file.find("./sbin", 0) == 0) {
                            continue;
                        }

                        std::error_code error;

                        DEBUG("removing " << root_dir / file)
                        std::filesystem::remove(root_dir / file, error);
                        if (error)
                            ERROR("failed to remove " << file);
                    }
                }
            }
        }

        auto installed_package_info =
                sys_db->add(meta_info, files,
                            mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR),
                            false, is_dependency);
        packages.push_back({installed_package_info, files});
    }

    if (!mConfig->get("installer.triggers", true)) {
        INFO("skipping triggers");
        std::cout << BOLD("successfully") << " "
                << BLUE((mConfig->get("is-updating", false) ? "installed"
                    : "updated"))
                << " " << GREEN(packages.size()) << BOLD(" package(s)")
                << std::endl;
        return;
    }

    Triggerer triggerer;
    triggerer.trigger(packages);

    std::cout << BOLD("successfully") << " "
            << BLUE((mConfig->get("is-updating", false) ? "installed"
                : "updated"))
            << " " << GREEN(packages.size()) << BOLD(" package(s)")
            << std::endl;
}

void Installer::install(std::vector<MetaInfo> pkgs, Repository* repository,
                        SystemDatabase* system_database) {
    std::vector<std::pair<std::string, MetaInfo>> packages_pair;
    Downloader downloader(mConfig);

    std::filesystem::path root_dir =
            mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
    std::filesystem::path pkgs_dir =
            mConfig->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR);

    std::vector<MetaInfo> required_packages, packages_to_install;

    if (mConfig->get("force", false) == false) {
        for (auto const& p: pkgs) {
            auto system_package = system_database->get(p.id);
            if (system_package.has_value() &&
                system_package->cache == p.cache) {
                INFO(p.id << " is already installed and upto date");
                continue;
            }
            required_packages.push_back(p);
        }
    } else {
        required_packages = pkgs;
    }

    if (mConfig->get("installer.depends", true) == true) {
        auto resolver =
                Resolver<MetaInfo>(DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION,
                                   DEFAULT_DEPENDS_FUNCTION);
        PROCESS("generating dependency graph");
        std::vector<MetaInfo> dependencies;
        for (auto const& p: required_packages) {
            resolver.depends(p, dependencies);
        }

        auto skip_fun = DEFAULT_SKIP_PACKAGE_FUNCTION;

        for (auto const& p: required_packages) {
            auto iter = std::find_if(dependencies.begin(), dependencies.end(),
                                     [&p](const MetaInfo& meta_info) -> bool {
                                         return p.id == meta_info.id;
                                     });
            if (iter == dependencies.end()) {
                if (mConfig->get("force", false) == false) {
                    if (!skip_fun(p)) dependencies.push_back(p);
                } else {
                    dependencies.push_back(p);
                }
            } else {
                // TODO: fix here
                // (*iter)->unsetDependency();
            }
        }

        if (dependencies.size()) {
            MESSAGE(BLUE("::"), "required " << dependencies.size() << " package(s)");
            for (auto const& i: dependencies) {
                std::cout << "- " << i.id << ":" << i.version << std::endl;
            }
            ask_user("Do you want to continue", mConfig);
        }

        packages_to_install = dependencies;
    } else {
        for (auto const& i: required_packages) {
            packages_to_install.push_back(i);
        }
    }

    if (packages_to_install.size() == 0) {
        INFO("no operation required");
        return;
    }

    for (auto i: packages_to_install) {
        auto package_name = i.package_name();
        std::filesystem::path package_path =
                pkgs_dir / package_name;

        if (!std::filesystem::exists(package_path.parent_path())) {
            std::error_code err;
            std::filesystem::create_directories(package_path.parent_path(), err);
            if (err) {
                throw std::runtime_error(
                    "failed to create required directory " + package_path.parent_path().string() + " '" + err.message()
                    + "'");
            }
        }

        if (std::filesystem::exists(package_path) &&
            !mConfig->get("download.force", false)) {
            INFO(i.id << " found in cache");
        } else {
            PROCESS("downloading " << i.id);
            downloader.get(("cache/" + package_name),
                           package_path.c_str());
        }

        packages_pair.push_back({package_path, i});
    }
    return install(packages_pair, system_database);
}
