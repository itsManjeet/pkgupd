#include "packager.hh"

#include "formats/tar/fonts.hh"
#include "formats/tar/icons.hh"
#include "formats/tar/theme.hh"
#include "formats/image.hh"
#include "formats/tar.hh"

namespace rlxos::libpkgupd {
std::shared_ptr<Packager> Packager::create(PackageType packageType,
                                           std::string const& packageFile) {
  switch (packageType) {
    case PackageType::PACKAGE:
    case PackageType::RLXOS:
      return std::make_shared<Tar>(packageFile);
    case PackageType::APPIMAGE:
      return std::make_shared<Image>(packageFile);
    case PackageType::FONT:
      return std::make_shared<Fonts>(packageFile);
    case PackageType::ICON:
      return std::make_shared<Icons>(packageFile);
    case PackageType::THEME:
      return std::make_shared<Themes>(packageFile);
    default:
      throw std::runtime_error("unsupport package type: " +
                               packageTypeToString(packageType));
  }
  return nullptr;
}

std::shared_ptr<Packager> Packager::create(std::string const& packageFile) {
  PackageType packageType = static_cast<PackageType>(-1);
  if (std::filesystem::path(packageFile).has_extension()) {
    auto ext = std::filesystem::path(packageFile).extension().string();
    ext = ext.substr(1, ext.length() - 1);
    packageType = stringToPackageType(ext);
  }
  switch (packageType) {
    case PackageType::PACKAGE:
    case PackageType::RLXOS:
      return std::make_shared<Tar>(packageFile);
    case PackageType::APPIMAGE:
      return std::make_shared<Image>(packageFile);
    case PackageType::FONT:
      return std::make_shared<Fonts>(packageFile);
    case PackageType::ICON:
      return std::make_shared<Icons>(packageFile);
    case PackageType::THEME:
      return std::make_shared<Themes>(packageFile);
    default:
      throw std::runtime_error("unsupport package type: " +
                               packageTypeToString(packageType));
  }
  return nullptr;
}
}  // namespace rlxos::libpkgupd
