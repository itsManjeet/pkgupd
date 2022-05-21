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

  resolver = std::make_shared<Resolver>(
      [&](char const* id) -> std::shared_ptr<PackageInfo> {
        return repository->get(id);
      },
      [&](PackageInfo* info) -> bool {
        return system_database->get(info->id().c_str()) != nullptr;
      });

  std::vector<std::shared_ptr<PackageInfo>> pkgs;
  for (auto const& pkg_id : args) {
    auto pkg = repository->get(pkg_id.c_str());
    if (pkg == nullptr) {
      cerr << "Error! '" << pkg_id << "' is missing in repository, "
           << repository->error() << endl;
      return -1;
    }
    pkgs.push_back(pkg);
  }

  std::vector<std::shared_ptr<PackageInfo>> resolved_packages;

  if (config->get("no-depends", false) == false) {
    cout << ":: generating dependency tree ::" << endl;
    for (auto p : pkgs) {
      if (!resolver->resolve(p)) {
        cerr << resolver->error() << endl;
        return -1;
      }
    }
    pkgs = resolver->list();

    cout << "    found " << pkgs.size() << " dependencies" << endl;
    if (!config->get("mode.all-yes", false)) {
      cout << "Press [Y] if you want to contine: ";
      int c = cin.get();
      if (c != 'Y' && c != 'y') {
        cerr << "user cancelled the operation" << endl;
        return 1;
      }
    }
  }

  auto pkgs_dir =
      filesystem::path(config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR));

  for (auto p : pkgs) {
    cout << ":: installing " << p->id() << " " << p->version() << endl;
    installer = Installer::create(p->type(), config);
    if (installer == nullptr) {
      cerr << "Error! no valid installer avaliable for '" + p->id()
           << "' of type '" << PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(p->type())]
           << "'" << endl;
      return -1;
    }

    auto pkgfile = pkgs_dir / (PACKAGE_FILE(p));

    if (filesystem::exists(pkgfile)) {
      cout << "    package file found in cache" << endl;
    } else {
      cout << "    retrieving " << p->repository() << "/" << PACKAGE_FILE(p)
           << " " << endl;
      if (!downloader->get((p->repository() + "/" + PACKAGE_FILE(p)).c_str(),
                           pkgfile.c_str())) {
        cerr << "Error! failed to retrieve package file " << downloader->error()
             << endl;
        return -1;
      }
    }

    // TODO: add validator here

    cout << "    injecting package " << pkgfile << endl;
    auto installed_package_info = installer->install(
        pkgfile.c_str(), system_database.get(), config->get("force", false));
    if (installed_package_info == nullptr) {
      cerr << "Error! installation failed: " << installer->error() << endl;
      return -1;
    }

    cout << "    executing triggers" << endl;
    if (!triggerer->trigger(installed_package_info.get())) {
      cerr << "Error! failed to execute triggers" << endl;
      return -1;
    }
  }

  return 0;
}