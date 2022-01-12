#ifndef LIBPKGUPD_MESON
#define LIBPKGUPD_MESON

#include "../builder.hh"
namespace rlxos::libpkgupd {
class Meson : public Builder {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir, std::vector<std::string> const& environ);
};
}  // namespace rlxos::libpkgupd

#endif