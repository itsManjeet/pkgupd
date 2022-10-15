#ifndef LIBPKGUPD_MAKEFILE
#define LIBPKGUPD_MAKEFILE

#include "../Compiler.hxx"

namespace libpkgupd {
class Makefile : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace libpkgupd

#endif