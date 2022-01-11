#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include "database.hh"

namespace rlxos::libpkgupd {
class Repository : public Database {
 public:
  Repository(std::string const &dataDir) : Database(dataDir) {
    DEBUG("Repository Database: " << dataDir);
  }

  std::vector<Package> all();
  std::optional<Package> operator[](std::string const &pkgid);
};
}  // namespace rlxos::libpkgupd

#endif