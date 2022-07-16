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
  os << "Install package from repository" << endl
     << PADDING << " " << BOLD("Options:") << endl
     << PADDING << "  - installer.depends=" << BOLD("<bool>")
     << "   # Enable or disable dependency management" << endl
     << PADDING << "  - force=" << BOLD("<bool>")
     << "               # Force installation of already installed package"
     << endl
     << PADDING << "  - installer.triggers=" << BOLD("<bool>")
     << "  # Skip triggers during installation, also include user/group "
        "creation"
     << endl
     << PADDING << "  - downloader.force=" << BOLD("<bool>")
     << "    # Force redownload of specified package" << endl
     << endl;
  ;
}

PKGUPD_MODULE(install) {
  std::shared_ptr<Installer> installer;
  std::shared_ptr<SystemDatabase> system_database;
  std::shared_ptr<Repository> repository;

  system_database = std::make_shared<SystemDatabase>(config);
  repository = std::make_shared<Repository>(config);
  installer = std::make_shared<Installer>(config);

  std::vector<std::shared_ptr<PackageInfo>> packages;
  for (auto const& i : args) {
    auto package_info = repository->get(i.c_str());
    if (package_info == nullptr) {
      ERROR("required package '" << i << "' not found");
      return 1;
    }
    auto system_package = system_database->get(i.c_str());
    if (system_package != nullptr &&
        system_package->version() == package_info->version()) {
      continue;
    }
    packages.push_back(package_info);
  }

  if (!installer->install(packages, repository.get(), system_database.get())) {
    ERROR(installer->error());
    return 1;
  }

  return 0;
}