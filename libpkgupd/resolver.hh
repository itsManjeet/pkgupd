#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>

#include "defines.hh"
#include "repository.hh"
#include "system-database.hh"

namespace rlxos::libpkgupd {
class Resolver : public Object {
 private:
  Repository &m_Repository;
  SystemDatabase &m_SystemDatabase;
  std::vector<std::string> m_PackagesList, m_Visited;

  bool _to_skip(std::string const &pkgid);

 public:
  Resolver(SystemDatabase &systemDatabase, Repository &repository)
      : m_Repository(repository), m_SystemDatabase(systemDatabase) {}

  bool resolve(std::string const &pkgid, bool all = false);

  void clear();

  std::vector<std::string> const &list() const { return m_PackagesList; }
};
}  // namespace rlxos::libpkgupd

#endif