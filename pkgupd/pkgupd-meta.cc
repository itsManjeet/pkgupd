#include "../libpkgupd/archive-manager/archive-manager.hh"
#include "../libpkgupd/archive-manager/tarball/tarball.hh"
#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/utils/utils.hh"
using namespace rlxos::libpkgupd;

#include <fstream>
using namespace std;

PKGUPD_MODULE_HELP(meta) {
  os << "generate repository meta information" << endl;
}

PKGUPD_MODULE(meta) {
  auto pkgs_dir = std::filesystem::path(
      config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR));
  std::vector<std::string> repos;
  config->get(REPOS, repos);

  if (repos.size() == 0) {
    cout << "Error! no repositories specified" << endl;
    return 1;
  }

  for (auto const& i : repos) {
    cout << ":: generating meta information for " << i << "::" << endl;
    auto repo_path = pkgs_dir / i;
    if (!filesystem::exists(repo_path)) {
      cerr << "Error! '" << repo_path << "' not exists for " << i << endl;
      return 1;
    }
    std::ofstream file(repo_path / "meta");
    file << "pkgs:" << std::endl;
    std::map<std::string, std::shared_ptr<PackageInfo>> pkgs;
    for (auto const& pkg : filesystem::directory_iterator(repo_path)) {
      if (pkg.path().filename().string() == "meta") continue;

      if (pkg.is_directory() || !pkg.path().has_extension()) {
        continue;
      }

      auto ext = pkg.path().extension().string().substr(1);
      auto package_type = PACKAGE_TYPE_FROM_STR(ext.c_str());
      if (package_type == PackageType::N_PACKAGE_TYPE) {
        cerr << "unsupported package '" << pkg.path() << endl;
        continue;
      }

      auto archive_manager = ArchiveManager::create(package_type);
      if (archive_manager == nullptr) {
        cerr << "Error! no supported archive manager for package type '"
             << PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(package_type)] << "'" << endl;
        return 1;
      }

      auto package_info = archive_manager->info(pkg.path().c_str());
      if (package_info == nullptr) {
        cerr << "Error! failed to read package information from '" << pkg.path()
             << "', " << archive_manager->error() << endl;
        return 1;
      }
      auto iter = pkgs.find(package_info->id());
      if (iter == pkgs.end()) {
        pkgs[package_info->id()] = package_info;
      } else {
        auto u = utils::get_version(package_info->version());
        auto v = utils::get_version(pkgs[package_info->id()]->version());
        if (u > v) {
          pkgs[package_info->id()] = package_info;
        }
      }
    }
    for (auto const& i : pkgs) {
      i.second->dump(file, true);
    }
    file.close();
  }

  return 0;
}