#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/triggerer.hh"
#include "../libpkgupd/uninstaller/uninstaller.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(remove) { os << "remove package from system" << endl; }

PKGUPD_MODULE(remove) {
  CHECK_ARGS(1);

  auto pkg_id = args[0];
  auto system_database = std::make_shared<SystemDatabase>(config);

  auto pkg_info = system_database->get(pkg_id.c_str());
  if (pkg_info == nullptr) {
    cerr << "Error! no package found with id '" + pkg_id << "'" << endl;
    return 1;
  }

  auto uninstaller = std::make_shared<Uninstaller>(config);
  if (!uninstaller->uninstall(pkg_info.get(), system_database.get())) {
    cerr << "Error! failed to remove '" + pkg_id + "' " << uninstaller->error()
         << endl;
    return 1;
  }

  if (!config->get(SKIP_TRIGGERS, false)) {
    auto triggerer = std::make_shared<Triggerer>();
    if (!triggerer->trigger(pkg_info.get())) {
      cerr << "Error! failed to execute post removal triggers '"
           << triggerer->error() << endl;
      return 1;
    }
  }

  return 0;
}