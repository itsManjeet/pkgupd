#include "system-database.hh"

#include <string.h>
#include <sys/stat.h>

#include <ctime>
#include <fstream>

#include "defines.hh"

namespace rlxos::libpkgupd {

InstalledPackageInfo::InstalledPackageInfo(YAML::Node const &data,
                                           char const *file)
    : PackageInfo(data, file) {
  READ_LIST(std::string, "files", mFiles);
  READ_VALUE(std::string, "installed_on", mInstalledon);
}

bool SystemDatabase::list_all(std::vector<std::string> &ids) {
  if (std::filesystem::exists(data_dir)) {
    for (auto const &i : std::filesystem::directory_iterator(data_dir)) {
      ids.push_back(i.path().filename().string());
    }
  }
  return true;
}
std::shared_ptr<InstalledPackageInfo> SystemDatabase::get(char const *pkgid) {
  auto datafile = std::filesystem::path(data_dir) / std::string(pkgid);
  if (!std::filesystem::exists(datafile)) {
    p_Error = "no package found with name '" + std::string(pkgid) +
              "' in system database";
    return nullptr;
  }

  return std::make_shared<InstalledPackageInfo>(YAML::LoadFile(datafile),
                                                datafile.c_str());
}

std::shared_ptr<InstalledPackageInfo> SystemDatabase::add(
    PackageInfo *pkginfo, std::vector<std::string> const &files,
    std::string root, bool update) {
  auto data_file = std::filesystem::path(data_dir) / pkginfo->id();
  if (!std::filesystem::exists(data_dir)) {
    std::error_code code;
    if (!std::filesystem::create_directories(data_dir, code)) {
      p_Error = "failed to create required data directory, " + code.message();
      return nullptr;
    }
  }

  std::ofstream file(data_file);
  if (!file.is_open()) {
    p_Error = "failed to open data file to write at " + data_file.string();
    return nullptr;
  }

  pkginfo->dump(file);

  if (files.size()) {
    file << "files:\n";
    for (auto const &f : files) {
      file << " - " << f << '\n';
    }
  }

  std::time_t t = std::time(0);
  std::tm *now = std::localtime(&t);

  file << "installed_on: " << (now->tm_year + 1900) << "/" << (now->tm_mon + 1)
       << "/" << (now->tm_mday) << " " << (now->tm_hour) << ":" << (now->tm_min)
       << std::endl;

  file.close();
  return std::make_shared<InstalledPackageInfo>(pkginfo, files);
}

bool SystemDatabase::remove(InstalledPackageInfo *pkginfo) {
  auto data_file = std::filesystem::path(data_dir) / pkginfo->id();
  std::error_code code;
  if (!std::filesystem::exists(data_file)) {
    return true;
  }

  std::filesystem::remove(data_file, code);
  if (code) {
    p_Error = code.message();
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd