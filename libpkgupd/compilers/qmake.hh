#ifndef LIBPKGUPD_QMAKE
#define LIBPKGUPD_QMAKE

#include "../builder.hh"

namespace rlxos::libpkgupd {
class QMake : public Compiler {
 protected:
  bool compile(Recipe const& recipe, std::string dir, std::string destdir,
               std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif