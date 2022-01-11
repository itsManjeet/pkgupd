#ifndef _LIBPKGUPD_HH_
#define _LIBPKGUPD_HH_

#include "colors.hh"
#include "image.hh"
#include "installer.hh"
#include "packager.hh"
#include "remover.hh"
#include "resolver.hh"
#include "tar.hh"

namespace rlxos::libpkgupd {

struct UpdateInformation {
  Package previous;
  Package updated;
};
class Pkgupd : public Object {
 private:
  SystemDatabase m_SystemDatabase;
  Repository m_Repository;
  Installer m_Installer;
  Remover m_Remover;
  Downloader m_Downloader;

  Resolver m_Resolver;
  Triggerer m_Triggerer;

  std::string m_RootDir;

  bool m_IsForce;
  bool m_IsSkipTriggers;

 public:
  Pkgupd(std::string const& systemDatabasePath,
         std::string const& repositoryPath, std::string const& packagesPath,
         std::vector<std::string> const& mirrors, std::string const& version,
         std::string const& rootsPath, bool isForce = false,
         bool isSkipTriggers = false)
      : m_SystemDatabase(systemDatabasePath),
        m_Repository(repositoryPath),
        m_Installer(m_SystemDatabase, m_Repository, m_Downloader, packagesPath),
        m_RootDir(rootsPath),
        m_Remover(m_SystemDatabase, rootsPath),
        m_Resolver(m_SystemDatabase, m_Repository),
        m_IsForce(isForce),
        m_IsSkipTriggers(isSkipTriggers),
        m_Downloader(mirrors, version) {}

  bool install(std::vector<std::string> const& packages);

  bool remove(std::vector<std::string> const& packages);

  bool update(std::vector<std::string> const& packages);

  std::optional<Package> info(std::string packageName);

  std::vector<UpdateInformation> outdate();

  std::vector<std::string> depends(std::string const& package);

  std::vector<Package> search(std::string query);

  bool trigger(std::vector<std::string> const& packages);

  bool sync();
};
}  // namespace rlxos::libpkgupd
#endif