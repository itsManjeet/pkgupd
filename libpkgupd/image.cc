#include "image.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace rlxos::libpkgupd {

std::tuple<int, std::string> image::getdata(std::string const& _filepath) {
  std::string filepath = _filepath;
  if (filepath.substr(0, 2) == "./") {
    filepath = filepath.substr(2, filepath.length() - 2);
  }

  auto [status, output] =
      exec().output(_pkgfile + " --appimage-extract " + filepath, "/tmp/");
  if (status != 0) {
    return {status, "failed to get data from " + _pkgfile};
  }

  std::ifstream file("/tmp/squashfs-root/" + filepath);
  if (!file.good()) {
    return {1, filepath + " file is missing"};
  }

  std::filesystem::remove_all("/tmp/squashfs-root");

  return {0, std::string((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>())};
}

std::shared_ptr<image::package> image::info() {
  auto [status, content] = getdata("./info");
  if (status != 0) {
    _error = "failed to read package information " + std::to_string(status) +
             ", " + content;
    return nullptr;
  }

  DEBUG("info: " << content);
  YAML::Node data;

  try {
    data = YAML::Load(content);
  } catch (YAML::Exception const& e) {
    _error = "corrupt package data, " + std::string(e.what());
    return nullptr;
  }

  return std::make_shared<image::package>(data, _pkgfile);
}

std::vector<std::string> image::list() {
  return {"/apps/" + std::filesystem::path(_pkgfile).filename().string()};
}

bool image::compress(std::string const& srcdir,
                     std::shared_ptr<pkginfo> const& info) {
  std::string pardir = std::filesystem::path(_pkgfile).parent_path();
  if (!std::filesystem::exists(pardir)) {
    std::error_code err;
    std::filesystem::create_directories(pardir, err);
    if (err) {
      _error = "failed to create " + pardir + ", " + err.message();
      return false;
    }
  }

  std::ofstream fileptr(srcdir + "/info");

  fileptr << "id: " << info->id() << "\n"
          << "version: " << info->version() << "\n"
          << "about: " << info->about() << "\n";

  if (info->depends(false).size()) {
    fileptr << "depends:"
            << "\n";
    for (auto const& i : info->depends(false)) fileptr << " - " << i << "\n";
  }

  if (info->users().size()) {
    fileptr << "users: " << std::endl;
    for (auto const& i : info->users()) {
      i->print(fileptr);
    }
  }

  if (info->groups().size()) {
    fileptr << "groups: " << std::endl;
    for (auto const& i : info->groups()) {
      i->print(fileptr);
    }
  }

  if (info->install_script().size()) {
    fileptr << "install_script: | " << std::endl;
    std::stringstream ss(info->install_script());
    std::string line;
    while (std::getline(ss, line, '\n')) fileptr << "  " << line << std::endl;
  }

  fileptr.close();

  std::shared_ptr<recipe::package> _pkg =
      std::dynamic_pointer_cast<recipe::package>(info);

  auto lib_flag = _pkg->getflag("copy-libs");
  if (lib_flag != nullptr && lib_flag->value() == "yes") {
    std::set<std::string> req_libs = _list_req(srcdir);
    std::filesystem::path libdir = std::filesystem::path(srcdir) / "lib";
    std::error_code err;
    std::filesystem::create_directories(libdir);
    if (err) {
      _error = err.message();
      return false;
    }

    for (auto const& i : req_libs) {
      if (std::filesystem::exists(libdir /
                                  std::filesystem::path(i).filename())) {
        continue;
      }
      if (!std::filesystem::exists(i)) {
        ERROR("Missing " << i);
        continue;
      }
      DEBUG("copying " << i);
      std::error_code err;
      std::filesystem::copy_file(
          i, libdir / std::filesystem::path(i).filename(), err);
      if (err) {
        _error = err.message();
        return false;
      }
    }
  }

  {
    DEBUG("writing AppRun")

    std::string appRun_Path = std::filesystem::path(srcdir) / "AppRun";
    if (!std::filesystem::exists(appRun_Path)) {
      std::ofstream file(appRun_Path);
      if (_pkg->parent()->node()["AppRun"]) {
        file << _pkg->parent()->node()["AppRun"].as<std::string>();
      } else {
        _error = "no AppRun file found";
        return false;
      }
    }

    if (int status = chmod((srcdir + "/AppRun").c_str(), 0755); status != 0) {
      ERROR("failed to set executable permission on AppRun");
      return false;
    }
  }

  {
    DEBUG("writing desktop file")
    std::string desktopfilePath =
        std::filesystem::path(srcdir) / (_pkg->parent()->id() + ".desktop");
    if (!std::filesystem::exists(desktopfilePath)) {
      if (_pkg->parent()->node()["Desktopfile"]) {
        std::ofstream file(desktopfilePath);
        file << _pkg->parent()->node()["Desktopfile"].as<std::string>();
      } else {
        _error = "no desktop file exist";
        return false;
      }
    }
  }

  {
    DEBUG("packing AppImage")
    int status = exec().execute(
        "appimagetool --sign " + srcdir + " " + _pkgfile, ".", {"ARCH=x86_64"});
    if (status != 0) {
      _error = "failed to pack appimage";
      return false;
    }
  }

  return true;
}

bool image::install_icon(std::string const& outdir) {
  std::string icon_dest_dir =
      std::filesystem::path(outdir) / "apps" / "share" / "pixmaps";

  if (!std::filesystem::exists(icon_dest_dir)) {
    std::error_code err;
    std::filesystem::create_directories(icon_dest_dir, err);
    if (err) {
      _error = "failed to create pixmap directory " + err.message();
      return false;
    }
  }

  auto [status, data] = getdata("./" + _package->id() + ".png");
  if (status != 0) {
    _error = "no icon file found " + data;
    return false;
  }

  std::ofstream iconfile(icon_dest_dir + "/" + _package->id() + ".png",
                         std::ios_base::binary);
  iconfile << data;
  iconfile.close();
  return true;
}

bool image::install_desktopfile(std::string const& outdir) {
  std::string deskfile_dest_dir =
      std::filesystem::path(outdir) / "apps" / "share" / "applications";
  if (!std::filesystem::exists(deskfile_dest_dir)) {
    std::error_code err;
    std::filesystem::create_directories(deskfile_dest_dir, err);
    if (err) {
      _error = "failed to create applications directory " + err.message();
      return false;
    }
  }

  auto [status, data] = getdata("./" + _package->id() + ".desktop");
  if (status != 0) {
    _error = "no desktop file found " + data;
    return false;
  }

  std::ofstream deskfile(deskfile_dest_dir + "/" + _package->id() + ".desktop");
  std::stringstream ss(data);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    if (line.length() == 0) {
      continue;
    }

    if (line.find("Exec=", 0) == 0) {
      auto del_index = line.find_first_of(' ');
      if (del_index == std::string::npos) {
        line = "Exec=/apps/" + _package->packagefile();
      } else {
        line = "Exec=/apps/" + _package->packagefile() +
               line.substr(del_index, line.length() - del_index);
      }
    } else if (line.find("Icon=", 0) == 0) {
      line = "Icon=/apps/share/pixmaps/" + _package->id() + ".png";
    }

    deskfile << line << std::endl;
  }

  return true;
}

