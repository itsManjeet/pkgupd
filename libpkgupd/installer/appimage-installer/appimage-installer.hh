#ifndef LIBPKGUPD_APPIMAGE_INSTALLER_HH
#define LIBPKGUPD_APPIMAGE_INSTALLER_HH

#include "../installer.hh"

namespace rlxos::libpkgupd {
class AppImageInstaller : public Installer::Injector {
 private:
 public:
  AppImageInstaller(Configuration* config) : Installer::Injector{config} {}

  std::shared_ptr<PackageInfo> inject(char const* path,
                                      std::vector<std::string>& files);
};
}  // namespace rlxos::libpkgupd

#endif