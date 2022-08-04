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
  std::vector<InstalledPackageInfo*> packages;
  for (auto const& i : system_database->get()) {
    packages.push_back(i.second.get());
  }
  if (!triggerer->trigger(packages)) {
    cerr << "Error! " << triggerer->error() << endl;
  }
  return 0;
}