bool image::extract(std::string const& outdir) {
  std::error_code err;
  std::string appdir = outdir + "/apps/";

  std::filesystem::copy(
      _pkgfile,
      outdir + "/apps/" + std::filesystem::path(_pkgfile).filename().string(),
      std::filesystem::copy_options::overwrite_existing, err);
  if (err) {
    _error = "failed to install " + _pkgfile + ", " + err.message();
    return false;
  }

  if (_package == nullptr) {
    _package = info();
  }

  if (!install_icon(outdir)) {
    return false;
  }

  if (!install_desktopfile(outdir)) {
    return false;
  }

  return true;
}

image::image(std::string const& p) : archive(p) {}

std::string image::_mimetype(std::string const& path) {
  auto [status, output] =
      exec().output("file --mime-type " + path + " | awk '{print $2}'");
  if (status != 0) {
    return "";
  }

  return output.substr(0, output.size() - 1);
}

std::set<std::string> image::_list_lib(std::string const& path) {
  std::string lib_env;
  if (_libpath.size()) {
    lib_env = "LD_LIBRARY_PATH=";
    std::string sep;
    for (auto const& i : _libpath) {
      lib_env += sep + i;
      sep = ":";
    }
  }

  auto [status, output] =
      exec().output(lib_env + " ldd " + path + " | awk '{print $3}'");
  if (status != 0) {
    return {};
  }

  std::stringstream ss(output);
  std::set<std::string> list;
  std::string lib;
  while (std::getline(ss, lib, '\n')) {
    if (lib.length() == 0 || lib == "not") {
      continue;
    }
    DEBUG("found " + lib);
    list.insert(lib);
  }

  return list;
}

std::set<std::string> image::_list_req(std::string const& appdir) {
  std::set<std::string> list;

  for (auto const& d : std::filesystem::recursive_directory_iterator(appdir)) {
    if (d.is_directory()) {
      continue;
    }

    std::string _mime = _mimetype(d.path());
    if (_mime == "application/x-executable" ||
        _mime == "application/x-sharedlib") {
      std::set<std::string> _loc_list = _list_lib(d.path());
      list.insert(_loc_list.begin(), _loc_list.end());
    }
  }

  return list;
}

}  // namespace rlxos::libpkgupd
