#include "Engine.h"

#include "ArchiveManager.h"
#include "Resolver.h"
#include "Trigger.h"
#include "picosha2.h"
#include <ranges>
#include <utility>

Engine::Engine(const Configuration& config)
        : config(config), root(config.get<std::string>(DIR_ROOT, "/")),
          server(config.get<std::string>("server", "http://repo.rlxos.dev")) {}

std::filesystem::path Engine::download(
        const MetaInfo& meta_info, bool force) const {
    for (auto const& ext : {"", ".doc", ".devel"}) {
        auto cache_file =
                root / config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) /
                "cache" / (meta_info.package_name() + ext);
        if (std::filesystem::exists(cache_file) && !force) { continue; }

        Http().url(server + "/cache/" + meta_info.package_name() + ext)
                .download(cache_file);
        if (!std::filesystem::exists(cache_file))
            throw std::runtime_error("failed to download " + meta_info.name());
    }

    return root / config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) /
           "cache" / meta_info.package_name();
}

void Engine::uninstall(const InstalledMetaInfo& installed_meta_info) {
    std::vector<std::string> installed_files_list;
    system_database.get_files(installed_meta_info, installed_files_list);
    for (auto const& installed_file :
            std::ranges::reverse_view(installed_files_list)) {
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
                    std::cerr << "FAILED TO REMOVE " << installed_filepath
                              << " : " << error.message() << std::endl;
                }
            }
        } catch (std::exception const& exc) {
            std::cerr << "FAILED TO REMOVE " << installed_filepath << " : "
                      << exc.what() << std::endl;
        }
    }
    system_database.remove(installed_meta_info);
}

InstalledMetaInfo Engine::install(
        const MetaInfo& meta_info, std::vector<std::string>& deprecated_files) {
    auto cache_file = root /
                      config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) /
                      "cache" / meta_info.package_name();
    std::vector<std::string> files_list;

    std::vector<std::string> old_files_list;
    if (auto const old_installed_meta_info = system_database.get(meta_info.id);
            old_installed_meta_info) {
        try {
            system_database.get_files(*old_installed_meta_info, old_files_list);
        } catch (...) {
            // No need to stop execution here
        }
    }

    for (auto const& ext : {"", ".doc", ".devel"}) {
        auto package_cache_file = cache_file.string() + ext;
        if (!std::filesystem::exists(package_cache_file)) {
            throw std::runtime_error(
                    "missing cache file " + package_cache_file);
        }
        // Just to verify package
        ArchiveManager::list(package_cache_file, files_list);
        if (!config.get("no-backup", false)) {
            for (const auto& file : meta_info.backup) {
                auto const filepath = root / file;
                if (std::filesystem::exists(filepath)) {
                    std::error_code error;
                    std::filesystem::copy(filepath,
                            (filepath.string() + ".old"),
                            std::filesystem::copy_options::recursive |
                                    std::filesystem::copy_options::
                                            overwrite_existing,
                            error);
                    if (error) {
                        throw std::runtime_error(
                                "failed to backup file " + filepath.string());
                    }
                }
            }
        }

        std::vector<std::string> dummy_files_list;
        ArchiveManager::extract(cache_file, root, dummy_files_list);

        if (!config.get("no-backup", false)) {
            for (const auto& file : meta_info.backup) {
                auto const filepath = root / file;
                if (std::filesystem::exists(filepath) &&
                        std::filesystem::exists((filepath.string() + ".old"))) {
                    std::error_code error;
                    std::filesystem::copy(filepath,
                            (filepath.string() + ".new"),
                            std::filesystem::copy_options::recursive |
                                    std::filesystem::copy_options::
                                            overwrite_existing,
                            error);
                    if (error) {
                        throw std::runtime_error(
                                "failed to add new backup file " +
                                filepath.string());
                    }

                    std::filesystem::rename(
                            (filepath.string() + ".old"), filepath, error);
                    if (error) {
                        throw std::runtime_error(
                                "failed to recover backup file " +
                                filepath.string() + ", " + error.message());
                    }
                }
            }
        }
    }

    if (!old_files_list.empty()) {
        for (auto const& i : old_files_list) {
            if (std::find(files_list.begin(), files_list.end(), i) ==
                    files_list.end()) {
                deprecated_files.emplace_back(i);
            }
        }
    }

    return system_database.add(meta_info, files_list, root, false, false);
}

std::map<std::string, InstalledMetaInfo> const& Engine::list_installed() const {
    return system_database.get();
}

void Engine::list_installed_files(const InstalledMetaInfo& installed_meta_info,
        std::vector<std::string>& files_list) {
    system_database.get_files(installed_meta_info, files_list);
}

