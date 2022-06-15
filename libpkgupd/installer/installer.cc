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

bool Installer::install(std::vector<std::string> pkgs, SystemDatabase* sys_db) {
  std::vector<std::shared_ptr<InstalledPackageInfo>> packages;

  std::filesystem::path root_dir =
      mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

  if (access(root_dir.c_str(), W_OK) != 0) {
    p_Error = "no write permission on root directory = '" + root_dir.string() +
              "', " + std::string(strerror(errno));
    return false;
  }
  for (auto const& pkg : pkgs) {
    std::filesystem::path package_path(pkg);
    if (!std::filesystem::exists(pkg)) {
      p_Error = "package file '" + pkg + "' not exists";
      return false;
    }

    if (!package_path.has_extension()) {
      p_Error = "invalid package id '" + pkg + "' no extension";
      return false;
    }

    auto ext = package_path.extension().string().substr(1);
    auto package_type = PACKAGE_TYPE_FROM_STR(ext.c_str());
    if (package_type == PackageType::N_PACKAGE_TYPE) {
      p_Error = "no support package type found for '" + ext + "'";
      return false;
    }

    PROCESS("installing " << package_path.filename().string());
    auto injector = Injector::create(package_type, mConfig);
    if (injector == nullptr) {
      p_Error = "no supported injector configured for '" + ext + "'";
      return false;
    }

    std::vector<std::string> files;
    auto package_info = injector->inject(package_path.c_str(), files);
    if (package_info == nullptr) {
      p_Error = injector->error();
      return false;
    }

    auto old_package_info = sys_db->get(package_info->id().c_str());
    if (old_package_info != nullptr) {
      std::cout << "Found old package info" << std::endl;
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
                    mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR));
    if (installed_package_info == nullptr) {
      p_Error =
          "failed to register '" + package_info->id() + "', " + sys_db->error();
      return false;
    }
    packages.push_back(installed_package_info);
  }

  if (!mConfig->get("installer.triggers", true)) {
    INFO("skipping triggers");
    return true;
  }

  Triggerer triggerer;
  return triggerer.trigger(packages);
}

bool Installer::install(std::vector<std::shared_ptr<PackageInfo>> pkgs,
                        Repository* repository,
                        SystemDatabase* system_database) {
  std::vector<std::string> packages;
  auto resolver = std::make_shared<Resolver>(DEFAULT_GET_PACKAE_FUNCTION,
                                             DEFAULT_SKIP_PACKAGE_FUNCTION);
  Downloader downloader(mConfig);

  std::filesystem::path root_dir =
      mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  std::filesystem::path pkgs_dir =
      mConfig->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR);

  if (mConfig->get("installer.depends", false) == false) {
    PROCESS("generating dependency graph");
    for (auto p : pkgs) {
      if (!resolver->resolve(p)) {
        p_Error = resolver->error();
        return false;
      }
    }
    if (resolver->list().size() == 0 && mConfig->get("force", false)) {
    } else {
      pkgs = resolver->list();
    }

    MESSAGE(BLUE("::"), "required " << pkgs.size() << " packagess");
    for (auto const& i : pkgs) {
      std::cout << "- " << i->id() << ":" << i->version() << std::endl;
    }
    if (!ask_user("Do you want to continue", mConfig)) {
      ERROR("user cancelled the operation");
      return 1;
    }
  }

  for (auto const& i : pkgs) {
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

    packages.push_back(package_path);
  }
  return install(packages, system_database);
}