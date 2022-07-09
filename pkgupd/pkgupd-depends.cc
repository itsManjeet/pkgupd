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
  auto repository = Repository(config);
  auto system_database = SystemDatabase(config);

  bool list_all = config->get("depends.all", false);
  auto resolver = Resolver(
      [&](char const* id) -> std::shared_ptr<PackageInfo> {
        return repository.get(id);
      },
      [&](PackageInfo* pkginfo) -> bool {
        if (!list_all) {
          return system_database.get(pkginfo->id().c_str()) != nullptr;
        }
        return false;
      });

  std::vector<std::shared_ptr<PackageInfo>> packagesList;
  for (auto const& i : args) {
    if (!resolver.depends(i, packagesList)) {
      ERROR(resolver.error());
      return 1;
    }
  }

  for (auto const& i : packagesList) {
    cout << i->id() << endl;
  }
  return 0;
}