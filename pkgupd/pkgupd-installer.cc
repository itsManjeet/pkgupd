#include <bits/stdc++.h>
using namespace std;

#include "../libpkgupd/downloader.hh"
#include "../libpkgupd/installer/installer.hh"
#include "../libpkgupd/repository.hh"
#include "../libpkgupd/resolver.hh"
#include "../libpkgupd/system-database.hh"
#include "../libpkgupd/triggerer.hh"
#include "common.hh"

using namespace rlxos::libpkgupd;

PKGUPD_MODULE_HELP(install) {
  os << "install specified package from the repository" << endl;
}

PKGUPD_MODULE(install) {
  std::shared_ptr<Installer> installer;
  std::shared_ptr<SystemDatabase> system_database;
  std::shared_ptr<Repository> repository;
  std::shared_ptr<Resolver> resolver;
  std::shared_ptr<Downloader> downloader;
  std::shared_ptr<Triggerer> triggerer;

  system_database = std::make_shared<SystemDatabase>(config);
  repository = std::make_shared<Repository>(config);

  downloader = std::make_shared<Downloader>(config);
  triggerer = std::make_shared<Triggerer>();

  resolver = std::make_shared<Resolver>(DEFAULT_GET_PACKAE_FUNCTION,
                                        DEFAULT_SKIP_PACKAGE_FUNCTION);

  std::vector<std::shared_ptr<PackageInfo>> pkgs;
  for (auto const& pkg_id : args) {
    auto installed_pkg = system_database->get(pkg_id.c_str());
    if (installed_pkg != nullptr && !config->get("force", false)) {
      continue;
    }
    auto pkg = repository->get(pkg_id.c_str());
    if (pkg == nullptr) {
      ERROR("no package " << RED(pkg_id)
                          << " found in repository database.\n  Because "
                          << repository->error() << ", try syncing again");
      return -1;
    }
    pkgs.push_back(pkg);
  }

  if (pkgs.size() == 0) {
    MESSAGE(BLUE("::"), "package(s) is/are already installed");
    return 0;
  }

  std::vector<std::shared_ptr<PackageInfo>> resolved_packages;

  if (config->get("no-depends", false) == false) {
    PROCESS("generating dependency graph");
    for (auto p : pkgs) {
      if (!resolver->resolve(p)) {
        cerr << resolver->error() << endl;
        return -1;
      }
    }
    if (resolver->list().size() == 0 && config->get("force", false)) {
    } else {
      pkgs = resolver->list();
    }

    MESSAGE(BLUE("::"), "required " << pkgs.size() << " packagess");
    for (auto const& i : pkgs) {
      cout << "- " << i->id() << ":" << i->version() << endl;
    }
    if (!ask_user("Do you want to continue", config)) {
      ERROR("user cancelled the operation");
      return 1;
    }
  }

  auto pkgs_dir =
      filesystem::path(config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR));

  for (auto p : pkgs) {
    PROCESS((config->get("is-updating", false) ? "updating" : "installing")
            << " " << p->id() << "-" << p->version());
    installer = Installer::create(p->type(), config);
    if (installer == nullptr) {
      ERROR("Error! no valid installer avaliable for '" + p->id()
            << "' of type '" << PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(p->type())]
            << "'");
      return -1;
    }

    auto pkgfile = pkgs_dir / p->repository() / (PACKAGE_FILE(p));
    if (!std::filesystem::exists(pkgfile.parent_path())) {
      std::error_code error;
      std::filesystem::create_directories(pkgfile.parent_path(), error);
      if (error) {
        cerr << "Error! failed to create required package directories" << endl;
        return -1;
      }
    }

    if (filesystem::exists(pkgfile)) {
    } else {
      if (!downloader->get((p->repository() + "/" + PACKAGE_FILE(p)).c_str(),
                           pkgfile.c_str())) {
        ERROR("Error! failed to retrieve package file " << downloader->error());
        return -1;
      }
    }

    // TODO: add validator here

    auto installed_package_info = installer->install(
        pkgfile.c_str(), system_database.get(), config->get("force", false));
    if (installed_package_info == nullptr) {
      ERROR("Error! installation failed: " << installer->error())
      return -1;
    }

    if (config->get("installer.triggers", true)) {
      if (!triggerer->trigger(installed_package_info.get())) {
        ERROR("Error! failed to execute triggers");
        return -1;
      }

      if (installed_package_info->script().size()) {
        if (int status = Executor::execute(installed_package_info->script());
            status != 0) {
          ERROR("install script failed with exit code: " << status);
        }
      }
    }
  }

  cout << BOLD("successfully") << " "
       << BLUE((config->get("is-updating", false) ? "installed" : "updated"))
       << " " << GREEN(pkgs.size()) << BOLD(" package(s)") << endl;

  return 0;
}