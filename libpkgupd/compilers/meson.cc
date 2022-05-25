#include "meson.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool Meson::compile(Recipe* recipe, Configuration* config, std::string dir,
                    std::string destdir, std::vector<std::string>& environ) {
  auto configure = recipe->configure();
  if (configure.find("--prefix") == std::string::npos) {
    configure =
        " --prefix=" +
        config->get<std::string>(BUILD_CONFIG_PREFIX, DEFAULT_PREFIX) +
        " --sysconfdir=" +
        config->get<std::string>(BUILD_CONFIG_SYSCONFDIR, DEFAULT_SYSCONFDIR) +
        " --libdir=" +
        config->get<std::string>(BUILD_CONFIG_LIBDIR, DEFAULT_LIBDIR) +
        " --libexecdir=" +
        config->get<std::string>(BUILD_CONFIG_LIBEXECDIR, DEFAULT_LIBEXECDIR) +
        " --bindir=" +
        config->get<std::string>(BUILD_CONFIG_BINDIR, DEFAULT_BINDIR) +
        " --sbindir=" +
        config->get<std::string>(BUILD_CONFIG_SBINDIR, DEFAULT_SBINDIR) +
        " --datadir=" +
        config->get<std::string>(BUILD_CONFIG_DATADIR, DEFAULT_DATADIR) +
        " --localstatedir=" +
        config->get<std::string>(BUILD_CONFIG_LOCALSTATEDIR,
                                 DEFAULT_LOCALSTATEDIR) +
        " " + configure;
  }
  if (int status = Executor().execute("meson  _pkgupd_builddir " + configure,
                                      dir, environ);
      status != 0) {
    p_Error = "failed to do meson configuration";
    return false;
  }

  if (int status = Executor().execute(
          "ninja -C _pkgupd_builddir " + recipe->compile(), dir, environ);
      status != 0) {
    p_Error = "failed to build with meson";
    return false;
  }

  auto install = recipe->install();
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