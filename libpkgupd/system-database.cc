#include "system-database.hh"

#include <string.h>
#include <sys/stat.h>

#include <ctime>
#include <fstream>
#include <optional>

namespace rlxos::libpkgupd {

std::optional<Package> SystemDatabase::operator[](std::string const &pkgid) {
  auto datafile = p_DataDir + "/" + pkgid;
  if (!std::filesystem::exists(datafile)) {
    p_Error = "no package found with name '" + pkgid + "' in system database";
    return {};
  }

  DEBUG("Found at: " << datafile);

  return Package(YAML::LoadFile(datafile), datafile);
}

bool SystemDatabase::isInstalled(Package const &pkginfo) {
  return ((*this)[pkginfo.id()].has_value());
}

bool SystemDatabase::isOutDated(Package const &pkginfo) {
  auto installedPackage = (*this)[pkginfo.id()];
  if (!installedPackage.has_value())
    throw std::runtime_error(pkginfo.id() + " is missing in sysdb");

  return (installedPackage->version() != pkginfo.version());
}

bool SystemDatabase::unregisterFromSystem(Package const &pkginfo) {
  auto installedPackage = (*this)[pkginfo.id()];
  if (!installedPackage) {
    p_Error = "no package with id " + pkginfo.id() + " is installed";
    return false;
  }

  std::string datafile = p_DataDir + "/" + pkginfo.id();
  std::error_code err;
  std::filesystem::remove(datafile, err);
  if (err) {
    p_Error = err.message();
    return false;
  }

  return true;
}

std::vector<Package> SystemDatabase::all() {
  if (!std::filesystem::exists(p_DataDir)) {
    p_Error = "no packages database found";
    return {};
  }

  std::vector<Package> pkgs;
  for (auto const &i : std::filesystem::directory_iterator(p_DataDir)) {
    YAML::Node data = YAML::LoadFile(i.path().string());
    pkgs.push_back(Package(data, i.path().string()));
  }

  return pkgs;
}

bool SystemDatabase::registerIntoSystem(Package const &pkginfo,
                                        std::vector<std::string> const &files,
                                        std::string root, bool toupdate) {
  try {
    if (isInstalled(pkginfo) && !isOutDated(pkginfo) && !toupdate) {
      p_Error = pkginfo.id() + " " + pkginfo.version() +
                " is already registered in the system";
      return false;
    }

    toupdate = isOutDated(pkginfo);
  } catch (...) {
  }

  auto datafile = p_DataDir + "/" + pkginfo.id();
  {
    std::ofstream file(datafile);
    if (!file.is_open()) {
      p_Error = "failed to open sysdb to register " + pkginfo.id();
      return false;
    }
    pkginfo.dump(file);

    // DUMP extra data
    std::time_t t = std::time(0);
    std::tm *now = std::localtime(&t);

    file << "installed_on: " << (now->tm_year + 1900) << "/"
         << (now->tm_mon + 1) << "/" << (now->tm_mday) << " " << (now->tm_hour)
         << ":" << (now->tm_min) << std::endl;

    file << "files:" << std::endl;
    for (auto i : files) file << "  - " << i << std::endl;
  }

  return true;
}
}  // namespace rlxos::libpkgupd