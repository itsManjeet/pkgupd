#include "../../recipe.hh"
#include <tuple>
#include <string>

#include <io.hh>
#include <rlx.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class auto_ : public obj
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

    pkgupd::recipe const &recipe;
    pkgupd::package *pkg;
    string src_dir;
    string pkg_dir;

    configurator get_configurators(string path)
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

    builder get_builder(string path)
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

    string get_env(const string &f, string fallback)
    {
        auto env = recipe.environ(pkg);

        for (auto const &i : env)
        {
            size_t idx = i.find_first_of('=');
            if (i.substr(0, idx) == f)
                return i.substr(idx + 1, i.length() - (idx + 1));
        }

        return fallback;
    }

    std::string get_config_args(configurator c)
    {
        string prefix = get_env("PREFIX", "/usr");
        string libdir = get_env("LIBDIR", prefix + "/lib");
        string sysconfdir = get_env("SYSCONFDIR", "/etc");
        string bindir = get_env("BINDIR", prefix + "/bin");
        string localstatedir = get_env("LOCALSTATEDIR", "/var");
        string datadir = get_env("DATADIR", prefix + "/share");

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

    std::string get_install_args(builder b)
    {
        return (b == builder::make ? "DESTDIR=" + pkg_dir : "") + "install";
    }

public:
    auto_(pkgupd::recipe const &recipe,
          YAML::Node const &node,
          pkgupd::package *pkg,
          string src_dir,
          string pkg_dir)

        : recipe(recipe),
          pkg(pkg),
          src_dir(src_dir),
          pkg_dir(pkg_dir)
    {
    }

    auto get_flag_value(string const &f)
    {
        for (auto const &i : pkg->flags())
            if (i.id() == f)
                return std::tuple{
                    i.value(),
                    i.only()};

        return std::tuple{string(""), false};
    }

    bool configure(string build_dir)
    {
        auto _conf = get_configurators(src_dir);
        if (_conf == configurator::none)
        {
            _error = "no valid configurator found in '" + src_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _conf, "'");

        auto conf_args = get_config_args(_conf);
        auto [_conf_args, force] = get_flag_value("configure");
        if (force)
            conf_args = _conf_args;
        else
            conf_args += " " + _conf_args;

        string _cmd = io::format(_conf, " ", conf_args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe.environ(pkg)))
        {
            _error = "configuration failed";
            return false;
        }

        return true;
    }

    bool compile(string build_dir)
    {
        auto _builder = get_builder(build_dir);
        if (_builder == builder::none)
        {
            _error = "no valid builder found in '" + build_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _builder, "'");

        auto compile_args = get_compile_args(_builder);

        auto [_compile_args, force] = get_flag_value("compile");
        if (force)
            compile_args = _compile_args;
        else
            compile_args += " " + _compile_args;

        string _cmd = io::format(_builder, " ", compile_args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe.environ(pkg)))
        {
            _error = "configuration failed";
            return false;
        }

        return true;
    }

    bool install(string build_dir)
    {
        auto _builder = get_builder(build_dir);
        if (_builder == builder::none)
        {
            _error = "no valid builder found in '" + build_dir + "'";
            return false;
        }

        io::info("found ", color::MAGENTA, "'", _builder, "'");

        auto args = get_install_args(_builder);

        auto [_args, force] = get_flag_value("compile");
        if (force)
            args = _args;
        else
            args += " " + _args;

        string _cmd = io::format(_builder, " ", _args);

        if (rlx::utils::exec::command(_cmd, build_dir, recipe.environ(pkg)))
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
};

extern "C" std::tuple<bool, string> pkgupd_build(
    pkgupd::recipe const &recipe,
    YAML::Node const &node,
    pkgupd::package *pkg,
    string src_dir,
    string pkg_dir)
{
    auto _a = auto_(recipe, node, pkg, src_dir, pkg_dir);
    string _build_dir = src_dir + "/build_" + pkg->id();

    io::process("configuring source");
    if (!_a.configure(_build_dir))
        return {false, _a.error()};

    io::process("compiling codes");
    if (!_a.compile(_build_dir))
        return {false, _a.error()};

    if (!_a.install(_build_dir))
        return {false, _a.error()};

    return {true, ""};
}