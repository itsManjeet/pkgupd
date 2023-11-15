#ifndef LIBPKGUPD_UNINSTALLER_HH
#define LIBPKGUPD_UNINSTALLER_HH

#include "../configuration.hxx"
#include "../system-database.hxx"

namespace rlxos::libpkgupd {
    class Uninstaller : public Object {
    protected:
        Configuration *mConfig;

    public:
        explicit Uninstaller(Configuration *config) : mConfig{config} {}

        bool uninstall(std::shared_ptr<InstalledPackageInfo> pkginfo,
                       SystemDatabase *sys_db);
    };
}  // namespace rlxos::libpkgupd

#endif