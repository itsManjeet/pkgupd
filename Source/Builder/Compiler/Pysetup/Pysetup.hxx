#ifndef LIBPKGUPD_PYSETUP
#define LIBPKGUPD_PYSETUP

#include "../builder.hxx"

namespace libpkgupd {
class PySetup : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace libpkgupd

#endif