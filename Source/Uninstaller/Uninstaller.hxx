#ifndef LIBPKGUPD_UNINSTALLER_HH
#define LIBPKGUPD_UNINSTALLER_HH

#include "../configuration.hxx"
#include "../system-database.hxx"

namespace libpkgupd {
class Uninstaller : public Object {
 protected:
  Configuration* mConfig;

 public:
  Uninstaller(Configuration* config) : mConfig{config} {}

  bool uninstall(InstalledPackageInfo* pkginfo,
                         SystemDatabase* sys_db);
};
}  // namespace libpkgupd

#endif