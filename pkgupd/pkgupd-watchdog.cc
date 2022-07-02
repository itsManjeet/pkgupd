#include "../libpkgupd/common.hh"
#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/installer/appimage-installer/appimage-installer.hh"
using namespace rlxos::libpkgupd;

#include <thread>

PKGUPD_MODULE_HELP(watchdog) {
  os << "Setup the directory as trigger point for pkgupd" << std::endl
     << PADDING << " Directory in (watchdog.dir) will be inspected" << std::endl
     << PADDING << " for changes, Any package put in that directory"
     << std::endl
     << PADDING << " will be integrated into the system" << std::endl
     << std::endl;
}

enum class Status {
  CREATED,
  MODIFIED,
  REMOVED,
};

class Watcher {
 private:
  std::unordered_map<std::string, std::filesystem::file_time_type> mStamp;
  std::string mPath;
  std::chrono::duration<int, std::milli> mDelay;

  bool contains(std::string const& key) {
    auto el = mStamp.find(key);
    return el != mStamp.end();
  }

 public:
  Watcher(std::string path, std::chrono::duration<int, std::milli> delay)
      : mPath(path), mDelay(delay) {
    for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
      if (file.path().has_extension() && file.path().extension() == ".app") {
        mStamp[file.path().string()] == std::filesystem::last_write_time(file);
      }
    }
  }

  void start(const std::function<void(std::string, Status)>& action) {
    while (true) {
      std::this_thread::sleep_for(mDelay);
      auto it = mStamp.begin();
      while (it != mStamp.end()) {
        if (!std::filesystem::exists(it->first)) {
          action(it->first, Status::REMOVED);
          it = mStamp.erase(it);
        } else {
          it++;
        }
      }

      for (auto& file : std::filesystem::directory_iterator(mPath)) {
        if (!(file.is_regular_file() && file.path().has_extension() &&
              file.path().extension() == ".app")) {
          continue;
        }
        auto curfileWriteTime = std::filesystem::last_write_time(file);
        if (!contains(file.path().string())) {
          mStamp[file.path().string()] = curfileWriteTime;
          action(file.path().string(), Status::CREATED);
        } else {
          if (mStamp[file.path().string()] != curfileWriteTime) {
            mStamp[file.path().string()] = curfileWriteTime;
            action(file.path().string(), Status::MODIFIED);
          }
        }
      }
    }
  }
};

static void push_notification(std::string title, std::string mesg) {
  Executor::execute("/usr/bin/notify-send -i 'software' -a pkgupd '" + title +
                    "' '" + mesg + "'");
}

PKGUPD_MODULE(watchdog) {
  INFO("starting watchdog");
  std::string path = config->get<std::string>(
      "watchdog.dir", std::string(getenv("HOME")) + "/Applications");
  std::filesystem::path app_data =
      config->get<std::string>(DIR_APPS_DATA, DEFAULT_APPS_DIR "/share");

  if (!std::filesystem::exists(path)) {
    ERROR("required path " << path << " not exists, exiting")
    return 1;
  }

  if (!std::filesystem::exists(app_data / "pkgupd")) {
    std::error_code error;
    std::filesystem::create_directories(app_data / "pkgupd", error);
    if (error) {
      ERROR("failed to create pkgupd local data " << error.message());
      return 1;
    }
  }

  AppImageInstaller installer(config);

  Watcher watcher(path, std::chrono::milliseconds(1000));
  watcher.start([&](std::string path, Status status) -> void {
    std::string file_hash = std::to_string(std::hash<std::string>()(path));
    std::string file_datapath = app_data / "pkgupd" / file_hash;

    if (status != Status::REMOVED) {
      // check if file is in use
      FILE* f = fopen(path.c_str(), "rb");
      if (f == nullptr) {
        return;
      }
      fclose(f);

      // check if already installed
      if (std::filesystem::exists(file_datapath)) {
        return;
      }
    }

    auto cleanup = [&app_data](std::string loc) {
      std::ifstream infile(loc);
      std::string dpath;
      while (std::getline(infile, dpath)) {
        dpath = dpath.substr(2);
        if (std::filesystem::exists(dpath) &&
            dpath.find(app_data.string(), 0) == 0) {
          std::error_code error;
          INFO("removing " << dpath);
          std::filesystem::remove(dpath, error);
          if (error) {
            ERROR("failed to remove " << dpath << ", " << error.message());
          }
        }
      }

      std::filesystem::remove(loc);
    };
    switch (status) {
      case Status::MODIFIED: {
        if (std::filesystem::exists(file_datapath)) {
          INFO("clearing previous cache");
          cleanup(file_datapath);
        }
      }

      case Status::CREATED: {
        push_notification(
            "Installing " + std::filesystem::path(path).filename().string(),
            "integrating " + path + " into " + app_data.string());
        INFO("intergrating " << path);
        std::vector<std::string> files;
        if (!installer.intergrate(
                path.c_str(), files,
                [&](mINI::INIStructure& desktopFile) -> void {
                  auto oldAction = desktopFile["Desktop Entry"]["Actions"];
                  std::string id = "Uninstall";
                  if (oldAction.back() != ';') id = ";" + id;

                  desktopFile["Desktop Entry"]["Actions"] += id + ";";
                  desktopFile["Desktop Action Uninstall"]["Name"] = "Uninstall";
                  desktopFile["Desktop Action Uninstall"]["Exec"] =
                      "rm " + path;
                })) {
          push_notification("Integration failed for " +
                                std::filesystem::path(path).filename().string(),
                            "Got error " + installer.error());
          ERROR("failed to integrate " << path);
          return;
        }

        std::ofstream outfile(file_datapath);
        for (auto const& i : files) {
          outfile << i << std::endl;
        }
        outfile.close();

        for(auto const& i : {"home", "config"}) {
          if (!std::filesystem::exists(path +"." + i)) {
            std::error_code error;
            std::filesystem::create_directories(path + "." + i);          
          }
        }
        

        Executor::execute("update-desktop-database " +
                          (app_data / "applications").string());
        return;
      } break;

      case Status::REMOVED: {
        push_notification(
            "removing " + std::filesystem::path(path).filename().string(),
            "clearing data from " + app_data.string());
        INFO("removing " << path);
        if (std::filesystem::exists(file_datapath)) {
          cleanup(file_datapath);
        }
        Executor::execute("update-desktop-database " +
                          (app_data / "applications").string());
      }
    }
  });
  return 1;
}