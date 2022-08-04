#include "installer.hh"
using namespace rlxos::libpkgupd;

#include "../common.hh"
#include "../downloader.hh"
#include "../repository.hh"
#include "../resolver.hh"
#include "../triggerer.hh"
#include "appimage-installer/appimage-installer.hh"
#include "package-installer/package-installer.hh"

std::shared_ptr<Installer::Injector> Installer::Injector::create(
    PackageType package_type, Configuration* config) {
  switch (package_type) {
    case PackageType::APPIMAGE:
      return std::make_shared<AppImageInstaller>(config);
    case PackageType::RLXOS:
    case PackageType::PACKAGE:
      return std::make_shared<PackageInstaller>(config);
  }
  return nullptr;
}

bool Installer::install(
    std::vector<std::pair<std::string, PackageInfo*>> const& pkgs,
    SystemDatabase* sys_db) {
  std::vector<InstalledPackageInfo*> packages;

  std::filesystem::path root_dir =
      mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

  if (access(root_dir.c_str(), W_OK) != 0) {
    p_Error = "no write permission on root directory = '" + root_dir.string() +
              "', " + std::string(strerror(errno));
    return false;
  }
  for (auto const& pkg : pkgs) {
    std::filesystem::path package_path(pkg.first);
    if (!std::filesystem::exists(package_path)) {
      p_Error = "package file '" + package_path.string() + "' not exists";
      return false;
    }

    PROCESS(
        "installing " << pkg.second->id() << " " << pkg.second->version()
                      << (pkg.second->isDependency() ? " as dependency" : ""));

    auto injector = Injector::create(pkg.second->type(), mConfig);
    if (injector == nullptr) {
      p_Error = "no supported injector configured for '" +
                std::string(PACKAGE_TYPE_STR[int(pkg.second->type())]) + "'";
      return false;
    }

    std::vector<std::string> backups;
    mConfig->get("backup", backups);

    backups.insert(backups.end(), pkg.second->backups().begin(),
                   pkg.second->backups().end());

    if (!mConfig->get("no-backup", false)) {
      for (auto const& i : backups) {
        std::filesystem::path backup_path = root_dir / i;
        if (std::filesystem::exists(backup_path)) {
          std::error_code error;
          std::filesystem::copy(
              backup_path, (backup_path.string() + ".old"),
              std::filesystem::copy_options::recursive |
                  std::filesystem::copy_options::overwrite_existing,
              error);
          if (error) {
            p_Error = "failed to backup file " + backup_path.string();
            return false;
          }
        }
      }
    }

    std::vector<std::string> files;
    auto package_info = injector->inject(package_path.c_str(), files);
    if (package_info == nullptr) {
      p_Error = injector->error();
      return false;
    }

    if (!mConfig->get("no-backup", false)) {
      for (auto const& i : backups) {
        std::filesystem::path backup_path = root_dir / i;
        if (std::filesystem::exists(backup_path) &&
            std::filesystem::exists((backup_path.string() + ".old"))) {
          std::error_code error;
          std::filesystem::copy(
              backup_path, (backup_path.string() + ".new"),
              std::filesystem::copy_options::recursive |
                  std::filesystem::copy_options::overwrite_existing,
              error);
          if (error) {
            p_Error = "failed to add new backup file " + backup_path.string();
            return false;
          }

          std::filesystem::rename((backup_path.string() + ".old"), backup_path,
                                  error);
          if (error) {
            p_Error = "failed to recover backup file " + backup_path.string() +
                      ", " + error.message();
            return false;
          }
        }
      }
    }

    bool is_dependency = pkg.second->isDependency();

    auto old_package_info = sys_db->get(package_info->id().c_str());
    if (old_package_info != nullptr) {
      is_dependency = old_package_info->isDependency();
      PROCESS("cleaning old packages")
      auto old_files = old_package_info->files();
      for (auto i = old_files.rbegin(); i != old_files.rend(); ++i) {
        std::string file = *i;
        if (file.length()) {
          file = file.substr(2);
        }
        if (std::filesystem::exists(root_dir / file) &&
            std::find(files.begin(), files.end(), "./" + file) == files.end()) {
          if (file.find("./bin", 0) == 0 || file.find("./lib", 0) == 0 ||
              file.find("./sbin", 0) == 0) {
            continue;
          }

          std::error_code error;

          DEBUG("removing " << root_dir / file)
          std::filesystem::remove(root_dir / file, error);
          if (error) ERROR("failed to remove " << file);
        }
      }
    }

    auto installed_package_info =
        sys_db->add(package_info.get(), files,
                    mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR),
                    false, is_dependency);
    if (installed_package_info == nullptr) {
      p_Error = "failed to register '" + package_info->id() + "', " +
                sys_db->error();
      return false;
    }
    packages.push_back(installed_package_info);

  }

  if (!mConfig->get("installer.triggers", true)) {
    INFO("skipping triggers");
    std::cout << BOLD("successfully") << " "
              << BLUE((mConfig->get("is-updating", false) ? "installed"
                                                          : "updated"))
              << " " << GREEN(packages.size()) << BOLD(" package(s)")
              << std::endl;
    return true;
  }

  Triggerer triggerer;
  if (!triggerer.trigger(packages)) {
    p_Error = triggerer.error();
    return false;
  }

  std::cout << BOLD("successfully") << " "
            << BLUE((mConfig->get("is-updating", false) ? "installed"
                                                        : "updated"))
            << " " << GREEN(packages.size()) << BOLD(" package(s)")
            << std::endl;
  return true;
}

