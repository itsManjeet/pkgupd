#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include "database.hh"

namespace rlxos::libpkgupd {

class SystemDatabase : public Database {
 public:
  SystemDatabase(std::string const &d) : Database(d) {
    DEBUG("System Database: " << p_DataDir);
  }

  std::optional<Package> operator[](std::string const &pkgid);

  std::vector<Package> all();

  bool isInstalled(Package const &pkginfo);

  bool isOutDated(Package const &pkginfo);

  bool registerIntoSystem(Package const &pkginfo,
           std::vector<std::string> const &files, std::string root,
           bool update = false);

  bool unregisterFromSystem(Package const &pkginfo);
};
}  // namespace rlxos::libpkgupd

#endif