#ifndef LIBPKGUPD_PYSETUP
#define LIBPKGUPD_PYSETUP

#include "../builder.hh"

namespace rlxos::libpkgupd {
class PySetup : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif