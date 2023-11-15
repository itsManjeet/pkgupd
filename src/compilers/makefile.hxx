#ifndef LIBPKGUPD_MAKEFILE
#define LIBPKGUPD_MAKEFILE

#include "../builder/builder.hxx"

namespace rlxos::libpkgupd {
    class Makefile : public Compiler {
    protected:
        bool compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ);
    };
}  // namespace rlxos::libpkgupd

#endif