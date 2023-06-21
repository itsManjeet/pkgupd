#ifndef LIBPKGUPD_COMPILERS_SYSTEM_HH
#define LIBPKGUPD_COMPILERS_SYSTEM_HH

#include "../builder.hxx"

namespace rlxos::libpkgupd {
    class System : public Compiler {
    protected:
        bool compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ) override;
    };
}  // namespace rlxos::libpkgupd

#endif