#ifndef LIBPKGUPD_CMAKE
#define LIBPKGUPD_CMAKE

#include "../builder/builder.hxx"

namespace rlxos::libpkgupd {
    class CMake : public Compiler {
    protected:
        bool compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ);
    };
}  // namespace rlxos::libpkgupd

#endif