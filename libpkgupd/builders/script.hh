#ifndef LIBPKGUPD_SCRIPT
#define LIBPKGUPD_SCRIPT

#include "../builder.hh"
namespace rlxos::libpkgupd {
class Script : public Builder {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir, std::vector<std::string> const& environ);
};
}  // namespace rlxos::libpkgupd

#endif