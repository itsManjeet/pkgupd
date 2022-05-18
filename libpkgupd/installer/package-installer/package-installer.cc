#include "package-installer.hh"
using namespace rlxos::libpkgupd;

#include "../../archive-manager/tarball/tarball.hh"
#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::TARBALL

std::shared_ptr<InstalledPackageInfo> PackageInstaller::install(
    char const* path, SystemDatabase* sys_db, bool force) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::string root_dir;
  std::vector<std::string> extracted_files;
  std::shared_ptr<InstalledPackageInfo> installed_package_info;

  archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  root_dir = mConfig->get<std::string>("root-dir", DEFAULT_ROOT_DIR);

  // TODO: check and clean deprecated files

  if (!archive_manager->extract(path, root_dir.c_str(), extracted_files)) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  if (!sys_db->add(package_info.get(), extracted_files, root_dir, force)) {
    p_Error = sys_db->error();
    return nullptr;
  }

  installed_package_info = sys_db->get(package_info->id().c_str());
  // TODO: check equality of installed package info;
  return installed_package_info;
}