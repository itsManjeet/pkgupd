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
}  // namespace rlxos::libpkgupd
