#include "../../recipe.hh"
#include "../../compiler/plugin.hh"
#include <tuple>
#include <string>

#include <io.hh>
#include <rlx.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class auto_ : public plugin::compiler
{
private:
    enum class configurator
    {
        none,
        autoconf,
        autogen,
        pysetup,
        meson,
        cmake
    };
    enum class builder
    {
        none,
        ninja,
        make,
    };

    std::map<string, configurator> configurators =
        {
            {"configure", configurator::autoconf},
            {"autogen.sh", configurator::autogen},
            {"CMakeLists.txt", configurator::cmake},
            {"meson.build", configurator::meson},
            {"setup.py", configurator::pysetup},
        };

    std::map<string, builder> builders =
        {
            {"Makefile", builder::make},
            {"build.ninja", builder::ninja},
        };

    configurator get_configurators(pkgupd::package *pkg, string path)
    {
        // check if custom  configurator is provided
        for (auto const &i : pkg->flags())
            if (i.id() == "configurator")
                if (configurators.find(i.value()) == configurators.end())
                    return configurator::none;
                else
                    return configurators[i.value()];

        // otherwise search
        for (auto const &i : configurators)
            if (std::filesystem::exists(path + "/" + i.first))
                return i.second;

        return configurator::none;
    }

    builder get_builder(pkgupd::package *pkg, string path)
    {
        // check if custom  builder is provided
        for (auto const &i : pkg->flags())
            if (i.id() == "builder")
                if (builders.find(i.value()) == builders.end())
                    return builder::none;
                else
                    return builders[i.value()];

        for (auto const &i : builders)
            if (std::filesystem::exists(path + "/" + i.first))
                return i.second;

        return builder::none;
    }

    string get_env(pkgupd::recipe *recipe, pkgupd::package *pkg, const string &f, string fallback)
    {
        auto env = recipe->environ(pkg);

        for (auto const &i : env)
        {
            size_t idx = i.find_first_of('=');
            if (i.substr(0, idx) == f)
                return i.substr(idx + 1, i.length() - (idx + 1));
        }

        return fallback;
    }

    std::string get_config_args(pkgupd::recipe *recipe, pkgupd::package *pkg, configurator c)
    {
        string prefix = get_env(recipe, pkg, "PREFIX", "/usr");
        string libdir = get_env(recipe, pkg, "LIBDIR", prefix + "/lib");
        string sysconfdir = get_env(recipe, pkg, "SYSCONFDIR", "/etc");
        string bindir = get_env(recipe, pkg, "BINDIR", prefix + "/bin");
        string localstatedir = get_env(recipe, pkg, "LOCALSTATEDIR", "/var");
        string datadir = get_env(recipe, pkg, "DATADIR", prefix + "/share");

        string args;
        if (c == configurator::autoconf ||
            c == configurator::autogen ||
            c == configurator::meson)
        {
            return io::format(
                "--prefix=", prefix,
                " --libdir=", libdir,
                " --sysconfdir=", sysconfdir,
                " --bindir=", bindir,
                " --sbindir=", bindir,
                " --libexecdir=", libdir,
                " --datadir=", datadir,
                " --localstatedir=", localstatedir,

                // add meson buildtype=release
                (c == configurator::meson ? " --buildtype=release  .. " : ""));
        }
        else if (c == configurator::cmake)
        {
            return io::format(
                "-DCMAKE_INSTALL_PREFIX=", prefix,
                " -DCMAKE_INSTALL_LIBDIR=", libdir,
                " -DCMAKE_BUILD_TYPE=RELEASE",
                " -DCMAKE_INSTALL_SYSCONFDIR=", sysconfdir,
                " -DCMAKE_INSTALL_BINDIR=", bindir,
                " -DCMAKE_INSTALL_SBINDIR=", bindir,
                " -DCMAKE_INSTALL_LOCALSTATEDIR=", localstatedir,
                " -DCMAKE_INSTALL_DATADIR=", datadir,
                " -DCMAKE_INSTALL_LIBEXECDIR=", libdir,
                " -DCMAKE_INSTALL_SYSCONFDIR=", sysconfdir,
                " -S ..");
        }
        throw std::runtime_error("unknown configurator provided");
    }

    std::string get_compile_args(builder b)
    {
        return "-j $(nproc)";
    }

    std::string get_install_args(builder b, string pkg_dir)
    {
        return (b == builder::make ? "DESTDIR=" + pkg_dir : "") + "install";
    }

public:
    auto_(YAML::Node const &config)
        : plugin::compiler(config)
    {
    }

    auto get_flag_value(pkgupd::package *pkg, string const &f)
    {
        for (auto const &i : pkg->flags())
            if (i.id() == f)
                return std::tuple{
                    i.value(),
                    i.only()};

        return std::tuple{string(""), false};
    }

    bool configure(pkgupd::recipe *recipe, pkgupd::package *pkg, string src_dir, string build_dir)
    {
        auto _conf = get_configurators(pkg, src_dir);
        if (_conf == configurator::none)
        {
            _error = "no valid configurator found in '" + src_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _conf, "'");

        auto conf_args = get_config_args(recipe, pkg, _conf);
        auto [_conf_args, force] = get_flag_value(pkg, "configure");
        if (force)
            conf_args = _conf_args;
        else
            conf_args += " " + _conf_args;

        string _cmd = io::format(_conf, " ", conf_args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe->environ(pkg)))
        {
            _error = "configuration failed";
            return false;
        }

        return true;
    }

    bool compile(pkgupd::recipe *recipe, pkgupd::package *pkg, string build_dir)
    {
        auto _builder = get_builder(pkg, build_dir);
        if (_builder == builder::none)
        {
            _error = "no valid builder found in '" + build_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _builder, "'");

        auto compile_args = get_compile_args(_builder);

        auto [_compile_args, force] = get_flag_value(pkg, "compile");
        if (force)
            compile_args = _compile_args;
        else
            compile_args += " " + _compile_args;

        string _cmd = io::format(_builder, " ", compile_args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe->environ(pkg)))
        {
            _error = "configuration failed";
            return false;
        }

        return true;
    }

    bool install(pkgupd::recipe *recipe, pkgupd::package *pkg, string build_dir, string pkg_dir)
    {
        auto _builder = get_builder(pkg, build_dir);
        if (_builder == builder::none)
        {
            _error = "no valid builder found in '" + build_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _builder, "'");

        auto args = get_install_args(_builder, pkg_dir);

        auto [_args, force] = get_flag_value(pkg, "compile");
        if (force)
            args = _args;
        else
            args += " " + _args;

        string _cmd = io::format(_builder, " ", _args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe->environ(pkg)))
        {
            _error = "configuration failed";
            return false;
        }

        return true;
    }

    friend std::ostream &operator<<(std::ostream &os, configurator c)
    {
        switch (c)
        {
        case configurator::cmake:
            os << "cmake";
            break;
        case configurator::autogen:
            os << "../autogen.sh";
            break;
        case configurator::autoconf:
            os << "../configure";
            break;
        case configurator::meson:
            os << "meson";
            break;
        }
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, builder c)
    {
        switch (c)
        {
        case builder::ninja:
            os << "ninja";
            break;
        case builder::make:
            os << "make";
            break;
        }
        return os;
    }

    bool compile(pkgupd::recipe *recipe, pkgupd::package *pkg, string src_dir, string pkg_dir)
    {
        string _build_dir = src_dir + "/build_" + pkg->id();
        if (!configure(recipe, pkg, src_dir, _build_dir))
        {
            _error = "failed to configure";
            return false;
        }

        if (!compile(recipe, pkg, _build_dir))
        {
            _error = "failed to compile";
            return false;
        }

        if (!install(recipe, pkg, _build_dir, pkg_dir))
        {
            _error = "failed to install";
            return false;
        }

        return true;
    }
};

extern "C" plugin::compiler *pkgupd_init(YAML::Node const &c)
{
    return new auto_(c);
}