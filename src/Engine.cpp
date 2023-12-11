#include <utility>

#include "Engine.h"

#include <ranges>
#include <format>

#include "Resolver.h"
#include "ArchiveManager.h"
#include "Trigger.h"
#include "picosha2.h"


Engine::Engine(const Configuration &config)
        : config(config),
          root(config.get<std::string>(DIR_ROOT, "/")),
          server(config.get<std::string>("server", "http://repo.rlxos.dev")) {
    system_database.load(config.get<std::string>(DIR_DATA, DEFAULT_DATA_DIR));
}

std::filesystem::path Engine::download(const MetaInfo &meta_info, bool force) const {
    auto cache_file = root / config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) / "cache" / meta_info.
            package_name();
    if (std::filesystem::exists(cache_file) && !force) {
        return cache_file;
    }
    Downloader::download(server + "/cache/" + meta_info.cache, cache_file);
    return cache_file;
}

void Engine::uninstall(const InstalledMetaInfo &installed_meta_info) {
    std::vector<std::string> installed_files_list;
    system_database.get_files(installed_meta_info, installed_files_list);
    for (auto const &installed_file: std::ranges::reverse_view(installed_files_list)) {
        auto installed_filepath = root / installed_file;

        try {
            if (std::filesystem::exists(installed_filepath)) {
                std::error_code error;
                if (std::filesystem::is_directory(installed_filepath) &&
                    std::filesystem::is_empty(installed_filepath)) {
                    std::filesystem::remove_all(installed_filepath, error);
                } else if (std::filesystem::is_symlink(installed_filepath)) {
                    std::filesystem::remove(installed_filepath, error);
                } else if (!std::filesystem::is_directory(installed_filepath)) {
                    std::filesystem::remove(installed_filepath, error);
                }

                if (error) {
                    std::cerr << "FAILED TO REMOVE " << installed_filepath << " : "
                              << error.message() << std::endl;
                }
            }
        }
        catch (std::exception const &exc) {
            std::cerr << "FAILED TO REMOVE " << installed_filepath << " : " << exc.what()
                      << std::endl;
        }
    }
    system_database.remove(installed_meta_info);
}

InstalledMetaInfo Engine::install(const MetaInfo &meta_info, std::vector<std::string> &deprecated_files) {
    auto cache_file = root / config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) / "cache" / meta_info.
            package_name();
    if (!std::filesystem::exists(cache_file)) {
        throw std::runtime_error("missing cache file " + cache_file.string());
    }

    std::vector<std::string> files_list;
    // Just to verify package
    ArchiveManager::list(cache_file, files_list);

    if (!config.get("no-backup", false)) {
        for (const auto &file: meta_info.backup) {
            auto const filepath = root / file;
            if (std::filesystem::exists(filepath)) {
                std::error_code error;
                std::filesystem::copy(
                        filepath, (filepath.string() + ".old"),
                        std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::overwrite_existing,
                        error);
                if (error) {
                    throw std::runtime_error("failed to backup file " + filepath.string());
                }
            }
        }
    }

    files_list.clear();
    ArchiveManager::extract(cache_file, root, files_list);

    if (!config.get("no-backup", false)) {
        for (const auto &file: meta_info.backup) {
            auto const filepath = root / file;
            if (std::filesystem::exists(filepath) &&
                std::filesystem::exists((filepath.string() + ".old"))) {
                std::error_code error;
                std::filesystem::copy(
                        filepath, (filepath.string() + ".new"),
                        std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::overwrite_existing,
                        error);
                if (error) {
                    throw std::runtime_error("failed to add new backup file " + filepath.string());
                }

                std::filesystem::rename((filepath.string() + ".old"), filepath,
                                        error);
                if (error) {
                    throw std::runtime_error("failed to recover backup file " + filepath.string() +
                                             ", " + error.message());
                }
            }
        }
    }


    if (auto const old_installed_meta_info = system_database.get(meta_info.id); old_installed_meta_info) {
        std::vector<std::string> old_files_list;
        try {
            system_database.get_files(*old_installed_meta_info, old_files_list);
        }
        catch (...) {
            // No need to stop execution here
        }
        deprecated_files.insert(deprecated_files.end(), old_files_list.begin(), old_files_list.end());
    }

    return system_database.add(meta_info, files_list, root, false, false);
}

std::map<std::string, InstalledMetaInfo> const &Engine::list_installed() const {
    return system_database.get();
}

