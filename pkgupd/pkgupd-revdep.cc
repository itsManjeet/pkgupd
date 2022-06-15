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

  PROCESS("searching reverse dependencies");
  for (auto const& package_id : all_packages) {
    auto package = repository->get(package_id.c_str());
    if (package == nullptr) {
      ERROR(repository->error());
      return 1;
    }

    if (std::find(package->depends().begin(), package->depends().end(),
                  parent) != package->depends().end()) {
      packages.push_back(package);
    } else {
      auto resolver = std::make_shared<Resolver>(
          DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION, parent);
      if (!resolver->resolve(package)) {
        ERROR(resolver->error());
        return 1;
      }
      if (resolver->found()) {
        packages.push_back(package);
      }
    }
  }

  for (auto const& i : packages) {
    std::cout << i->id() << std::endl;
  }

  return 0;
}