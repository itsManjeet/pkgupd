#include "makefile.hxx"

#include "../exec.hxx"
#include "../recipe.hxx"

namespace rlxos::libpkgupd {
    bool Makefile::compile(Recipe *recipe, Configuration *config, std::string dir,
                           std::string destdir, std::vector<std::string> &environ) {
        // Do build
        int status =
                Executor().execute("make ${MAKEFLAGS} " + recipe->compile().length() ? recipe->compile() : "", dir,
                                   environ);
        if (status != 0) {
            p_Error = "failed to build with makefile";
            return false;
        }

        status = Executor().execute(
                "make ${MAKEFLAGS} STRIP=true PREFIX=/usr DESTDIR=${pkgupd_pkgdir} " +
                recipe->install().length() ? recipe->install() : "install",
                dir, environ);
        if (status != 0) {
            p_Error = "failed to do install with makefile";
            return false;
        }

        return true;
    }
}  // namespace rlxos::libpkgupd