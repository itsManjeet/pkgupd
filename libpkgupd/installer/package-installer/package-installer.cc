#include "package-installer.hh"
using namespace rlxos::libpkgupd;

#include "../../archive-manager/tarball/tarball.hh"
#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::TARBALL

#include <algorithm>

std::shared_ptr<PackageInfo> PackageInstaller::inject(
    char const* path, std::vector<std::string>& files) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::filesystem::path root_dir;

  archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  if (!archive_manager->list(path, files)) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

  if (!archive_manager->extract(path, root_dir.c_str(), files)) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  return package_info;
}