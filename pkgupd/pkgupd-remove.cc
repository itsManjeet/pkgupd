#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/triggerer.hh"
#include "../libpkgupd/uninstaller/uninstaller.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(remove) { os << "remove package from system" << endl; }

PKGUPD_MODULE(remove) {
  CHECK_ARGS(1);

  auto pkg_id = args[0];

  std::vector<std::string> excludedPackages;

  // exclude all system image packages
  config->get("system.packages", excludedPackages);
  if (std::find(excludedPackages.begin(), excludedPackages.end(), pkg_id) !=
      excludedPackages.end()) {
    ERROR("can't remove package from system image");
    return 1;
  }

  auto system_database = std::make_shared<SystemDatabase>(config);

  auto pkg_info = system_database->get(pkg_id.c_str());
  if (pkg_info == nullptr) {
    ERROR("no package with name " << RED(pkg_id)
                                  << " is installed in the system");
    return 1;
  }

  PROCESS("removing " << GREEN(pkg_info->id()) << ":"
                      << BLUE(pkg_info->version()));
  auto uninstaller = std::make_shared<Uninstaller>(config);
  if (!uninstaller->uninstall(pkg_info.get(), system_database.get())) {
    ERROR("failed to remove " << RED(pkg_id) << " " << uninstaller->error());
    return 1;
  }

  if (!config->get(SKIP_TRIGGERS, false)) {
    auto triggerer = std::make_shared<Triggerer>();
    if (!triggerer->trigger({pkg_info})) {
      ERROR("failed to execute post removal triggers '" << triggerer->error());
      return 1;
    }
  }

  return 0;
}