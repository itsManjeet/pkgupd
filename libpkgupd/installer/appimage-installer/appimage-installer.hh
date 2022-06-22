#ifndef LIBPKGUPD_APPIMAGE_INSTALLER_HH
#define LIBPKGUPD_APPIMAGE_INSTALLER_HH

#include "../../archive-manager/archive-manager.hh"
#include "../installer.hh"
#include "external/ini.h"

namespace rlxos::libpkgupd {
class AppImageInstaller : public Installer::Injector {
 private:
  bool patch(std::string filepath,
             std::map<std::string, std::string> replaces = {});

  bool extract(ArchiveManager* archiveManager, std::string archive_file,
               std::string filepath, std::string app_dir,
               std::string& target_path);

 public:
  AppImageInstaller(Configuration* config) : Installer::Injector{config} {}

  bool intergrate(char const* path, std::vector<std::string>& files,
                  std::function<void(mINI::INIStructure&)> desktopFileModifier);

  std::shared_ptr<PackageInfo> inject(char const* path,
                                      std::vector<std::string>& files);
};
}  // namespace rlxos::libpkgupd

#endif