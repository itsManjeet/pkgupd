#include "appimage-installer.hh"
using namespace rlxos::libpkgupd;

#include "../../archive-manager/squash/squash.hh"
#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::APPIMAGE

#include <sys/stat.h>

#include <fstream>

#include "../../utils/utils.hh"

std::shared_ptr<InstalledPackageInfo> AppImageInstaller::install(
    char const* path, SystemDatabase* sys_db, bool force) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::filesystem::path root_dir, apps_dir;
  std::vector<std::string> extracted_files;
  std::shared_ptr<InstalledPackageInfo> installed_package_info;
  std::string offset;

  archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return nullptr;
  }
  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  apps_dir = mConfig->get<std::string>(DIR_APPS, DEFAULT_APPS_DIR);

  auto app_path = root_dir / apps_dir / std::filesystem::path(path).filename();

  extracted_files.push_back(apps_dir / std::filesystem::path(path).filename());

  for (std::string dir :
       {"share/pixmaps", "share/applications", "share/dbus-1/services"}) {
    std::error_code err;
    std::filesystem::create_directories(root_dir / apps_dir / dir, err);
    if (err) {
      p_Error = "failed to create required directories, " + err.message();
      return nullptr;
    }
  }
  std::error_code err;
  std::filesystem::copy(path, app_path,
                        std::filesystem::copy_options::overwrite_existing, err);
  if (err) {
    p_Error =
        "failed to inject app '" + app_path.string() + "'" + err.message();
    return nullptr;
  }

  if (chmod(app_path.string().c_str(), 0755) != 0) {
    p_Error = "failed to change executable permission '" +
              std::string(strerror(errno)) + "' for '" + app_path.string() +
              "'";
    return nullptr;
  }

  std::string icon_file;
  if (!archive_manager->get(path, (package_info->id() + ".png").c_str(),
                            icon_file)) {
    p_Error = "failed to read icon file, not exists";
    return nullptr;
  }

  std::string icon_file_path =
      apps_dir / "share/pixmaps" /
      (package_info->id() + "-" + utils::random(10) + ".png");

  std::ofstream icon(root_dir / icon_file_path);
  icon << icon_file;
  icon.close();
  extracted_files.push_back(icon_file_path);

  std::string desktop_file;
  if (!archive_manager->get(path, (package_info->id() + ".desktop").c_str(),
                            desktop_file)) {
    p_Error = "failed to read desktop file, not exists";
    return nullptr;
  }

  std::string desktop_file_path =
      apps_dir / "share/applications" /
      (package_info->id() + "-" + utils::random(10) + ".desktop");
  std::ofstream desktop(root_dir / desktop_file_path);
  desktop << desktop_file;
  desktop.close();

  if (!sys_db->add(package_info.get(), extracted_files, root_dir, force)) {
    p_Error = sys_db->error();
    return nullptr;
  }

  installed_package_info = sys_db->get(package_info->id().c_str());
  p_Error = sys_db->error();
  // TODO: check equality of installed package info;
  return installed_package_info;
}