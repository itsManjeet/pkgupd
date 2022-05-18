#include "system-database.hh"

#include <string.h>
#include <sys/stat.h>

#include <ctime>
#include <fstream>

namespace rlxos::libpkgupd {

InstalledPackageInfo::InstalledPackageInfo(YAML::Node const &data,
                                           char const *file)
    : PackageInfo(data, file) {
  READ_LIST(std::string, "files", mFiles);
}
std::shared_ptr<InstalledPackageInfo> SystemDatabase::get(char const *pkgid) {
  auto data_dir = mConfig->get<std::string>("data-dir", DEFAULT_DATA_DIR);
  auto datafile = std::filesystem::path(data_dir) / pkgid;
  if (!std::filesystem::exists(datafile)) {
    p_Error = "no package found with name '" + std::string(pkgid) +
              "' in system database";
    return nullptr;
  }

  return std::make_shared<InstalledPackageInfo>(YAML::LoadFile(datafile),
                                                datafile.c_str());
}

bool SystemDatabase::add(PackageInfo *pkginfo,
                         std::vector<std::string> const &files,
                         std::string root, bool update) {
  auto data_dir = mConfig->get<std::string>("data-dir", DEFAULT_DATA_DIR);
  auto data_file = std::filesystem::path(data_dir) / pkginfo->id();

  std::ofstream file(data_file);

  pkginfo->dump(file);

  if (files.size()) {
    file << "files:\n";
    for (auto const &f : files) {
      file << f << '\n';
    }
  }

  std::time_t t = std::time(0);
  std::tm *now = std::localtime(&t);

  file << "installed_on: " << (now->tm_year + 1900) << "/" << (now->tm_mon + 1)
       << "/" << (now->tm_mday) << " " << (now->tm_hour) << ":" << (now->tm_min)
       << std::endl;

  file.close();
  return true;
}

bool SystemDatabase::remove(InstalledPackageInfo *pkginfo) {
  auto data_dir = mConfig->get<std::string>("data-dir", DEFAULT_DATA_DIR);
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