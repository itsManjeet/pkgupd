#include "packager.hh"

#include "image.hh"
#include "tar.hh"

namespace rlxos::libpkgupd {
std::shared_ptr<Packager> Packager::create(PackageType packageType,
                                           std::string const& packageFile) {
  switch (packageType) {
    case PackageType::PACKAGE:
      return std::make_shared<Tar>(packageFile);
    case PackageType::APPIMAGE:
      return std::make_shared<Image>(packageFile);
    default:
      throw std::runtime_error("unsupport package type: " +
                               packageTypeToString(packageType));
  }
  return nullptr;
}
}  // namespace rlxos::libpkgupd
