#include <bits/stdc++.h>
using namespace std;

#include "../libpkgupd/downloader.hh"
#include "../libpkgupd/installer/installer.hh"
#include "../libpkgupd/repository.hh"
#include "../libpkgupd/resolver.hh"
#include "../libpkgupd/system-database.hh"
#include "../libpkgupd/triggerer.hh"

using namespace rlxos::libpkgupd;

PKGUPD_MODULE_HELP(install) {
  os << "install specified package from the repository" << endl;
}

PKGUPD_MODULE(install) {
  std::shared_ptr<Installer> installer;
  std::shared_ptr<SystemDatabase> system_database;
  std::shared_ptr<Repository> repository;
  std::shared_ptr<Resolver> resolver;
  std::shared_ptr<Downloader> downloader;
  std::shared_ptr<Triggerer> triggerer;

  system_database = std::make_shared<SystemDatabase>(config);
  repository = std::make_shared<Repository>(config);

  downloader = std::make_shared<Downloader>(config);
  triggerer = std::make_shared<Triggerer>();

  resolver =
      std::make_shared<Resolver>(system_database.get(), repository.get());

  std::vector<std::shared_ptr<PackageInfo>> pkgs;
  for (auto const& pkg_id : args) {
    auto installed_pkg = system_database->get(pkg_id.c_str());
    if (installed_pkg != nullptr && !config->get("force", false)) {
      continue;
    }
    auto pkg = repository->get(pkg_id.c_str());
    if (pkg == nullptr) {
      ERROR("no package " << RED(pkg_id)
                          << " found in repository database.\n  Because "
                          << repository->error() << ", try syncing again");
      return -1;
    }
    pkgs.push_back(pkg);
  }

  if (pkgs.size() == 0) {
    MESSAGE(BLUE("::"), "package(s) is/are already installed");
    return 0;
  }

  std::vector<std::shared_ptr<PackageInfo>> resolved_packages;

  if (config->get("no-depends", false) == false) {
    PROCESS("generating dependency graph");
    for (auto p : pkgs) {
      if (!resolver->resolve(p)) {
        cerr << resolver->error() << endl;
        return -1;
      }
    }
    if (resolver->list().size() == 0 && config->get("force", false)) {
    } else {
      pkgs = resolver->list();
    }

    MESSAGE(BLUE("::"), "found " << pkgs.size() << " dependencies");
    if (!config->get("mode.all-yes", false)) {
      cout << BOLD("Press [Y] if you want to contine: ");
      int c = cin.get();
      if (c != 'Y' && c != 'y') {
        ERROR("user cancelled the operation")
        return 1;
      }
    }
  }

  auto pkgs_dir =
      filesystem::path(config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR));

  for (auto p : pkgs) {
    PROCESS("installing " << p->id() << "-" << p->version());
    installer = Installer::create(p->type(), config);
    if (installer == nullptr) {
      ERROR("Error! no valid installer avaliable for '" + p->id()
            << "' of type '" << PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(p->type())]
            << "'");
      return -1;
    }

    auto pkgfile = pkgs_dir / (PACKAGE_FILE(p));

    if (filesystem::exists(pkgfile)) {
    } else {
      if (!downloader->get((p->repository() + "/" + PACKAGE_FILE(p)).c_str(),
                           pkgfile.c_str())) {
        ERROR("Error! failed to retrieve package file " << downloader->error());
        return -1;
      }
    }

    // TODO: add validator here

    auto installed_package_info = installer->install(
        pkgfile.c_str(), system_database.get(), config->get("force", false));
    if (installed_package_info == nullptr) {
      ERROR("Error! installation failed: " << installer->error())
      return -1;
    }

    if (config->get("installer.triggers", true)) {
      if (!triggerer->trigger(installed_package_info.get())) {
        ERROR("Error! failed to execute triggers");
        return -1;
      }
    }
  }

  return 0;
}