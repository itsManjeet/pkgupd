#ifndef LIBPKGUPD_UNINSTALLER_HH
#define LIBPKGUPD_UNINSTALLER_HH

#include "../configuration.hh"
#include "../system-database.hh"

namespace rlxos::libpkgupd {
class Uninstaller : public Object {
 protected:
  Configuration* mConfig;

 public:
  Uninstaller(Configuration* config) : mConfig{config} {}

  bool uninstall(InstalledPackageInfo* pkginfo,
                         SystemDatabase* sys_db);
};
}  // namespace rlxos::libpkgupd

#endif