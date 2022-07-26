#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include "configuration.hh"
#include "package-info.hh"
namespace rlxos::libpkgupd {

class InstalledPackageInfo : public PackageInfo {
 private:
  std::vector<std::string> mFiles;
  std::string mInstalledon;

 public:
  InstalledPackageInfo(PackageInfo *pkginfo,
                       std::vector<std::string> const &files)
      : PackageInfo{*pkginfo}, mFiles{files} {}

  InstalledPackageInfo(YAML::Node const &node, char const *file);

  std::vector<std::string> const &files() const {
    return mFiles;
  };
  std::string const &installed_on() const { return mInstalledon; }
};

class SystemDatabase : public Object {
 private:
  Configuration *mConfig;
  std::string data_dir;

  std::map<std::string, std::unique_ptr<InstalledPackageInfo>> mPackages;

 public:
  SystemDatabase(Configuration *config) : mConfig{config} {
    data_dir = mConfig->get<std::string>(DIR_DATA, DEFAULT_DATA_DIR);
    init();
  }

  bool init();

  std::map<std::string, std::unique_ptr<InstalledPackageInfo>> const &get()
      const {
    return mPackages;
  }

  InstalledPackageInfo *get(char const *id);

  InstalledPackageInfo *add(PackageInfo *pkginfo,
                            std::vector<std::string> const &files,
                            std::string root, bool update = false,
                            bool is_dependency = false);

  bool remove(char const* id);
};
}  // namespace rlxos::libpkgupd

#endif