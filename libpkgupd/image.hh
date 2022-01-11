#ifndef _PKGUPD_IMAGE_HH_
#define _PKGUPD_IMAGE_HH_

#include "packager.hh"

namespace rlxos::libpkgupd {
class Image : public Packager {
 public:
  Image(std::string const& p);

  std::tuple<int, std::string> get(std::string const& filepath);

  std::optional<Package> info();

  std::vector<std::string> list();

  bool extract(std::string const& outdir);

  bool compress(std::string const& srcdir, Package const& info);
};
}  // namespace rlxos::libpkgupd

#endif