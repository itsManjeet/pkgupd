#ifndef _LIBPKGUPD_REMOVER_HH_
#define _LIBPKGUPD_REMOVER_HH_

#include "defines.hh"
#include "system-database.hh"
#include "triggerer.hh"

namespace rlxos::libpkgupd {
class Remover : public Object {
 private:
  std::string m_RootDir;
  SystemDatabase &m_SystemDatabase;
  Triggerer m_Triggerer;

  std::vector<std::vector<std::string>> m_FilesList;

  bool _skip_trigger = false;

 public:
  Remover(SystemDatabase &systemDatabase, std::string const &rootdir)
      : m_SystemDatabase(systemDatabase), m_RootDir(rootdir) {}

  bool remove(Package const &pkginfo);

  bool remove(std::vector<std::string> const &pkgs, bool skip_triggers = false);
};
}  // namespace rlxos::libpkgupd

#endif