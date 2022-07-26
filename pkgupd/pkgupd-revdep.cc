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
  auto system_database = std::make_shared<SystemDatabase>(config);

  std::vector<std::string> packages;

  std::function<bool(std::string const& id, std::vector<std::string>& list)>

      get_reverse_dependency =
          [&](std::string const& id, std::vector<std::string>& list) -> bool {
    for (auto const& i : system_database->get()) {
      std::cout << "checking " << i.first << std::endl;
      auto iter = std::find_if(
          i.second->depends().begin(), i.second->depends().end(),
          [&](std::string const& dep_id) -> bool { return dep_id == id; });
      if (iter == i.second->depends().end()) {
        continue;
      }
      if (std::find_if(list.begin(), list.end(),
                       [&](std::string const& pkgid) -> bool {
                         return pkgid == (*iter);
                       }) == list.end()) {
        list.push_back(*iter);
        get_reverse_dependency(*iter, list);
      };
    }
    return true;
  };

  get_reverse_dependency(parent, packages);
  for (auto const& i : packages) {
    std::cout << "-> " << i << std::endl;
  }

  return 0;
}