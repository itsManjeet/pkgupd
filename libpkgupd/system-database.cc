#include "system-database.hh"

#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <ctime>
#include <fstream>

#include "defines.hh"

namespace rlxos::libpkgupd {

InstalledPackageInfo::InstalledPackageInfo(YAML::Node const &data,
                                           char const *file)
    : PackageInfo(data, file) {
  READ_VALUE(std::string, "installed_on", mInstalledon);
  
  if (data["files"]) mFiles.reserve(data["files"].size());
  READ_LIST(std::string, "files", mFiles);
  OPTIONAL_VALUE(bool, "is-dependency", m_IsDependency, false);
}

bool SystemDatabase::init() {
  mPackages.clear();

  PROCESS("reading system database");
  for (auto const &file : std::filesystem::directory_iterator(data_dir)) {
    if (file.path().has_extension()) continue;
    try {
      mPackages[file.path().filename().string()] =
          std::make_unique<InstalledPackageInfo>(
              YAML::LoadFile(file.path().string()), file.path().c_str());
    } catch (std::exception const &exception) {
      std::cerr << "failed to load: " << exception.what() << std::endl;
    }
  }
  return true;
}

InstalledPackageInfo *SystemDatabase::get(char const *pkgid) {
  auto iter = mPackages.find(pkgid);
  if (iter == mPackages.end()) {
    return nullptr;
  }
  return iter->second.get();
}

InstalledPackageInfo *SystemDatabase::add(PackageInfo *pkginfo,
                                          std::vector<std::string> const &files,
                                          std::string root, bool update,
                                          bool is_dependency) {
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

  file << "is-dependency: " << (is_dependency ? "true" : "false") << "\n";

  std::time_t t = std::time(0);
  std::tm *now = std::localtime(&t);

  file << "installed_on: " << (now->tm_year + 1900) << "/" << (now->tm_mon + 1)
       << "/" << (now->tm_mday) << " " << (now->tm_hour) << ":" << (now->tm_min)
       << std::endl;

  file.close();

  mPackages[pkginfo->id()] =
      std::make_unique<InstalledPackageInfo>(pkginfo, files);

  return mPackages[pkginfo->id()].get();
}

bool SystemDatabase::remove(char const *id) {
  auto data_file = std::filesystem::path(data_dir) / id;
  std::error_code code;
  if (!std::filesystem::exists(data_file)) {
    return true;
  }

  std::filesystem::remove(data_file, code);
  if (code) {
    p_Error = "failed to remove data file: " + data_file.string() + ", " +
              code.message();
    return false;
  }

  // auto iter = mPackages.find(id);
  // if (iter == mPackages.end()) {
  //   return true;
  // }
  // mPackages.erase(iter);
  return true;
}
}  // namespace rlxos::libpkgupd