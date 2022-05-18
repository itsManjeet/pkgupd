#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include "configuration.hh"
#include "package-info.hh"
namespace rlxos::libpkgupd {

class InstalledPackageInfo : public PackageInfo {
 private:
  std::vector<std::string> mFiles;

 public:
  InstalledPackageInfo(PackageInfo *pkginfo,
                       std::vector<std::string> const &files)
      : PackageInfo{*pkginfo}, mFiles{files} {}

  InstalledPackageInfo(YAML::Node const &node, char const *file);

  std::vector<std::string> const &files() const { return mFiles; }
};

class SystemDatabase : public Object {
 private:
  Configuration *mConfig;

 public:
  SystemDatabase(Configuration *config) : mConfig{config} {}

  std::shared_ptr<InstalledPackageInfo> get(char const *id);

  bool add(PackageInfo *pkginfo, std::vector<std::string> const &files,
           std::string root, bool update = false);

  bool remove(InstalledPackageInfo *pkginfo);
};
}  // namespace rlxos::libpkgupd

#endif