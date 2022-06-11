#include "../libpkgupd/triggerer.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(trigger) {
  os << "Execute required triggers and create required users & groups" << endl;
}

PKGUPD_MODULE(trigger) {
  auto triggerer = std::make_shared<Triggerer>();
  auto system_database = std::make_shared<SystemDatabase>(config);
  std::vector<std::string> all_packages_id;
  system_database->list_all(all_packages_id);
  for (auto const& i : all_packages_id) {
    auto installed_info = system_database->get(i.c_str());
    if (installed_info == nullptr) {
      continue;
    }

    if (!triggerer->trigger({installed_info})) {
      cerr << "Error! " << triggerer->error() << endl;
    }
  }
  return 0;
}