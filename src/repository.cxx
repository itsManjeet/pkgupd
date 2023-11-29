#include "repository.hxx"

#include <algorithm>
#include <filesystem>

#include "defines.hxx"

namespace fs = std::filesystem;

namespace rlxos::libpkgupd {
    Repository::Repository(Configuration* config) : mConfig{config} {
        mConfig->get(REPOS, repos_list);
        repo_dir = mConfig->get<std::string>(DIR_REPO, DEFAULT_REPO_DIR);
        init();
    }

    void Repository::init() {
        mPackages.clear();

        PROCESS("Reading Repository Database");
        DEBUG("LOCATION   " << repo_dir);
        for (auto const& repo: repos_list) {
            auto repo_path = fs::path(repo_dir) / repo / mConfig->get<std::string>("server.stability", "testing");
            if (std::filesystem::exists(repo_path)) {
                DEBUG("REPOSITORY " << repo);
                try {
                    YAML::Node node = YAML::LoadFile(repo_path);
                    for (auto const& pkg_node: node["pkgs"]) {
                        mPackages[pkg_node["id"].as<std::string>()] = MetaInfo(pkg_node);
                    }
                } catch (std::exception const& exception) {
                    std::cerr << "failed to read " << repo_path << ", " << exception.what()
                            << std::endl;
                }
            } else {
                DEBUG("MISSING DATA FOR " << repo);
            }
        }

        DEBUG("DATABASE SIZE " << mPackages.size());
    }

    std::optional<MetaInfo> Repository::get(const std::string& id) {
        if (auto const iter = mPackages.find(id); iter != mPackages.end()) {
            return iter->second;
        }
        return std::nullopt;
    }
} // namespace rlxos::libpkgupd
