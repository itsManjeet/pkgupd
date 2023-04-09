#include "../libpkgupd/repository.hh"
#include "../libpkgupd/resolver.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(depends) {
  os << "List all the dependent packages required" << endl
     << PADDING << " " << BOLD("Options:") << endl
     << PADDING << "  - depends.all=" << BOLD("<bool>")
     << "    # List all dependent packages including already installed packages"
     << endl
     << endl;
}

PKGUPD_MODULE(depends) {
  auto repository = std::make_shared<Repository>(config);
  auto system_database = std::make_shared<SystemDatabase>(config);

  bool list_all = config->get("depends.all", false);
  auto resolver =
      Resolver<PackageInfo *>(DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION,
               DEFAULT_DEPENDS_FUNCTION);

  std::vector<PackageInfo*> packagesList;
  for (auto const& i : args) {
    if (!resolver.depends(i, packagesList)) {
      ERROR(resolver.error());
      return 1;
    }
  }

  for (auto const& i : args) {
    if (std::find_if(packagesList.begin(), packagesList.end(),
                     [&](PackageInfo* pkginfo) -> bool {
                       return pkginfo->id() == i;
                     }) == packagesList.end()) {
      packagesList.push_back(repository->get(i.c_str()));
    }
  }

  for (auto const& i : packagesList) {
    cout << i->id() << endl;
  }
  return 0;
}