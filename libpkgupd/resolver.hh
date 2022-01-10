#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>

#include "defines.hh"
#include "repodb.hh"
#include "sysdb.hh"

namespace rlxos::libpkgupd {
class Resolver : public Object {
 private:
  Repository &_repodb;
  SystemDatabase &_sysdb;
  std::vector<std::string> _data, _visited;

  bool _to_skip(std::string const &pkgid);

 public:
  Resolver(Repository &rp, SystemDatabase &sd) : _repodb{rp}, _sysdb{sd} {}

  bool resolve(std::string const &pkgid, bool all = false);

  GET_METHOD(std::vector<std::string>, data);
};
}  // namespace rlxos::libpkgupd

#endif