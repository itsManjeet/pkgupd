#ifndef _INSTALLER_HH_
#define _INSTALLER_HH_

#include "defines.hh"
#include "downloader.hh"
#include "repodb.hh"
#include "sysdb.hh"

namespace rlxos::libpkgupd {
class installer : public object {
 private:
  sysdb _sysdb;
  repodb _repodb;
  downloader _downloader;

  std::string _pkg_dir;

  bool _install(std::vector<std::string> const &pkgs,
                std::string const &root_dir,
                bool skip_triggers,
                bool force);

 public:
  installer(sysdb &sdb,
            repodb &rdb,
            downloader &dwn,
            std::string const &pkgdir)
      : _sysdb{sdb},
        _repodb{rdb},
        _downloader{dwn},
        _pkg_dir{pkgdir} {
  }

  bool install(std::vector<std::string> const &pkgs,
               std::string const &root_dir,
               bool skip_triggers,
               bool force);
};
}  // namespace rlxos::libpkgupd

#endif