void Engine::list_installed_files(const InstalledMetaInfo &installed_meta_info,
                                  std::vector<std::string> &files_list) {
    system_database.get_files(installed_meta_info, files_list);
}

void Engine::triggers(const std::vector<InstalledMetaInfo> &installed_meta_infos) const {
    std::vector<std::pair<InstalledMetaInfo, std::vector<std::string>>> files_list;
    for (auto const &installed_meta_info: installed_meta_infos) {
        files_list.push_back({installed_meta_info, {}});
        system_database.get_files(installed_meta_info, files_list.back().second);
    }

    Triggerer triggerer;
    triggerer.trigger(files_list);
}

void Engine::triggers() const {
    Triggerer triggerer;
    triggerer.trigger();
}

void Engine::sync(bool force) {
    auto repo_path = config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) + "/repo";
    if (!(std::filesystem::exists(repo_path) && !force)) {
        Downloader::download(server + "/origin", repo_path);
    }
    repository.load(repo_path);
}

void Engine::resolve(const std::vector<std::string> &ids, std::vector<MetaInfo> &output_meta_infos) const {
    auto resolver = Resolver<MetaInfo>(
            [&](const std::string &id) -> std::optional<MetaInfo> { return repository.get(id); },
            [&](const MetaInfo &meta_info) -> bool { return system_database.get(meta_info.id) != std::nullopt; },
            [&](const MetaInfo &pkg) -> std::vector<std::string> { return pkg.depends; }
    );

    resolver.depends(ids, output_meta_infos);
}

void Engine::resolve(const std::vector<MetaInfo> &meta_infos, std::vector<MetaInfo> &output_meta_infos) const {
    auto resolver = Resolver<MetaInfo>(
            [&](const std::string &id) -> std::optional<MetaInfo> { return repository.get(id); },
            [&](const MetaInfo &meta_info) -> bool { return system_database.get(meta_info.id) != std::nullopt; },
            [&](const MetaInfo &pkg) -> std::vector<std::string> { return pkg.depends; }
    );
    std::vector<std::string> ids;

    for (auto const &meta_info: meta_infos) {
        ids.push_back(meta_info.id);
    }
    resolver.depends(ids, output_meta_infos);
}

MetaInfo Engine::get_remote_meta_info(const std::string &id) const {
    if (auto const meta_info = repository.get(id); meta_info) {
        return *meta_info;
    }
    throw std::runtime_error("no component found with id " + id);
}

std::map<std::string, MetaInfo> const &Engine::list_remote() const {
    return repository.get();
}

std::filesystem::path Engine::build(const Builder::BuildInfo &build_info) {
    std::string hash_sum;

    {
        std::stringstream ss;
        ss << build_info.config.node;
        picosha2::hash256_hex_string(ss.str(), hash_sum);
    }

    for (auto const &dep: build_info.depends) {
        auto meta_info = this->repository.get(dep);
        if (!meta_info) throw std::runtime_error("missing runtime dependency '" + dep + "'");
        picosha2::hash256_hex_string(meta_info->str() + hash_sum, hash_sum);
    }

    for (auto const &dep: build_info.build_time_depends) {
        auto meta_info = this->repository.get(dep);
        if (!meta_info) throw std::runtime_error("missing buildtime dependency '" + dep + "'");
        picosha2::hash256_hex_string(meta_info->str() + hash_sum, hash_sum);
    }

    std::filesystem::path work_dir = config.get<std::string>("dir.build", std::filesystem::current_path() / "build");
    std::filesystem::path source_dir = config.get<std::string>("dir.sources", work_dir / "sources");
    std::filesystem::path packages_dir = config.get<std::string>("dir.packages", work_dir / "packages");

    auto build_root = work_dir / "build-root";
    auto install_root = work_dir / "install-root";

    auto package_path = packages_dir / std::format("{}-{}-{}.pkg", build_info.id, build_info.version, hash_sum);

    for (auto const &path: {build_root, install_root}) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }

    for (auto const &path: {source_dir, packages_dir, build_root, install_root}) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }

    auto builder = Builder(config, build_info);
    auto subdir = builder.prepare_sources(source_dir, build_root);
    if (!subdir) subdir = ".";

    build_root /= build_info.config.get<std::string>("build-dir", subdir->string());
    builder.compile_source(build_root, install_root);
    builder.pack(install_root, package_path);

    for (auto const &path: {build_root, install_root}) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }


    return package_path;
}
