#ifndef LIBPKGUPD_SCRIPT
#define LIBPKGUPD_SCRIPT

#include "../builder/builder.hxx"

namespace rlxos::libpkgupd {
    class Script : public Compiler {
    protected:
        bool compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ);
    };
}  // namespace rlxos::libpkgupd

#endif