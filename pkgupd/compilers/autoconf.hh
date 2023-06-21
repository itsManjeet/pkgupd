#ifndef LIBPKGUPD_AUTOCONF
#define LIBPKGUPD_AUTOCONF

#include "../builder.hh"
namespace rlxos::libpkgupd {
class AutoConf : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace rlxos::libpkgupd

#endif