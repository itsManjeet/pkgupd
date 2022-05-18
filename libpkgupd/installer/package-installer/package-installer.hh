#ifndef LIBPKGUPD_PACKAGE_INSTALLER_HH
#define LIBPKGUPD_PACKAGE_INSTALLER_HH

#include "../installer.hh"

namespace rlxos::libpkgupd {
class PackageInstaller : public Installer {
 public:
  PackageInstaller(Configuration* config) : Installer{config} {}

  std::shared_ptr<InstalledPackageInfo> install(char const* path,
                                                SystemDatabase* sys_db,
                                                bool force = false);
};
}  // namespace rlxos::libpkgupd

#endif