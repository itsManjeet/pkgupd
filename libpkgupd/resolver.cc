#include "resolver.hh"
namespace rlxos::libpkgupd {
bool resolver::_to_skip(std::string const &pkgid) {
  if ((std::find(_data.begin(), _data.end(), pkgid) != _data.end()))
    return true;

  if (_sysdb[pkgid] != nullptr) return true;

  if ((std::find(_visited.begin(), _visited.end(), pkgid) != _visited.end()))
    return true;

  _visited.push_back(pkgid);
  return false;
}
bool resolver::resolve(std::string const &pkgid, bool all) {
  if (_to_skip(pkgid)) return true;

  auto pkginfo_ = _repodb[pkgid];
  if (pkginfo_ == nullptr) {
    _error = "missing required dependency '" + pkgid + "'";
    return false;
  }

  for (auto const &i : pkginfo_->depends(all)) {
    if (!resolve(i, all)) {
      _error += "\n Trace Required by " + pkgid;
      return false;
    }
  }

  if ((std::find(_data.begin(), _data.end(), pkgid) == _data.end()))
    _data.push_back(pkgid);

  return true;
}
}  // namespace rlxos::libpkgupd