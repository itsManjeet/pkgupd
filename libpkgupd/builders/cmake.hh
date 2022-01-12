#ifndef LIBPKGUPD_CMAKE
#define LIBPKGUPD_CMAKE

#include "../builder.hh"
namespace rlxos::libpkgupd {
class Cmake : public Builder {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir, std::vector<std::string> const& environ);
};
}  // namespace rlxos::libpkgupd

#endif