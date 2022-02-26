#ifndef LIBPKGUPD_PYSETUP
#define LIBPKGUPD_PYSETUP

#include "../builder.hh"

namespace rlxos::libpkgupd {
class PySetup : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir,
               std::vector<std::string> const& environ);
};
}  // namespace rlxos::libpkgupd

#endif