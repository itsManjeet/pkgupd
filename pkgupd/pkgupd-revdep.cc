#include "libpkgupd/configuration.hh"
#include "libpkgupd/repository.hh"
#include "libpkgupd/resolver.hh"
#include "libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(revdep) {
  os << "List the reverse dependency of specified package" << std::endl;
}

PKGUPD_MODULE(revdep) {
  CHECK_ARGS(1);

  std::string parent = args[0];

  auto repository = std::make_shared<Repository>(config);
  auto system_database = std::make_shared<SystemDatabase>(config);

  std::vector<std::shared_ptr<PackageInfo>> packages;
  std::vector<std::string> all_packages;
  if (!repository->list_all(all_packages)) {
    ERROR("failed to list repository packages, " + repository->error());
    return 1;
  }

  std::function<bool(std::shared_ptr<PackageInfo> const&)>
      getReverseDependencies =
          [&](std::shared_ptr<PackageInfo> const& package) -> bool {
    if (std::find_if(
            packages.begin(), packages.end(),
            [&](std::shared_ptr<PackageInfo> const& package_info) -> bool {
              return package_info->id() == package->id();
            }) != packages.end()) {
      return true;
    }
    std::cout << "checking for " << package->id() << std::endl;

    for (auto const& a : all_packages) {
      auto package_info = repository->get(a.c_str());
      if (package_info == nullptr) {
        ERROR("missing required package '" + a + "'");
        return false;
      }
      if (std::find(package_info->depends().begin(),
                    package_info->depends().end(),
                    package->id()) != package_info->depends().end() &&
          std::find_if(
              packages.begin(), packages.end(),
              [&](std::shared_ptr<PackageInfo> const& package_info) -> bool {
                return package_info->id() == package->id();
              }) == packages.end()) {
        if (!getReverseDependencies(package_info)) {
          return false;
        }
      }
    }
    packages.push_back(package);
    return true;
  };

  PROCESS("searching reverse dependencies");
  auto package = repository->get(parent.c_str());
  if (package == nullptr) {
    ERROR("missing required package '" + parent + "'");
    return 0;
  }
  
  if (!getReverseDependencies(package)) {
    return 1;
  }

  for (auto const& i : packages) {
    std::cout << i->id() << std::endl;
  }

  return 0;
}