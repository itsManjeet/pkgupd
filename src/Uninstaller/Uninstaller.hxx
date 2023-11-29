#ifndef LIBPKGUPD_UNINSTALLER_HH
#define LIBPKGUPD_UNINSTALLER_HH

#include "../configuration.hxx"
#include "../system-database.hxx"

namespace rlxos::libpkgupd {
    class Uninstaller {
    protected:
        Configuration* mConfig;

    public:
        explicit Uninstaller(Configuration* config) : mConfig{config} {
        }

        void uninstall(const InstalledMetaInfo& meta_info,
                       SystemDatabase* sys_db);
    };
} // namespace rlxos::libpkgupd

#endif
