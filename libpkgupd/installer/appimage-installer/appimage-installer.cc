#include "appimage-installer.hh"
using namespace rlxos::libpkgupd;

#include "../../archive-manager/squash/squash.hh"
#define ARCHIVE_MANAGER_TYPE ArchiveManagerType::APPIMAGE

#include <sys/stat.h>

#include <fstream>
#include <regex>

#include "../../utils/utils.hh"

bool AppImageInstaller::patch(std::string filepath,
                              std::map<std::string, std::string> replaces) {
  std::ifstream infile(filepath);
  if (!infile.good()) {
    p_Error = "failed to read " + filepath;
    return false;
  }
  std::string input((std::istreambuf_iterator<char>(infile)),
                    (std::istreambuf_iterator<char>()));
  infile.close();
  for (auto const& i : replaces)
    input = std::regex_replace(input, std::regex(i.first), i.second);

  std::ofstream outfile(filepath);
  outfile << input;
  outfile.close();

  return true;
}

bool AppImageInstaller::extract(ArchiveManager* archiveManager,
                                std::string archive_file, std::string filepath,
                                std::string app_dir, std::string& target_path) {
  std::filesystem::path src = filepath;
  std::filesystem::path target = std::filesystem::path(app_dir) / src;
  auto idx = filepath.find(':');
  if (idx != std::string::npos) {
    src = filepath.substr(0, idx);
    target = std::filesystem::path(app_dir) / filepath.substr(idx + 1);
  }

  if (!std::filesystem::exists(target.parent_path())) {
    std::error_code code;
    std::filesystem::create_directories(target.parent_path(), code);
    if (code) {
      p_Error = "failed to create required dir '" +
                target.parent_path().string() + "', " + code.message();
      return false;
    }
  }

  if (!archiveManager->extract_file(archive_file.c_str(), src.c_str(),
                                    target.c_str())) {
    p_Error = "failed to extract '" + src.string() + "' to '" +
              target.string() + "', " + archiveManager->error();
    return false;
  }
  target_path = target;
  return true;
}

std::shared_ptr<PackageInfo> AppImageInstaller::inject(
    char const* path, std::vector<std::string>& files) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::filesystem::path root_dir, apps_dir, apps_data_dir;
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
  apps_data_dir =
      mConfig->get<std::string>(DIR_APPS_DATA, DEFAULT_APPS_DIR "/share");

  auto app_file = apps_dir / std::filesystem::path(path).filename().string();
  auto app_path = root_dir / app_file;

  files.push_back("./" + app_file.string());

  if (!std::filesystem::exists(apps_dir)) {
    std::error_code err;
    std::filesystem::create_directories(root_dir / apps_dir, err);
    if (err) {
      p_Error = "failed to create required app directory, " + err.message();
      return nullptr;
    }
  }

  // install app
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

  // install icon
  std::string _path;
  std::string icon_file_path = "pixmaps/" + package_info->id() + ".png";
  if (!extract(archive_manager.get(), path,
               package_info->id() + ".png:" + icon_file_path,
               root_dir / apps_data_dir, _path)) {
    return nullptr;
  }
  files.push_back("./" + apps_data_dir.string() + "/" + icon_file_path);

  // install desktop file
  std::string desktop_file_path =
      "applications/" + package_info->id() + "-" +
      std::to_string(std::hash<std::string>()(package_info->id() + "-" +
                                              package_info->version())) +
      ".desktop";

  files.push_back("./" + apps_data_dir.string() + "/" + desktop_file_path);

  std::string desktop_file;
  if (!archive_manager->get(path, (package_info->id() + ".desktop").c_str(),
                            desktop_file)) {
    p_Error = "failed to read desktop file, not exists";
    return nullptr;
  }

  std::ofstream desktop(root_dir / desktop_file_path);
  std::stringstream ss(desktop_file);
  files.push_back("./" + desktop_file_path);

  std::string line;
  bool is_action = false;
  while (std::getline(ss, line, '\n')) {
    if (line.find("Exec=", 0) == 0) {
      desktop << "Exec=/" << app_file.string() << std::endl;
    } else if (line.find("Icon=", 0) == 0) {
      desktop << "Icon=" << package_info->id() << std::endl;
    } else if (line.find("Actions=", 0) == 0) {
      is_action = true;
      desktop << line << ";Remove" << std::endl;
    } else {
      desktop << line << std::endl;
    }
  }

  if (!is_action) desktop << "Actions=Remove;" << std::endl;

  desktop << "\n[Desktop Action Remove]\n"
          << "Name=Uninstall\n"
          << "Exec=/usr/bin/pkexec pkgupd remove " << package_info->id()
          << std::endl;
  desktop.close();

  if (package_info->node()["extra"] && package_info->node()["extra"]["files"]) {
    for (auto const& i : package_info->node()["extra"]["files"]) {
      std::string target_path;
      if (!extract(archive_manager.get(), path, i.as<std::string>(),
                   root_dir / apps_data_dir, target_path)) {
        return nullptr;
      }
      files.push_back("./" + (apps_data_dir / target_path).string());

      if (!patch(target_path, {
                                  {"@@exec@@", "/" + app_file.string()},
                                  {"@@id@@", package_info->id()},
                              })) {
        return nullptr;
      }
    }
  }

  return package_info;
}