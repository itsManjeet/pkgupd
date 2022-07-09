#ifndef LIBPKGUPD_PACKAGE_INSTALLER_HH
#define LIBPKGUPD_PACKAGE_INSTALLER_HH

#include "../installer.hh"

namespace rlxos::libpkgupd {
class PackageInstaller : public Installer::Injector {
 public:
  PackageInstaller(Configuration* config) : Installer::Injector{config} {}

  std::shared_ptr<PackageInfo> inject(char const* path,
                                      std::vector<std::string>& files,
                                      bool is_dependency = false);
};
}  // namespace rlxos::libpkgupd

#endif