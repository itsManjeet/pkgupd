#include "repository.hh"

#include <filesystem>

#include "defines.hh"
namespace fs = std::filesystem;
namespace rlxos::libpkgupd {
Repository::Repository(Configuration *config) : mConfig{config} {
  mConfig->get(REPOS, repos_list);
  repo_dir = mConfig->get<std::string>(DIR_REPO, DEFAULT_REPO_DIR);
  init();
}

bool Repository::init() {
  mPackages.clear();

  PROCESS("reading repository database");
  for(auto const& repo : repos_list) {
    auto repo_path = fs::path(repo_dir) / repo;
    if (std::filesystem::exists(repo_path)) {
      try {
        YAML::Node node = YAML::LoadFile(repo_path);
        for(auto const& pkg_node :node["pkgs"]) {
          mPackages[pkg_node["id"].as<std::string>()] = std::make_unique<PackageInfo>(pkg_node, "");
        }
      } catch (std::exception const& exception) {
        std::cerr << "failed to read " << repo_path << ", " << exception.what() << std::endl;
      }
    }
  }
  return true;
}

PackageInfo* Repository::get(char const *packageName) {
  auto iter = mPackages.find(packageName);
  if (iter == mPackages.end()) {
    p_Error = "no found in ";
    return nullptr;
  }
  return iter->second.get();
}
}  // namespace rlxos::libpkgupd