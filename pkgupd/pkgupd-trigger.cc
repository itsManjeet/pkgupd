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
  for (auto const& i : system_database->get()) {
    auto installed_info = i.second.get();
    if (installed_info == nullptr) {
      continue;
    }

    if (!triggerer->trigger({installed_info})) {
      cerr << "Error! " << triggerer->error() << endl;
    }
  }
  return 0;
}