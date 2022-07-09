#include "../libpkgupd/common.hh"
#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/triggerer.hh"
#include "../libpkgupd/uninstaller/uninstaller.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(remove) {
  os << "Remove package from system" << endl
     << PADDING << " " << BOLD("Options:") << endl
     << PADDING << "  - system.packages=" << BOLD("<list>")
     << "    # Skip System packages for removal" << endl
     << endl;
}

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

  if (!ask_user("do you want to remove " + pkg_info->id(), config)) {
    ERROR("User cancelled the operation");
    return 1;
  }

  PROCESS("removing " << GREEN(pkg_info->id()) << ":"
                      << BLUE(pkg_info->version()));
  auto uninstaller = std::make_shared<Uninstaller>(config);
  if (!uninstaller->uninstall(pkg_info.get(), system_database.get())) {
    ERROR("failed to remove " << RED(pkg_id) << " " << uninstaller->error());
    return 1;
  }

  PROCESS("calculating unneeded packages");
  std::vector<std::string> allpackages;
  if (!system_database->list_all(allpackages)) {
    ERROR("failed to list all packages " << system_database->error());
    return 1;
  }

  std::vector<std::shared_ptr<InstalledPackageInfo>> packagesToRemove;

  auto is_required = [&](std::shared_ptr<PackageInfo> const& pkg) -> bool {
    // std::cout << "    checking requirement of " << pkg->id() << std::endl;
    for (auto const& i : allpackages) {
      auto pkginfo = system_database->get(i.c_str());
      if (pkginfo == nullptr) continue;
      if (pkginfo->isDependency()) continue;

      if (std::find(pkginfo->depends().begin(), pkginfo->depends().end(),
                    pkg->id()) != pkginfo->depends().end()) {
        // std::cout << "    " << pkg->id() << " is required by " <<
        // pkginfo->id()
        // << std::endl;
        return true;
      }
    }
    return false;
  };

  for (auto const& i : allpackages) {
    // std::cout << "checking " << i << std::endl;
    auto pkginfo = system_database->get(i.c_str());
    if (pkginfo == nullptr) {
      ERROR("internal error, failed to read package information for " << i);
      continue;
    }
    if (!pkginfo->isDependency()) {
      // std::cout << "  " << i << " is not a dependency" << std::endl;
      continue;
    }
    if (!is_required(pkginfo)) {
      // std::cout << "  " << i << " is not required" << std::endl;
      packagesToRemove.push_back(
          std::dynamic_pointer_cast<InstalledPackageInfo>(pkginfo));
    }
  }

  if (packagesToRemove.size()) {
    for (auto const& i : packagesToRemove) {
      std::cout << " - " << i->id() << std::endl;
    }

    if (!ask_user("do you want to remove unneeded dependencies", config)) {
      INFO("no unneeded dependencies removed");
      packagesToRemove.clear();
    } else {
      for (auto const& i : packagesToRemove) {
        PROCESS("removing " << GREEN(i->id()) << ":" << BLUE(i->version()));
        if (!uninstaller->uninstall(i.get(), system_database.get())) {
          ERROR("failed to remove " << i->id() << ", " << uninstaller->error());
        }
      }
    }
  }

  packagesToRemove.push_back(pkg_info);

  if (!config->get(SKIP_TRIGGERS, false)) {
    auto triggerer = std::make_shared<Triggerer>();
    if (!triggerer->trigger(packagesToRemove)) {
      ERROR("failed to execute post removal triggers '" << triggerer->error());
      return 1;
    }
  }

  return 0;
}