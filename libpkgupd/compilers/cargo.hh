#ifndef LIBPKGUPD_CARGO
#define LIBPKGUPD_CARGO

#include "../builder.hh"

namespace rlxos::libpkgupd {
class Cargo : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir,
               std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif