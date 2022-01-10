#ifndef _INSTALLER_HH_
#define _INSTALLER_HH_

#include "defines.hh"
#include "downloader.hh"
#include "repodb.hh"
#include "sysdb.hh"

namespace rlxos::libpkgupd {
class Installer : public Object {
 private:
  SystemDatabase _sysdb;
  Repository _repodb;
  Downloader _downloader;

  std::string _pkg_dir;

  bool _install(std::vector<std::string> const &pkgs,
                std::string const &root_dir, bool skip_triggers, bool force);

 public:
  Installer(SystemDatabase &sdb, Repository &rdb, Downloader &dwn, std::string const &pkgdir)
      : _sysdb{sdb}, _repodb{rdb}, _downloader{dwn}, _pkg_dir{pkgdir} {}

  bool install(std::vector<std::string> const &pkgs,
               std::string const &root_dir, bool skip_triggers, bool force);
};
}  // namespace rlxos::libpkgupd

#endif