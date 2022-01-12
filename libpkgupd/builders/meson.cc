#include "meson.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool Meson::compile(Recipe const& recipe, std::string dir, std::string destdir,
                    std::vector<std::string> const& environ) {
  auto configure = recipe.configure();
  if (configure.find("--prefix") != std::string::npos) {
    configure = " --prefix=" + PREFIX + " --sysconfdir=" + SYSCONF_DIR +
                " --libdir=" + LIBDIR + " --libexecdir=" + LIBEXEDIR +
                " --bindir=" + BINDIR + " --sbindir=" + SBINDIR +
                " --datadir=" + DATADIR + " --localstatedir=" + CACHEDIR + " " +
                configure;
  }
  if (int status = Executor().execute(
          "meson " + configure + " _pkgupd_builddir", dir, environ);
      status != 0) {
    p_Error = "failed to do meson configuration";
    return false;
  }

  if (int status = Executor().execute(
          "ninja -C _pkgupd_builddir " + recipe.compile(), dir, environ);
      status != 0) {
    p_Error = "failed to build with meson";
    return false;
  }

  auto install = recipe.install();
  if (install.find("install ") == std::string::npos) {
    install = "install " + install;
  }

  if (int status = Executor().execute("ninja -C _pkgupd_builddir " + install,
                                      dir, environ);
      status != 0) {
    p_Error = "failed to install with meson";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd