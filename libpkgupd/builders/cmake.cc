#include "cmake.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool Cmake::compile(Recipe const& recipe, std::string dir, std::string destdir,
                    std::vector<std::string> const& environ) {
  auto configure = recipe.configure();
  if (configure.find("-DCMAKE_INSTALL_PREFIX") != std::string::npos) {
    configure = " -DCMAKE_INSTALL_PREFIX=" + PREFIX +
                " -DCMAKE_INSTALL_SYSCONFDIR=" + SYSCONF_DIR +
                " -DCMAKE_INSTALL_LIBDIR=" + LIBDIR +
                " -DCMAKE_INSTALL_LIBEXECDIR=" + LIBEXEDIR +
                " -DCMAKE_INSTALL_BINDIR=" + BINDIR +
                " -DCMAKE_INSTALL_SBINDIR=" + SBINDIR +
                " -DCMAKE_INSTALL_DATADIR=" + DATADIR +
                " -DCMAKE_INSTALL_LOCALSTATEDIR=" + CACHEDIR + " " + configure;
  }
  if (int status = Executor().execute("cmake -B _pkgupd_buildir " + configure,
                                      dir, environ);
      status != 0) {
    p_Error = "failed to do cmake configuration";
    return false;
  }

  if (int status = Executor().execute(
          "cmake --build _pkgupd_builddir " + recipe.compile(), dir, environ);
      status != 0) {
    p_Error = "failed to build with cmake";
    return false;
  }

  if (int status = Executor().execute(
          "cmake --install _pkgupd_builddir " + recipe.install(), dir, environ);
      status != 0) {
    p_Error = "failed to install with cmake";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd