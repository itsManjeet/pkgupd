#ifndef _INSTALLER_HH_
#define _INSTALLER_HH_

#include "defines.hh"
#include "downloader.hh"
#include "repository.hh"
#include "system-database.hh"

namespace rlxos::libpkgupd {
class Installer : public Object {
 private:
  SystemDatabase &m_SystemDatabase;
  Repository &m_Repository;
  Downloader &m_Downloader;

  std::string m_PackageDir;

  bool _install(std::vector<std::string> const &pkgs,
               std::string const &root_dir, bool skip_triggers, bool force);

 public:
  Installer(SystemDatabase &systemDatabase, Repository &repository,
            Downloader &downloader, std::string const &packageDir)
      : m_SystemDatabase(systemDatabase),
        m_Repository(repository),
        m_Downloader(downloader),
        m_PackageDir(packageDir) {}

  bool install(std::vector<std::string> const &pkgs,
               std::string const &root_dir, bool skip_triggers, bool force);
};
}  // namespace rlxos::libpkgupd

#endif