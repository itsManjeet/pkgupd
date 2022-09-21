#ifndef LIBPKGUPD_MACHINE_INSTALLER_HH
#define LIBPKGUPD_MACHINE_INSTALLER_HH

#include "../installer.hh"

namespace rlxos::libpkgupd {
class MachineInstaller : public Installer::Injector {
 public:
  MachineInstaller(Configuration* config) : Installer::Injector{config} {}

  std::shared_ptr<PackageInfo> inject(char const* path,
                                      std::vector<std::string>& files,
                                      bool is_dependency = false);
};
}  // namespace rlxos::libpkgupd

#endif