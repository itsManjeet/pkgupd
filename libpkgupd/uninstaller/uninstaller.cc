#include "uninstaller.hh"
using namespace rlxos::libpkgupd;

bool Uninstaller::uninstall(InstalledPackageInfo* pkginfo,
                            SystemDatabase* sys_db) {
  std::string root_dir;
  std::vector<std::string> files;

  root_dir = mConfig->get<std::string>(DIR_ROOT, DEFAULT_ROOT_DIR);
  files = pkginfo->files();

  for (auto file = files.rbegin(); file != files.rend(); ++file) {
    auto filepath = std::filesystem::path(root_dir);
    if ((*file).find("./", 0) == 0) {
      filepath /= (*file).substr(2, (*file).length() - 2);
    } else {
      filepath /= *file;
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

  return sys_db->remove(pkginfo);
}