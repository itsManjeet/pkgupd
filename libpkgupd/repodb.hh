#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include "db.hh"

namespace rlxos::libpkgupd {
class Repository : public Database {
 public:
  Repository(std::string const &data_dir) : Database(data_dir) {
    DEBUG("Repository Database: " << data_dir);
  }

  std::vector<std::shared_ptr<PackageInformation>> all();
  std::shared_ptr<PackageInformation> operator[](std::string const &pkgid);
};
}  // namespace rlxos::libpkgupd

#endif