#include "installer.hh"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "exec.hh"
#include "image.hh"
#include "packager.hh"
#include "tar.hh"
#include "triggerer.hh"

namespace rlxos::libpkgupd {

bool Installer::_install(std::vector<std::string> const &packages,
                         std::string const &rootDir, bool isSkipTriggers,
                         bool isForce) {
  std::vector<Package> packagesList;
  std::vector<std::vector<std::string>> packagesFiles;

  for (auto const &i : packages) {
    std::string ext = std::filesystem::path(i).extension().string();
    ext = ext.substr(1, ext.length() - 1);

    auto packager = Packager::create(stringToPackageType(ext), i);

    DEBUG("getting information from "
          << std::filesystem::path(i).filename().string());

    auto info = packager->info();
    if (!info) {
      p_Error = packager->error();
      return false;
    }

    try {
      if (!isForce) {
        if (m_SystemDatabase.isInstalled(*info) &&
            !m_SystemDatabase.isOutDated(*info)) {
          INFO(info->id() + " latest version is already installed")
          continue;
        }
      }
    } catch (...) {
    }

    std::vector<std::string> old_files;
    if (m_SystemDatabase.isInstalled(*info)) {
      auto old_info = m_SystemDatabase[info->id()];
      for (auto const &i : old_info->node()["files"]) {
        old_files.push_back(i.as<std::string>());
      }
    }

    if (rootDir == "/") {
      PROCESS("installing " << BLUE(info->id()));
    } else {
      PROCESS("installing " << BLUE(info->id()) << " into " << RED(rootDir));
    }

    if (!packager->extract(rootDir)) {
      p_Error = packager->error();
      return false;
    }
    auto packageFile = packager->list();
    packagesFiles.push_back(packageFile);
    packagesList.push_back(*info);

    std::vector<std::string> deprecated_files;
    for (auto const &file : old_files) {
      if (std::find(packageFile.begin(), packageFile.end(), file) ==
          packageFile.end()) {
        DEBUG("deprecated file '" << file << "'");
        deprecated_files.push_back(file);
      }
    }

    if (deprecated_files.size()) {
      PROCESS("found '" << deprecated_files.size() << "' extra files");
      PROCESS("cleaning old files");
      for (auto file = deprecated_files.rbegin();
           file != deprecated_files.rend(); file++) {
        std::error_code err;
        std::string filepath = *file;
        if (filepath[0] == '.') {
          filepath = filepath.substr(1, filepath.length() - 1);
        }

        // Skip symbolic links update
        if (filepath.find("/usr/sbin", 0) == 0 ||
            filepath.find("/sbin", 0) == 0 || filepath.find("/bin", 0) == 0 ||
            filepath.find("/lib64", 0) == 0 ||
            filepath.find("/usr/lib64", 0) == 0) {
          DEBUG("skipping " << filepath);
          continue;
        }
        std::filesystem::remove_all(filepath, err);
        if (err) {
          ERROR("failed to clean old file '" << *file << "'");
        }
      }
    }

    if (info->script().length()) {
      if (isSkipTriggers) {
        INFO("skipped install script")
      } else {
        PROCESS("executing install script");
        auto env = std::vector<std::string>();
        env.push_back("VERSION=" + info->version());
        if (Executor().execute(info->script(), "/", env) != 0) {
          ERROR("failed to execute install script");
        }
      }
    }
  }

  bool withPkgname = packagesList.size() != 1;

  for (int i = 0; i < packagesList.size(); i++) {
    if (withPkgname) {
      PROCESS("registering into database " << BLUE(packagesList[i].id()));
    } else {
      PROCESS("registering into database");
    }

    if (!m_SystemDatabase.registerIntoSystem(packagesList[i], packagesFiles[i],
                                             rootDir, isForce)) {
      p_Error = m_SystemDatabase.error();
      return false;
    }
  }

  if (isSkipTriggers) {
    INFO("skipping triggers")
  } else {
    auto triggerer_ = Triggerer();
    if (!triggerer_.trigger(packagesFiles)) {
      p_Error = triggerer_.error();
      return false;
    }
  }

  if (isSkipTriggers) {
    INFO("skipped creating users account")
  } else {
    auto triggerer_ = Triggerer();
    if (!triggerer_.trigger(packagesList)) {
      p_Error = triggerer_.error();
      return false;
    }
  }

  return true;
}
bool Installer::install(std::vector<std::string> const &packages,
                        std::string const &rootDir, bool isSkipTriggers,
                        bool isForce) {
  std::vector<std::string> archiveList;

  for (auto const &i : packages) {
    if (std::filesystem::exists(i)) {
      archiveList.push_back(i);
      continue;
    }

    auto package = m_Repository[i];
    if (!package) {
      p_Error = "no package found with name '" + i + "'";
      return false;
    }

    if (m_SystemDatabase.isInstalled(*package) && !isForce) {
      p_Error = package->id() + " is already installed in the system";
      return false;
    }

    auto archiveFile = package->file();
    auto archiveFilePath =
        m_PackageDir + "/" + package->repository() + "/" + archiveFile;

    if (!std::filesystem::exists(archiveFilePath)) {
      PROCESS("getting " << archiveFile);
      if (!m_Downloader.get(archiveFile, package->repository(),
                            archiveFilePath)) {
        p_Error = m_Downloader.error();
        return false;
      }
    }

    archiveList.push_back(archiveFilePath);
  }

  return _install(archiveList, rootDir, isSkipTriggers, isForce);
}
}  // namespace rlxos::libpkgupd