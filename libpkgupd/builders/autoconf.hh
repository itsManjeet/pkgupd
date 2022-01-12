#ifndef LIBPKGUPD_AUTOCONF
#define LIBPKGUPD_AUTOCON

#include "../builder.hh"
namespace rlxos::libpkgupd {
class AutoConf : public Builder {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir, std::vector<std::string> const& environ);
};
}  // namespace rlxos::libpkgupd

#endif