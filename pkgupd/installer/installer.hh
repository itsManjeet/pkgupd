#ifndef LIBPKGUPD_INSTALLER_HH
#define LIBPKGUPD_INSTALLER_HH

#include "../configuration.hh"
#include "../defines.hh"
#include "../repository.hh"
#include "../system-database.hh"

namespace rlxos::libpkgupd {
class Installer : public Object {
   public:
    class Injector : public Object {
       protected:
        Configuration* mConfig;

       public:
        Injector(Configuration* config) : mConfig(config) {}
        virtual std::shared_ptr<PackageInfo> inject(char const* path,
                                                    std::vector<std::string>& files,
                                                    bool is_dependency = false) = 0;

        static std::shared_ptr<Injector> create(PackageType package_type,
                                                Configuration* config);
    };

   protected:
    Configuration* mConfig;

   public:
    Installer(Configuration* config) : mConfig{config} {}

    bool install(std::vector<std::pair<std::string, std::shared_ptr<PackageInfo>>> const& pkgs,
                 SystemDatabase* sys_db);

    bool install(std::vector<std::shared_ptr<PackageInfo>> pkgs, Repository* repository,
                 SystemDatabase* sys_db);
};
}  // namespace rlxos::libpkgupd

#endif