bool Installer::install(std::vector<PackageInfo*> pkgs, Repository* repository,
                        SystemDatabase* system_database) {
  std::vector<std::pair<std::string, PackageInfo*>> packages;
  Downloader downloader(mConfig);

  std::filesystem::path root_dir =
      mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  std::filesystem::path pkgs_dir =
      mConfig->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR);

  std::vector<PackageInfo*> required_packages, packages_to_install;

  if (mConfig->get("force", false) == false) {
    for (auto p : pkgs) {
      auto system_package = system_database->get(p->id().c_str());
      if (system_package != nullptr &&
          system_package->version() == p->version()) {
        INFO(p->id() << " is already installed and upto date");
        continue;
      }
      required_packages.push_back(p);
    }
  } else {
    required_packages = pkgs;
  }

  if (mConfig->get("installer.depends", true) == true) {
    auto resolver =
        Resolver(DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION);
    PROCESS("generating dependency graph");
    std::vector<PackageInfo*> dependencies;
    for (auto p : required_packages) {
      if (!resolver.depends(p, dependencies)) {
        p_Error = resolver.error();
        return false;
      }
    }

    auto skip_fun = DEFAULT_SKIP_PACKAGE_FUNCTION;

    for (auto const& p : required_packages) {
      auto iter = std::find_if(dependencies.begin(), dependencies.end(),
                               [&p](PackageInfo* pkginfo) -> bool {
                                 return p->id() == pkginfo->id();
                               });
      if (iter == dependencies.end()) {
        if (mConfig->get("force", false) == false) {
          if (!skip_fun(p)) dependencies.push_back(p);
        } else {
          dependencies.push_back(p);
        }
      } else {
        (*iter)->unsetDependency();
      }
    }

    if (dependencies.size()) {
      MESSAGE(BLUE("::"), "required " << dependencies.size() << " package(s)");
      for (auto const& i : dependencies) {
        std::cout << "- " << i->id() << ":" << i->version() << std::endl;
      }
      if (!ask_user("Do you want to continue", mConfig)) {
        p_Error = "user cancelled the operation";
        return false;
      }
    }

    packages_to_install = dependencies;
  } else {
    packages_to_install = required_packages;
  }

  if (packages_to_install.size() == 0) {
    INFO("no operation required");
    return true;
  }

  for (auto i : packages_to_install) {
    std::filesystem::path package_path =
        pkgs_dir / i->repository() / (PACKAGE_FILE(i));

    if (!std::filesystem::exists(package_path.parent_path())) {
      std::error_code err;
      std::filesystem::create_directories(package_path.parent_path(), err);
      if (err) {
        p_Error =
            "failed to create required directories '" + err.message() + "'";
        return false;
      }
    }

    if (std::filesystem::exists(package_path) &&
        !mConfig->get("download.force", false)) {
      INFO(i->id() << " found in cache");
    } else {
      PROCESS("downloading " << i->id());
      if (!downloader.get((i->repository() + "/" + (PACKAGE_FILE(i))).c_str(),
                          package_path.c_str())) {
        p_Error = "failed to get required file '" + PACKAGE_FILE(i);
        return false;
      }
    }

    packages.push_back({package_path, i});
  }
  return install(packages, system_database);
}