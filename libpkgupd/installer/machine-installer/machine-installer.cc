#include "machine-installer.hh"
using namespace rlxos::libpkgupd;
#include "../../archive-manager/tarball/tarball.hh"

#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::TARBALL

std::shared_ptr<PackageInfo> MachineInstaller::inject(
    char const* path, std::vector<std::string>& files, bool is_dependency) {
  auto archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  if (archive_manager == nullptr) {
    p_Error = "failed to initialize archive manager";
    return nullptr;
  }

  auto package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = " : failed to read information file from machine file : " +
              archive_manager->error();
    return nullptr;
  }

  if (!archive_manager->list(path, files)) {
    p_Error =
        " : failed to list files from machine : " + archive_manager->error();
    return nullptr;
  }

  std::filesystem::path machine_path =
      mConfig->get<std::string>("machine.path", "/var/lib/machines/") +
      (package_info->id());

  if (!archive_manager->extract(path, machine_path.c_str(), files)) {
    p_Error = " : failed to extract machine : " + archive_manager->error();
    return nullptr;
  }

  return package_info;
}