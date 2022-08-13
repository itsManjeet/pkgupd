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
    char const* path, std::vector<std::string>& files, bool is_dependency) {
  std::shared_ptr<ArchiveManager> archive_manager;
  std::shared_ptr<PackageInfo> package_info;
  std::filesystem::path root_dir, apps_dir, app_data_path;
  std::string offset;

  archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  package_info = archive_manager->info(path);
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return nullptr;
  }
  if (is_dependency) {
    package_info->setDependency();
  }

  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  apps_dir = mConfig->get<std::string>(DIR_APPS, DEFAULT_APPS_DIR);

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

  if (!intergrate(app_file.c_str(), files, [&](mINI::INIStructure& ini) {
        auto oldAction = ini["Desktop Entry"]["Actions"];
        std::string id = "Uninstall";
        ini["Desktop Entry"]["Actions"] += id + ";";

        ini["Desktop Action Uninstall"]["Name"] = "Uninstall";
        ini["Desktop Action Uninstall"]["Exec"] = "/bin/pkexec pkgupd remove " +
                                                  package_info->id() + " " +
                                                  "mode.ask=false";
      })) {
    return nullptr;
  }

  return package_info;
}

bool AppImageInstaller::intergrate(
    char const* app_file, std::vector<std::string>& files,
    std::function<void(mINI::INIStructure&)> desktopFileModifier) {
  auto archive_manager = ArchiveManager::create(ARCHIVE_MANAGER_TYPE);
  assert(archive_manager != nullptr);

  std::filesystem::path app_data_path =
      mConfig->get<std::string>(DIR_APPS_DATA, DEFAULT_APPS_DIR "/share");
  std::filesystem::path root_dir =
      mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);

  std::string path = (root_dir / std::string(app_file)).c_str();
  auto package_info = archive_manager->info(path.c_str());
  if (package_info == nullptr) {
    p_Error = archive_manager->error();
    return false;
  }

  // install icon
  std::string _path;
  std::filesystem::path icon_file_path = app_data_path / "icons" / "hicolor" /
                                         "256x256" / "apps" /
                                         (package_info->id() + ".png");
  if (!extract(archive_manager.get(), path,
               package_info->id() + ".png:" + icon_file_path.string(), root_dir,
               _path)) {
    return false;
  }
  files.push_back("./" + icon_file_path.string());

  // install desktop file
  std::filesystem::path desktop_file_path =
      app_data_path /
      ("applications/" + package_info->id() + "-" +
       std::to_string(std::hash<std::string>()(package_info->id() + "-" +
                                               package_info->version())) +
       ".desktop");
  if (!extract(archive_manager.get(), path,
               package_info->id() + ".desktop:" + desktop_file_path.string(),
               root_dir, _path)) {
    return false;
  }
  files.push_back("./" + desktop_file_path.string());

  // Legcay Patch
  {
    std::ifstream infile(_path);
    std::string input((std::istreambuf_iterator<char>(infile)),
                      (std::istreambuf_iterator<char>()));
    infile.close();

    std::ofstream outfile(_path);
    std::string line;
    std::stringstream ss(input);
    while (std::getline(ss, line, '\n')) {
      if (line.find("Exec=", 0) == 0) {
        auto first_space = line.find_first_of(' ');
        if (first_space == std::string::npos) {
          outfile << "Exec=/" << app_file << std::endl;
        } else {
          outfile << "Exec=/" << app_file << " " << line.substr(first_space)
                  << std::endl;
        }
      } else if (line.find("Icon=", 0) == 0) {
        outfile << "Icon=" << package_info->id() << std::endl;
      } else if (line.find("TryExec=", 0) == 0) {
        continue;
      } else {
        outfile << line << std::endl;
      }
    }
  }

  if (!patch(_path, {
                        {"@@exec@@", app_file},
                        {"@@icon@@", package_info->id()},
                    })) {
    return false;
  }

  if (desktopFileModifier != nullptr) {
    mINI::INIFile desktopfile(_path);
    mINI::INIStructure ini;
    desktopfile.read(ini);
    desktopFileModifier(ini);
    desktopfile.write(ini);
  }

  if (package_info->node()["extra"] && package_info->node()["extra"]["files"]) {
    for (auto const& i : package_info->node()["extra"]["files"]) {
      std::string target_path;
      if (!extract(archive_manager.get(), path, i.as<std::string>(),
                   root_dir / app_data_path, target_path)) {
        return false;
      }
      files.push_back("./" + (app_data_path / target_path).string());

      if (!patch(target_path, {
                                  {"@@exec@@", "/" + std::string(app_file)},
                                  {"@@id@@", package_info->id()},
                              })) {
        return false;
      }
    }
  }
  return true;
}