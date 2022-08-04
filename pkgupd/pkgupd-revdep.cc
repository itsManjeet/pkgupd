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

  // auto repository = std::make_shared<Repository>(config);
  auto system_database = SystemDatabase(config);
  for (auto const& i : system_database.get()) {
    if (std::find(i.second->depends().begin(), i.second->depends().end(),
                  parent) != i.second->depends().end()) {
      std::cout << "- " << i.second->id() << std::endl;
    }
  }
  return 0;
}