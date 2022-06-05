#include "installer.hh"
using namespace rlxos::libpkgupd;

#include "package-installer/package-installer.hh"
#include "appimage-installer/appimage-installer.hh"

std::shared_ptr<Installer> Installer::create(PackageType pkgtype,
                                             Configuration* config) {
  switch (pkgtype) {
    case PackageType::PACKAGE:
    case PackageType::RLXOS:
      return std::make_shared<PackageInstaller>(config);
    case PackageType::APPIMAGE:
      return std::make_shared<AppImageInstaller>(config);
    default:
      return nullptr;
  }
}