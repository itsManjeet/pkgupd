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
  auto repository = Repository(config);
  auto package = repository.get(args[0].c_str());
  if (package == nullptr) {
    ERROR("missing required package " << args[0]);
    return 1;
  }
  for (auto const& i : repository.get()) {
    if (find_if(i.second->depends().begin(), i.second->depends().end(),
                [&](std::string id) -> bool {
                  return id == package->id();
                }) != i.second->depends().end()) {
      std::cout << i.first << std::endl;
    }
  }

  return 0;
}