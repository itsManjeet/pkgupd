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
  std::map<std::string, std::unique_ptr<PackageInfo>> mPackages;

 public:
  Repository(Configuration *config);
  
  bool init();

  std::map<std::string, std::unique_ptr<PackageInfo>> const& get() const {
    return mPackages;
  }

  std::vector<std::string> const &repos() const { return repos_list; }

  PackageInfo* get(char const *pkgid);
};
}  // namespace rlxos::libpkgupd

#endif