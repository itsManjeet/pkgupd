#ifndef LIBPKGUPD_TAR
#define LIBPKGUPD_TAR

#include "packager.hh"

namespace rlxos::libpkgupd {
class Tar : public Packager {
 public:
  Tar(std::string const &p) : Packager(p) {}
  std::tuple<int, std::string> get(std::string const &filepath);

  std::optional<Package> info();

  std::vector<std::string> list();

  bool extract(std::string const &outdir);

  bool compress(std::string const &srcdir, Package const &info);
};
}  // namespace rlxos::libpkgupd
#endif