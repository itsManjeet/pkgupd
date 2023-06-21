#include "autoconf.hxx"

#include "../exec.hxx"
#include "../recipe.hxx"

namespace rlxos::libpkgupd {
    bool AutoConf::compile(Recipe *recipe, Configuration *config, std::string dir,
                           std::string destdir, std::vector<std::string> &environ) {
        auto configure = recipe->configure();

        bool same_dir = false;
        if (recipe->node()["autoconf-dir"]) {
            same_dir = recipe->node()["autoconf-dir"].as<bool>();
        }

        std::string configurator = "./configure";
        if (!same_dir) {
            configurator = "../configure";
            dir += "/_pkgupd_builddir";

            std::filesystem::create_directories(dir);
        }

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
        if (int status =
                    Executor().execute(configurator + " " + configure, dir, environ);
                status != 0) {
            p_Error = "failed to do autoconf configuration";
            return false;
        }

        if (int status = Executor().execute("make " + recipe->compile(), dir, environ);
                status != 0) {
            p_Error = "failed to build with autoconf";
            return false;
        }

        auto install = recipe->install();
        if (install.find("install ") == std::string::npos) {
            install = "install " + install;
        }

        if (int status = Executor().execute("make DESTDIR=" + destdir + " " + install,
                                            dir, environ);
                status != 0) {
            p_Error = "failed to install with autoconf";
            return false;
        }

        return true;
    }
}  // namespace rlxos::libpkgupd