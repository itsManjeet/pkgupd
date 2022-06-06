#include "package-installer.hh"
using namespace rlxos::libpkgupd;

#include "../../archive-manager/tarball/tarball.hh"
#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::TARBALL

#include <algorithm>

std::shared_ptr<InstalledPackageInfo> PackageInstaller::install(
    char const* path, SystemDatabase* sys_db, bool force) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::filesystem::path root_dir;
  std::vector<std::string> extracted_files;
  std::shared_ptr<InstalledPackageInfo> installed_package_info;

  archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  if (!archive_manager->list(path, extracted_files)) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

  if (!archive_manager->extract(path, root_dir.c_str(), extracted_files)) {
    p_Error = archive_manager->error();
    return nullptr;
  }

  std::vector<std::string> deprected_files;
  auto installed_package = sys_db->get(package_info->id().c_str());
  if (installed_package != nullptr && force != false) {
    auto old_files = installed_package->files();
    std::for_each(
        old_files.begin(), old_files.end(), [&](std::string const& file) {
          if (std::filesystem::exists(file) &&
              std::find(extracted_files.begin(), extracted_files.end(), file) ==
                  extracted_files.end()) {
            if (file.find("./bin", 0) == 0 || file.find("./lib", 0) == 0 ||
                file.find("./sbin", 0) == 0) {
              return;
            }

            std::error_code error;
            std::filesystem::remove(root_dir / file, error);
            if (error) ERROR("failed to remove " << file);
          }
        });
  }

  if (!sys_db->add(package_info.get(), extracted_files, root_dir, force)) {
    p_Error = sys_db->error();
    return nullptr;
  }

  installed_package_info = sys_db->get(package_info->id().c_str());
  p_Error = sys_db->error();
  // TODO: check equality of installed package info;
  return installed_package_info;
}