void Engine::triggers(
        const std::vector<InstalledMetaInfo>& installed_meta_infos) const {
    std::vector<std::pair<InstalledMetaInfo, std::vector<std::string>>>
            files_list;
    for (auto const& installed_meta_info : installed_meta_infos) {
        files_list.push_back({installed_meta_info, {}});
        system_database.get_files(
                installed_meta_info, files_list.back().second);
    }

    Triggerer triggerer;
    triggerer.trigger(files_list);
}

void Engine::triggers() const {
    Triggerer triggerer;
    triggerer.trigger();
}

void Engine::sync(bool force) {
    auto repo_path =
            config.get<std::string>(DIR_CACHE, DEFAULT_CACHE_DIR) + "/repo";
    if (!(std::filesystem::exists(repo_path) && !force)) {
        Http().url(server + "/" + config.get<std::string>("version", "stable"))
                .download(repo_path);
    }
    repository.load(repo_path);
}

void Engine::resolve(const std::vector<std::string>& ids,
        std::vector<MetaInfo>& output_meta_infos) const {
    auto resolver = Resolver<MetaInfo>(
            [&](const std::string& id) -> std::optional<MetaInfo> {
                return repository.get(id);
            },
            [&](const MetaInfo& meta_info) -> bool {
                return system_database.get(meta_info.id) != std::nullopt;
            },
            [&](const MetaInfo& pkg) -> std::vector<std::string> {
                return pkg.depends;
            });

    resolver.depends(ids, output_meta_infos);
}

void Engine::resolve(const std::vector<MetaInfo>& meta_infos,
        std::vector<MetaInfo>& output_meta_infos) const {
    auto resolver = Resolver<MetaInfo>(
            [&](const std::string& id) -> std::optional<MetaInfo> {
                return repository.get(id);
            },
            [&](const MetaInfo& meta_info) -> bool {
                auto installed_meta_info = system_database.get(meta_info.id);
                if (installed_meta_info == std::nullopt) return false;
                return installed_meta_info->cache == meta_info.cache;
            },
            [&](const MetaInfo& pkg) -> std::vector<std::string> {
                return pkg.depends;
            });
    std::vector<std::string> ids;

    for (auto const& meta_info : meta_infos) { ids.push_back(meta_info.id); }
    resolver.depends(ids, output_meta_infos);
}

MetaInfo Engine::get_remote_meta_info(const std::string& id) const {
    if (auto const meta_info = repository.get(id); meta_info) {
        return *meta_info;
    }
    throw std::runtime_error("no component found with id " + id);
}

std::map<std::string, MetaInfo> const& Engine::list_remote() const {
    return repository.get();
}

std::filesystem::path Engine::build(const Builder::BuildInfo& build_info,
        const std::optional<Container>& container) {

    std::filesystem::path work_dir = config.get<std::string>(
            "dir.build", std::filesystem::current_path() / "build");
    std::filesystem::path source_dir =
            config.get<std::string>("dir.sources", work_dir / "sources");
    std::filesystem::path packages_dir =
            config.get<std::string>("dir.packages", work_dir / "packages");

    auto build_root = work_dir / "build-root";
    auto install_root = work_dir / "install-root";

    auto package_path = packages_dir / build_info.package_name();

    for (auto const& path : {build_root, install_root}) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }

    for (auto const& path :
            {source_dir, packages_dir, build_root, install_root}) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }

    auto builder = Builder(config, build_info, container);
    auto subdir = builder.prepare_sources(source_dir, build_root);
    if (!subdir) subdir = ".";

    build_root /= build_info.config.get<std::string>(
            "build-dir", build_info.resolve(subdir->string(), config));
    build_root = build_info.resolve(build_root.string(), config);
    builder.compile_source(build_root, install_root);
    builder.pack(install_root, package_path);

    for (auto const& path : {build_root, install_root}) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
    }

    return package_path;
}

std::filesystem::path Engine::hash(const Builder::BuildInfo& build_info) {
    std::string hash_sum;

    {
        std::stringstream ss;
        ss << build_info.config.node;
        picosha2::hash256_hex_string(ss.str(), hash_sum);
    }

    for (auto const& dep : build_info.depends) {
        auto meta_info = this->repository.get(dep);
        if (!meta_info)
            throw std::runtime_error(
                    "missing runtime dependency '" + dep + "'");
        picosha2::hash256_hex_string(meta_info->str() + hash_sum, hash_sum);
    }

    for (auto const& dep : build_info.build_time_depends) {
        auto meta_info = this->repository.get(dep);
        if (!meta_info)
            throw std::runtime_error(
                    "missing build-time dependency '" + dep + "'");
        picosha2::hash256_hex_string(meta_info->str() + hash_sum, hash_sum);
    }
    return hash_sum;
}

void Engine::load_system_database() {
    system_database.load(config.get<std::string>(DIR_DATA, DEFAULT_DATA_DIR));
}
