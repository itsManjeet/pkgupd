#include "installer.hh"
using namespace rlxos::libpkgupd;

#include "package-installer/package-installer.hh"

std::shared_ptr<Installer> Installer::create(PackageType pkgtype,
                                             Configuration* config) {
  switch (pkgtype) {
    case PackageType::PACKAGE:
    case PackageType::RLXOS:
      return std::make_shared<PackageInstaller>(config);
    default:
      return nullptr;
  }
}