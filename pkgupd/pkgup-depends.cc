#include "../libpkgupd/repository.hh"
#include "../libpkgupd/resolver.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(depends) {
  os << "List all the dependent packages required" << endl;
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

  for (auto const& i : args) {
    auto pkginfo = repository.get(i.c_str());
    if (pkginfo == nullptr) {
      ERROR("Error! missing required package " << pkginfo);
      return 1;
    }

    if (!resolver.resolve(pkginfo)) {
      ERROR(resolver.error());
      return 1;
    }
  }

  for (auto const& i : resolver.list()) {
    cout << i->id() << endl;
  }
  return 0;
}