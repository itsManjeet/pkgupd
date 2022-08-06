#ifndef _LIBPKGUPD_COMPILERS_SYSTEM_HH_
#define _LIBPKGUPD_COMPILERS_SYSTEM_HH_

#include "../builder.hh"
namespace rlxos::libpkgupd {
class System : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif