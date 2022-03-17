#include "resolver.hh"
namespace rlxos::libpkgupd {
void Resolver::clear() {
  m_PackagesList.clear();
  m_Visited.clear();
}

bool Resolver::_to_skip(std::string const &pkgid) {
  if ((std::find(m_PackagesList.begin(), m_PackagesList.end(), pkgid) !=
       m_PackagesList.end()))
    return true;

  if (m_SystemDatabase[pkgid]) return true;

  if ((std::find(m_Visited.begin(), m_Visited.end(), pkgid) != m_Visited.end()))
    return true;

  m_Visited.push_back(pkgid);
  return false;
}
bool Resolver::resolve(std::string const &pkgid, bool all) {
  if (_to_skip(pkgid)) return true;

  DEBUG("checking " << pkgid);
  auto recipe = m_Repository.recipe(pkgid);

  if (!recipe) {
    p_Error = "missing required dependency '" + pkgid + "'";
    return false;
  }

  auto depends = recipe->depends();
  if (all) {
    auto build_depends = recipe->buildTime();
    depends.insert(depends.begin(), build_depends.begin(), build_depends.end());
  }

  for (auto const &i : depends) {
    DEBUG("depends " << i)
    if (!resolve(i, all)) {
      p_Error += "\n Trace Required by " + pkgid;
      return false;
    }
  }

  if ((std::find(m_PackagesList.begin(), m_PackagesList.end(), pkgid) ==
       m_PackagesList.end()))
    m_PackagesList.push_back(pkgid);

  return true;
}
}  // namespace rlxos::libpkgupd