#include "repository.hh"

#include <filesystem>

#include "defines.hh"
namespace fs = std::filesystem;
namespace rlxos::libpkgupd {
Repository::Repository(Configuration *config) : mConfig{config} {
  mConfig->get(REPOS, repos_list);
  repo_dir = mConfig->get<std::string>(DIR_REPO, DEFAULT_REPO_DIR);
}

bool Repository::list_all(std::vector<std::string> &ids) {
  for (auto const &repo : repos_list) {
    auto repo_path = fs::path(repo_dir) / repo;
    if (std::filesystem::exists(repo_path)) {
      try {
        YAML::Node node = YAML::LoadFile(repo_path);
        for (auto const &i : node["pkgs"]) {
          ids.push_back(i["id"].as<std::string>());
        }
      } catch (...) {
      }
    }
  }
  return true;
}
std::shared_ptr<PackageInfo> Repository::get(char const *packageName) {
  for (auto const &repo : repos_list) {
    auto repo_path = fs::path(repo_dir) / repo;
    if (!fs::exists(repo_path)) {
      p_Error = "repository data for '" + repo + "' not found";
      return nullptr;
    }

    YAML::Node node = YAML::LoadFile(repo_path);
    for (auto const &i : node["pkgs"]) {
      if (i["id"].as<std::string>() == packageName) {
        try {
          return std::make_shared<PackageInfo>(i, "");
        } catch (std::exception const &e) {
          p_Error = "failed to read package info for '" +
                    std::string(packageName) + "', " + std::string(e.what());
          return nullptr;
        }
      }
    }
  }

  p_Error = "no found in ";
  for (auto const &m : repos_list) {
    p_Error += m + " ";
  }
  return nullptr;
}
}  // namespace rlxos::libpkgupd