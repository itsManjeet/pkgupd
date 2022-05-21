#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include <memory>

#include "configuration.hh"
#include "package-info.hh"
namespace rlxos::libpkgupd {
class Repository : public Object {
 private:
  Configuration *mConfig;
  std::vector<std::string> repos_list;
  std::string repo_dir;

 public:
  Repository(Configuration *config);

  std::vector<std::string> const &repos() const { return repos_list; }

  bool list_all(std::vector<std::string> &ids);

  std::shared_ptr<PackageInfo> get(char const *pkgid);
};
}  // namespace rlxos::libpkgupd

#endif