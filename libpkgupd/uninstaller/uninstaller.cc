#include "uninstaller.hh"
using namespace rlxos::libpkgupd;

bool Uninstaller::uninstall(InstalledPackageInfo* pkginfo,
                            SystemDatabase* sys_db) {
  std::string root_dir;
  std::vector<std::string> package_files;

  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  if (!sys_db->get_files(pkginfo, package_files)) {
    p_Error = sys_db->error();
    return false;
  }
  for (auto file : package_files) {
    auto filepath = std::filesystem::path(root_dir);
    if (file.find("./", 0) == 0) {
      filepath /= file.substr(2, file.length() - 2);
    } else {
      filepath /= file;
    }

    std::error_code error;
    try {
      if (std::filesystem::exists(filepath)) {
        if (std::filesystem::is_directory(filepath) &&
            std::filesystem::is_empty(filepath)) {
          std::filesystem::remove_all(filepath, error);
        } else if (!std::filesystem::is_directory(filepath)) {
          std::filesystem::remove(filepath, error);
        }

        if (error) {
          std::cerr << "FAILED TO REMOVE " << filepath << " : "
                    << error.message() << std::endl;
        }
      }
    } catch (std::exception const& exc) {
      std::cerr << "FAILED TO REMOVE " << filepath << " : " << exc.what()
                << std::endl;
    }
  }

  if (!sys_db->remove(pkginfo->id().c_str())) {
    p_Error = sys_db->error();
    return false;
  }
  return true;
}