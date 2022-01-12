#include "autoconf.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool AutoConf::compile(Recipe const& recipe, std::string dir,
                       std::string destdir,
                       std::vector<std::string> const& environ) {
  auto configure = recipe.configure();

  bool same_dir = false;
  if (recipe.node()["autoconf-dir"]) {
    same_dir = recipe.node()["autoconf-dir"].as<bool>();
  }

  std::string configurator = "./configure";
  if (!same_dir) {
    configurator = "../configure";
    dir += "/_pkgupd_builddir";
  }

  if (configure.find("--prefix") != std::string::npos) {
    configure = " --prefix=" + PREFIX + " --sysconfdir=" + SYSCONF_DIR +
                " --libdir=" + LIBDIR + " --libexecdir=" + LIBEXEDIR +
                " --bindir=" + BINDIR + " --sbindir=" + SBINDIR +
                " --datadir=" + DATADIR + " --localstatedir=" + CACHEDIR + " " +
                configure;
  }
  if (int status =
          Executor().execute(configurator + " " + configure, dir, environ);
      status != 0) {
    p_Error = "failed to do meson configuration";
    return false;
  }

  if (int status = Executor().execute("make " + recipe.compile(), dir, environ);
      status != 0) {
    p_Error = "failed to build with meson";
    return false;
  }

  auto install = recipe.install();
  if (install.find("install ") == std::string::npos) {
    install = "install " + install;
  }

  if (int status = Executor().execute("make DESTDIR=" + destdir + " " + install,
                                      dir, environ);
      status != 0) {
    p_Error = "failed to install with meson";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd