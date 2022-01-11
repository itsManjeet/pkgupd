#include "resolver.hh"
namespace rlxos::libpkgupd {
bool Resolver::_to_skip(std::string const &pkgid) {
  if ((std::find(m_PackagesList.begin(), m_PackagesList.end(), pkgid) !=
       m_PackagesList.end()))
    return true;

  if (!m_SystemDatabase[pkgid]) return true;

  if ((std::find(m_Visited.begin(), m_Visited.end(), pkgid) != m_Visited.end()))
    return true;

  m_Visited.push_back(pkgid);
  return false;
}
bool Resolver::resolve(std::string const &pkgid, bool all) {
  if (_to_skip(pkgid)) return true;

  auto pkginfo_ = m_Repository[pkgid];
  if (!pkginfo_) {
    _error = "missing required dependency '" + pkgid + "'";
    return false;
  }

  for (auto const &i : pkginfo_->depends()) {
    if (!resolve(i, all)) {
      _error += "\n Trace Required by " + pkgid;
      return false;
    }
  }

  if ((std::find(m_PackagesList.begin(), m_PackagesList.end(), pkgid) ==
       m_PackagesList.end()))
    m_PackagesList.push_back(pkgid);

  return true;
}
}  // namespace rlxos::libpkgupd