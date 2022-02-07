#include "image.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace rlxos::libpkgupd {
auto const AppRun = R"END(#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH=${HERE}:${HERE}/usr/bin:${HERE}/bin:${HERE}/usr/sbin:${HERE}/sbin:${PATH}
export LD_LIBRARY_PATH=${HERE}:${HERE}/usr/lib:${HERE}/usr/lib/x86_64-linux-gnu:${HERE}/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}
export XDG_DATA_DIRS=${HERE}/usr/share:${XDG_DATA_DIRS}
export GSETTINGS_SCHEMA_DIR=${HERE}/usr/share/glib-2.0/schemas:${GSETTINGS_SCHEMA_DIR}
EXEC=$(grep -e '^Exec=.*' "${HERE}"/*.desktop | head -n 1 | cut -d "=" -f 2 | cut -d " " -f 1)
exec "${EXEC}" "$@"
)END";

std::tuple<int, std::string> Image::get(std::string const& _filepath) {
  std::string filepath = _filepath;
  if (filepath.substr(0, 2) == "./") {
    filepath = filepath.substr(2, filepath.length() - 2);
  }

  if (chmod(m_PackageFile.c_str(), 0755) != 0) {
    p_Error = "failed to set executable permission on '" + m_PackageFile + "'";
    return {1, p_Error};
  }

  auto [status, output] = Executor().output(
      m_PackageFile + " --appimage-extract " + filepath, "/tmp/");
  if (status != 0) {
    return {status, "failed to get data from " + m_PackageFile};
  }

  std::ifstream file("/tmp/squashfs-root/" + filepath);
  if (!file.good()) {
    return {1, filepath + " file is missing"};
  }

  std::filesystem::remove_all("/tmp/squashfs-root");

  return {0, std::string((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>())};
}

std::optional<Package> Image::info() {
  auto [status, content] = get("./info");
  if (status != 0) {
    p_Error = "failed to read package information " + std::to_string(status) +
              ", " + content;
    return {};
  }

  DEBUG("info: " << content);
  YAML::Node data;

  try {
    data = YAML::Load(content);
  } catch (YAML::Exception const& e) {
    p_Error = "corrupt package data, " + std::string(e.what());
    return {};
  }

  return Package(data, m_PackageFile);
}

std::vector<std::string> Image::list() {
  return {"/apps/" + std::filesystem::path(m_PackageFile).filename().string(),
          "/apps/share/pixmaps/" + info()->id() + ".png",
          "/apps/share/applications/" + info()->id() + ".desktop"};
}

bool Image::compress(std::string const& srcdir, Package const& package) {
  std::string pardir = std::filesystem::path(m_PackageFile).parent_path();
  if (!std::filesystem::exists(pardir)) {
    std::error_code err;
    std::filesystem::create_directories(pardir, err);
    if (err) {
      p_Error = "failed to create " + pardir + ", " + err.message();
      return false;
    }
  }

  {
    // Write AppRun
    auto apprun = std::filesystem::path(srcdir) / "AppRun";
    if (!std::filesystem::exists(apprun)) {
      std::ofstream file(apprun);
      if (package.node()["AppRun"]) {
        file << package.node()["AppRun"].as<std::string>();
      } else {
        file << AppRun;
      }
      file.close();
    }
  }

  {
    // Write desktopfile
    auto desktopfile =
        std::filesystem::path(srcdir) / (package.id() + ".desktop");

    if (!std::filesystem::exists(desktopfile)) {
      std::ofstream file(desktopfile);
      if (package.node()["DesktopFile"]) {
        file << package.node()["DesktopFile"].as<std::string>();
      } else {
        p_Error = "no desktop file found '" + desktopfile.string() + "'";
        return false;
      }
    }
  }

  {
    DEBUG("packing AppImage")
    int status = Executor().execute(
        "appimagetool --sign " + srcdir + " " + m_PackageFile, ".",
        {"ARCH=x86_64"});
    if (status != 0) {
      p_Error = "failed to pack appimage";
      return false;
    }
  }

  return true;
}

bool Image::extract(std::string const& outdir) {
  std::error_code err;
  std::string appdir = outdir + "/apps";

  std::string appdir_desktop = appdir + "/share/applications";
  std::string appdir_pixmap = appdir + "/share/pixmaps";

  for (auto const& dir : {appdir, appdir_desktop, appdir_pixmap}) {
    std::filesystem::create_directories(dir, err);
    if (err) {
      p_Error = "failed to create app dir: " + dir + ", " + err.message();
      return false;
    }
  }

  auto output_file =
      "/apps/" + std::filesystem::path(m_PackageFile).filename().string();

  std::filesystem::copy(m_PackageFile, outdir + output_file,
                        std::filesystem::copy_options::overwrite_existing, err);
  if (err) {
    p_Error = "failed to install " + m_PackageFile + ", " + err.message();
    return false;
  }

  auto package_info = info();

  // Installing desktop file
  {
    std::string desktop_file =
        appdir_desktop + "/" + package_info->id() + ".desktop";
    auto [status, data] = get("./" + package_info->id() + ".desktop");
    if (status != 0) {
      p_Error = data;
      return false;
    }

    std::stringstream ss(data);
    std::string line;
    std::ofstream file(desktop_file);
    while (std::getline(ss, line)) {
      if (!line.size()) {
        continue;
      }

      if (line.find("Exec=", 0) == 0) {
        auto idx = line.find_first_of('=');
        auto space = line.find_first_of(' ');

        auto newline = line.substr(0, idx) + "=" + output_file;
        if (space != std::string::npos) {
          newline += line.substr(space, line.length() - space);
        }
        line = newline;
      } else if (line.find("Icon=", 0) == 0) {
        line = "Icon=/apps/share/pixmaps/" + package_info->id() + ".png";
      }
      file << line << std::endl;
    }
    file.close();
  }

  // Install icon
  {
    auto [status, data] = get("./" + package_info->id() + ".png");
    if (status != 0) {
      p_Error = data;
      return false;
    }

    std::ofstream file(appdir_pixmap + "/" + package_info->id() + ".png");
    file << data;
    file.close();
  }

  return true;
}

Image::Image(std::string const& p) : Packager(p) {}

}  // namespace rlxos::libpkgupd
