#include "cmake.hxx"

#include "../exec.hxx"
#include "../recipe.hxx"

namespace rlxos::libpkgupd {
    bool CMake::compile(Recipe *recipe, Configuration *config, std::string dir,
                        std::string destdir, std::vector<std::string> &environ) {
        auto configure = recipe->configure();
        if (configure.find("-DCMAKE_INSTALL_PREFIX") == std::string::npos) {
            configure =
                    " -DCMAKE_INSTALL_PREFIX=" +
                    config->get<std::string>(BUILD_CONFIG_PREFIX, DEFAULT_PREFIX) +
                    " -DCMAKE_INSTALL_SYSCONFDIR=" +
                    config->get<std::string>(BUILD_CONFIG_SYSCONFDIR, DEFAULT_SYSCONFDIR) +
                    " -DCMAKE_INSTALL_LIBDIR=" +
                    config->get<std::string>(BUILD_CONFIG_LIBDIR, DEFAULT_LIBDIR) +
                    " -DCMAKE_INSTALL_LIBEXECDIR=" +
                    config->get<std::string>(BUILD_CONFIG_LIBEXECDIR, DEFAULT_LIBEXECDIR) +
                    " -DCMAKE_INSTALL_BINDIR=" +
                    config->get<std::string>(BUILD_CONFIG_BINDIR, DEFAULT_BINDIR) +
                    " -DCMAKE_INSTALL_SBINDIR=" +
                    config->get<std::string>(BUILD_CONFIG_SBINDIR, DEFAULT_SBINDIR) +
                    " -DCMAKE_INSTALL_DATADIR=" +
                    config->get<std::string>(BUILD_CONFIG_DATADIR, DEFAULT_DATADIR) +
                    " -DCMAKE_INSTALL_LOCALSTATEDIR=" +
                    config->get<std::string>(BUILD_CONFIG_LOCALSTATEDIR,
                                             DEFAULT_LOCALSTATEDIR) +
                    " " + configure;
        }

        if (int status = Executor().execute("cmake -B _pkgupd_builddir " + configure,
                                            dir, environ);
                status != 0) {
            p_Error = "failed to do cmake configuration";
            return false;
        }

        if (int status = Executor().execute(
                    "cmake --build _pkgupd_builddir " + recipe->compile(), dir, environ);
                status != 0) {
            p_Error = "failed to build with cmake";
            return false;
        }

        if (int status = Executor().execute(
                    "cmake --install _pkgupd_builddir " + recipe->install(), dir, environ);
                status != 0) {
            p_Error = "failed to install with cmake";
            return false;
        }

        return true;
    }
}  // namespace rlxos::libpkgupd