#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include "configuration.hxx"
#include "package-info.hxx"

namespace rlxos::libpkgupd {

    class InstalledPackageInfo : public PackageInfo {
       private:
        std::string mInstalledon;

       public:
        InstalledPackageInfo(PackageInfo *pkginfo) : PackageInfo{*pkginfo} {}

        InstalledPackageInfo(YAML::Node const &node, char const *file);

        std::string const &installed_on() const { return mInstalledon; }
    };

    class SystemDatabase : public Object {
       private:
        Configuration *mConfig;
        std::string data_dir;

        std::map<std::string, std::shared_ptr<InstalledPackageInfo>> mPackages;

       public:
        SystemDatabase(Configuration *config) : mConfig{config} {
            data_dir = mConfig->get<std::string>(DIR_DATA, DEFAULT_DATA_DIR);
            init();
        }

        bool init();

        std::map<std::string, std::shared_ptr<InstalledPackageInfo>> const &get()
            const {
            return mPackages;
        }

        bool get_files(std::shared_ptr<InstalledPackageInfo> packageInfo,
                       std::vector<std::string> &files);

        std::shared_ptr<InstalledPackageInfo> get(char const *id);

        std::shared_ptr<InstalledPackageInfo> add(std::shared_ptr<PackageInfo> pkginfo,
                                                  std::vector<std::string> const &files,
                                                  std::string root, bool update = false,
                                                  bool is_dependency = false);

        bool remove(char const *id);
    };
}  // namespace rlxos::libpkgupd

#endif