#include "../libpkgupd/repository.hh"
#include "../libpkgupd/system-database.hh"
#include "../libpkgupd/utils/utils.hh"
#include "common.hh"
using namespace rlxos::libpkgupd;

#include <filesystem>
namespace fs = std::filesystem;

#include <iostream>
using namespace std;

PKGUPD_MODULE(sync);
PKGUPD_MODULE(install);

PKGUPD_MODULE_HELP(update) { os << "perform the system updates" << endl; }

PKGUPD_MODULE(update) {
  std::shared_ptr<SystemDatabase> system_database =
      std::make_shared<SystemDatabase>(config);

  std::shared_ptr<Repository> repository = std::make_shared<Repository>(config);

  std::vector<std::string> packages_id;
  std::vector<std::string> outdated_packages;

  if (int status = PKGUPD_sync({}, config); status != 0) {
    return status;
  }

  PROCESS("checking system updates");
  if (args.size()) {
    packages_id = args;
  } else {
    if (!system_database->list_all(packages_id)) {
      ERROR(system_database->error());
      return 1;
    }
  }

  for (auto const& i : packages_id) {
    auto installed_info = system_database->get(i.c_str());
    if (installed_info == nullptr) {
      ERROR("missing installed package information for " << i << ", skipping");
      continue;
    }

    auto repository_info = repository->get(i.c_str());
    if (repository_info == nullptr) {
      ERROR("missing repository information for " << i << ", skipping");
      continue;
    }

    if (installed_info->version() != repository_info->version()) {
      INFO("updated for " << installed_info->id() << " "
                          << installed_info->version() << "->"
                          << repository_info->version());
      outdated_packages.push_back(installed_info->id());
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
  return PKGUPD_install(outdated_packages, config);
}