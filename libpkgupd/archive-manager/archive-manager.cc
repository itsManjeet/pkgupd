#include "archive-manager.hh"

#include "appimage/appimage.hh"
#include "squash/squash.hh"
#include "tarball/tarball.hh"

namespace rlxos::libpkgupd {
std::shared_ptr<ArchiveManager> ArchiveManager::create(
    ArchiveManagerType type) {
  switch (type) {
#define X(ID, Object)          \
  case ArchiveManagerType::ID: \
    return std::make_shared<Object>();

    ARCHIVE_TYPE_LIST
  }

#undef X

  return nullptr;
}

std::shared_ptr<ArchiveManager> ArchiveManager::create(PackageType type) {
  switch (type) {
    case PackageType::PACKAGE:
    case PackageType::RLXOS:
    case PackageType::FONT:
    case PackageType::ICON:
    case PackageType::THEME:
      return std::make_shared<TarBall>();
    case PackageType::APPIMAGE:
      return std::make_shared<AppImage>();
    case PackageType::IMAGE:
      return std::make_shared<Squash>();
  }

  return nullptr;
}

std::shared_ptr<ArchiveManager> ArchiveManager::create(
    std::string packagePath) {
  std::filesystem::path package(packagePath);
  if (!package.has_extension()) {
    return nullptr;
  }

  std::string ext = package.extension();
  auto packageType = PACKAGE_TYPE_FROM_STR(ext.c_str());
  if (packageType == PackageType::N_PACKAGE_TYPE) {
    return nullptr;
  }
  return ArchiveManager::create(packageType);
}
}  // namespace rlxos::libpkgupd
