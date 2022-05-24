#include "../libpkgupd/archive-manager/archive-manager.hh"
#include "../libpkgupd/archive-manager/tarball/tarball.hh"
#include "../libpkgupd/configuration.hh"
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
    ERROR("no repositories specified");
    return 1;
  }

  for (auto const& i : repos) {
    PROCESS("generating meta information for " << GREEN(i));
    auto repo_path = pkgs_dir / i;
    if (!filesystem::exists(repo_path)) {
      ERROR("Error! '" << repo_path << "' not exists for " << i);
      return 1;
    }
    std::ofstream file(repo_path / "meta");
    file << "pkgs:" << std::endl;
    for (auto const& pkg : filesystem::directory_iterator(repo_path)) {
      if (pkg.path().filename().string() == "meta") continue;

      if (pkg.is_directory() || !pkg.path().has_extension()) {
        continue;
      }

      auto ext = pkg.path().extension().string().substr(1);
      auto package_type = PACKAGE_TYPE_FROM_STR(ext.c_str());
      if (package_type == PackageType::N_PACKAGE_TYPE) {
        ERROR("unsupported package '" << pkg.path());
        continue;
      }

      auto archive_manager = ArchiveManager::create(package_type);
      if (archive_manager == nullptr) {
        ERROR("Error! no supported archive manager for package type '"
              << PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(package_type)] << "'");
        continue;
      }

      auto package_info = archive_manager->info(pkg.path().c_str());
      if (package_info == nullptr) {
        ERROR("Error! failed to read package information from '"
              << pkg.path() << "', " << archive_manager->error());
        continue;
      }

      package_info->dump(file, true);
    }
    file.close();
  }

  return 0;
}