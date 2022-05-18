#include "repository.hh"

#include <filesystem>

#include "defines.hh"
namespace fs = std::filesystem;
namespace rlxos::libpkgupd {
std::shared_ptr<PackageInfo> Repository::get(char const *packageName) {
  std::vector<std::string> repos_list;
  repos(repos_list);

  auto repo_dir = mConfig->get<std::string>("repo_dir", DEFAULT_REPO_DIR);
  for (auto const &repo : repos_list) {
    auto repo_path = fs::path(repo_dir) / repo;
    if (!fs::exists(repo_path)) {
      throw std::runtime_error(repo_path.string() + " for " + repo +
                               " not exists");
    }

    YAML::Node node = YAML::LoadFile(repo_path);
    for (auto const &i : node["pkgs"]) {
      if (i["id"].as<std::string>() == packageName) {
        return std::make_shared<PackageInfo>(i, "");
      }
    }
  }
  
  p_Error = "no recipe file found for " + std::string(packageName);
  return nullptr;
}
}  // namespace rlxos::libpkgupd