#ifndef LIBPKGUPD_INSTALLER_HH
#define LIBPKGUPD_INSTALLER_HH

#include "../configuration.hxx"
#include "../defines.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"

namespace rlxos::libpkgupd {
    class Installer : public Object {
       protected:
        Configuration *mConfig;

       public:
        Installer(Configuration *config) : mConfig{config} {}

        bool install(std::vector<std::pair<std::string, std::shared_ptr<PackageInfo>>> const &pkgs,
                     SystemDatabase *sys_db);

        bool install(std::vector<std::shared_ptr<PackageInfo>> pkgs, Repository *repository,
                     SystemDatabase *sys_db);
    };
}  // namespace rlxos::libpkgupd

#endif