#ifndef LIBPKGUPD_INSTALLER_HH
#define LIBPKGUPD_INSTALLER_HH

#include "../configuration.hxx"
#include "../defines.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"

namespace rlxos::libpkgupd {
    class Installer {
    protected:
        Configuration* mConfig;

    public:
        explicit Installer(Configuration* config) : mConfig{config} {
        }

        void install(std::vector<std::pair<std::string, MetaInfo>> const& pkgs,
                     SystemDatabase* sys_db);

        void install(std::vector<MetaInfo> pkgs, Repository* repository,
                     SystemDatabase* sys_db);
    };
} // namespace rlxos::libpkgupd

#endif
