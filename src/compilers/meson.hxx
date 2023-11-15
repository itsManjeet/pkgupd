#ifndef LIBPKGUPD_MESON
#define LIBPKGUPD_MESON

#include "../builder/builder.hxx"

namespace rlxos::libpkgupd {
    class Meson : public Compiler {
    protected:
        bool compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ);
    };
}  // namespace rlxos::libpkgupd

#endif