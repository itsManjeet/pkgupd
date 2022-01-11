#include "installer.hh"

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
      _error = packager->error();
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

    if (rootDir == "/") {
      PROCESS("installing " << BLUE(info->id()));
    } else {
      PROCESS("installing " << BLUE(info->id()) << " into " << RED(rootDir));
    }

    if (!packager->extract(rootDir)) {
      _error = packager->error();
      return false;
    }
    packagesFiles.push_back(packager->list());
    packagesList.push_back(*info);
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
      _error = m_SystemDatabase.error();
      return false;
    }

    if (!isSkipTriggers) {
      if (packagesList[i].script().size()) {
        if (withPkgname) {
          PROCESS("post install script " << BLUE(packagesList[i].id()));
        } else {
          PROCESS("post installation script");
        }

        if (int status = Executor().execute(packagesList[i].script());
            status != 0) {
          _error =
              "install script failed to exit code: " + std::to_string(status);
          return false;
        }
      }
    }
  }

  if (isSkipTriggers) {
    INFO("skipping triggers")
  } else {
    auto triggerer_ = Triggerer();
    if (!triggerer_.trigger(packagesFiles)) {
      _error = triggerer_.error();
      return false;
    }
  }

  if (isSkipTriggers) {
    INFO("skipped creating users account")
  } else {
    auto triggerer_ = Triggerer();
    if (!triggerer_.trigger(packagesList)) {
      _error = triggerer_.error();
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
      _error = "no package found with name '" + i + "'";
      return false;
    }

    if (m_SystemDatabase.isInstalled(*package) && !isForce) {
      _error = package->id() + " is already installed in the system";
      return false;
    }

    auto archiveFile = package->file();
    auto archiveFilePath = m_PackageDir + "/" + archiveFile;

    if (!std::filesystem::exists(archiveFilePath)) {
      PROCESS("getting " << archiveFile);
      if (!m_Downloader.get(archiveFile, archiveFilePath)) {
        _error = m_Downloader.error();
        return false;
      }
    }

    archiveList.push_back(archiveFilePath);
  }

  return _install(archiveList, rootDir, isSkipTriggers, isForce);
}
}  // namespace rlxos::libpkgupd