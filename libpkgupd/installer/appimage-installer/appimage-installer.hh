#ifndef LIBPKGUPD_APPIMAGE_INSTALLER_HH
#define LIBPKGUPD_APPIMAGE_INSTALLER_HH

#include "../installer.hh"

namespace rlxos::libpkgupd {
class AppImageInstaller : public Installer {
 private:
 public:
  AppImageInstaller(Configuration* config) : Installer{config} {}

  std::shared_ptr<InstalledPackageInfo> install(char const* path,
                                                SystemDatabase* sys_db,
                                                bool force = false);
};
}  // namespace rlxos::libpkgupd

#endif