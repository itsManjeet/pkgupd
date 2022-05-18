#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include <memory>

#include "configuration.hh"
#include "package-info.hh"
namespace rlxos::libpkgupd {
class Repository : public Object {
 private:
  Configuration *mConfig;

 public:
  Repository(Configuration *config) : mConfig{config} {}

  void repos(std::vector<std::string> &repos) const {
    mConfig->get("repos", repos);
  }

  std::shared_ptr<PackageInfo> get(char const *pkgid);
};
}  // namespace rlxos::libpkgupd

#endif