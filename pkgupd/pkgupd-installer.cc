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

  system_database = std::make_shared<SystemDatabase>(config);
  repository = std::make_shared<Repository>(config);
  installer = std::make_shared<Installer>(config);

  std::vector<std::shared_ptr<PackageInfo>> packages;
  for (auto const& i : args) {
    auto package_info = repository->get(i.c_str());
    if (package_info == nullptr) {
      ERROR("required package '" << package_info << "' not found");
      return 1;
    }
    packages.push_back(package_info);
  }

  if (!installer->install(packages, repository.get(), system_database.get())) {
    ERROR(installer->error());
    return 1;
  }

  cout << BOLD("successfully") << " "
       << BLUE((config->get("is-updating", false) ? "installed" : "updated"))
       << " " << GREEN(packages.size()) << BOLD(" package(s)") << endl;

  return 0;
}