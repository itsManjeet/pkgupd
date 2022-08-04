#include "../libpkgupd/common.hh"
#include "../libpkgupd/installer/installer.hh"
#include "../libpkgupd/repository.hh"
#include "../libpkgupd/system-database.hh"
#include "../libpkgupd/utils/utils.hh"
using namespace rlxos::libpkgupd;

#include <filesystem>
namespace fs = std::filesystem;

#include <iostream>
using namespace std;

PKGUPD_MODULE(sync);

PKGUPD_MODULE_HELP(update) {
  os << "Update non-system packages of system" << endl
     << PADDING << " " << BOLD("Options:") << endl
     << PADDING << "  - system.packages=" << BOLD("<list>")
     << "    # Skip all system packages" << endl
     << PADDING << "  - update.exclude=" << BOLD("<list>")
     << "    # Specify package to exclude from update" << endl
     << endl;
}

PKGUPD_MODULE(update) {
  std::shared_ptr<SystemDatabase> system_database =
      std::make_shared<SystemDatabase>(config);

  std::shared_ptr<Repository> repository = std::make_shared<Repository>(config);
  std::shared_ptr<Installer> installer = std::make_shared<Installer>(config);

  std::vector<std::string> packages_id;
  std::vector<PackageInfo*> outdated_packages;
  std::vector<std::string> excludedPackages;

  // exclude all system image packages
  config->get("system.packages", excludedPackages);
  config->get("update.exclude", excludedPackages);

  if (int status = PKGUPD_sync({}, config); status != 0) {
    return status;
  }

  PROCESS("checking system updates");
  for (auto const& i : system_database->get()) {
    if (std::find(excludedPackages.begin(), excludedPackages.end(), i.first) !=
        excludedPackages.end()) {
      continue;
    }
    auto installed_info = i.second.get();
    auto repository_info = repository->get(i.first.c_str());
    if (repository_info == nullptr) {
      ERROR("missing repository information for " << i.first << ", skipping");
      continue;
    }

    if (installed_info->version() != repository_info->version()) {
      INFO("updates for " << installed_info->id() << " "
                          << installed_info->version() << "->"
                          << repository_info->version());
      outdated_packages.push_back(repository_info);
    }
  }

  if (outdated_packages.size() == 0) {
    cout << BOLD("system is upto date") << endl;
    return 0;
  }

  INFO("found " << outdated_packages.size() << " update(s)");
  if (!ask_user("Do you want to continue", config)) {
    ERROR("user cancelled the operation");
    return 1;
  }

  config->node()["force"] = true;
  config->node()["mode.ask"] = false;
  config->node()["is-updating"] = true;
  if (!installer->install(outdated_packages, repository.get(),
                          system_database.get())) {
    ERROR(installer->error());
    return 1;
  }

  cout << BOLD("successfully") << " " << BLUE("updated") << " "
       << GREEN(outdated_packages.size()) << BOLD(" package(s)") << endl;

  return 0;
}