#ifndef LIBPKGUPD_INSTALLER_HH
#define LIBPKGUPD_INSTALLER_HH

#include "../configuration.hh"
#include "../defines.hh"
#include "../system-database.hh"

namespace rlxos::libpkgupd {
class Installer : public Object {
 protected:
  Configuration* mConfig;

 public:
  Installer(Configuration* config) : mConfig{config} {}

  virtual std::shared_ptr<InstalledPackageInfo> install(
      char const* path, SystemDatabase* sys_db, bool force = false) = 0;

  static std::shared_ptr<Installer> create(PackageType pkgtype, Configuration* config);
};
}  // namespace rlxos::libpkgupd

#endif