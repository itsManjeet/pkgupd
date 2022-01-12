#include "image.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace rlxos::libpkgupd {

std::tuple<int, std::string> Image::get(std::string const& _filepath) {
  std::string filepath = _filepath;
  if (filepath.substr(0, 2) == "./") {
    filepath = filepath.substr(2, filepath.length() - 2);
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
  return {"/apps/" + std::filesystem::path(m_PackageFile).filename().string()};
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
  std::string appdir = outdir + "/apps/";

  std::filesystem::copy(
      m_PackageFile,
      outdir + "/apps/" +
          std::filesystem::path(m_PackageFile).filename().string(),
      std::filesystem::copy_options::overwrite_existing, err);
  if (err) {
    p_Error = "failed to install " + m_PackageFile + ", " + err.message();
    return false;
  }

  return true;
}

Image::Image(std::string const& p) : Packager(p) {}

}  // namespace rlxos::libpkgupd
