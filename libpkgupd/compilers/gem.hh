#ifndef LIBPKGUPD_GEM
#define LIBPKGUPD_GEM

#include "../builder.hh"

namespace rlxos::libpkgupd {
class Gem : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir,
               std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif