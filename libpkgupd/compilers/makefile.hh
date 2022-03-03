#ifndef LIBPKGUPD_MAKEFILE
#define LIBPKGUPD_MAKEFILE

#include "../builder.hh"

namespace rlxos::libpkgupd {
class Makefile : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir,
               std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif