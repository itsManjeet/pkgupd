#ifndef LIBPKGUPD_SCRIPT
#define LIBPKGUPD_SCRIPT

#include "../builder.hh"
namespace rlxos::libpkgupd {
class Script : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir, std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif