#include "archive-manager.hh"

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
  }

  return nullptr;
}
}  // namespace rlxos::libpkgupd
