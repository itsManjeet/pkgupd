#include "../../recipe.hh"
#include <tuple>
#include <string>

#include <io.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

/// TODO rewrite in better way
enum buildtool
{
    NONE,
    AUTOCONF,
    AUTOGEN,
    PYSETUP,
    MESON,
    CMAKE,
};

std::map<string, buildtool> buildtool_files = {
    {"configure", AUTOCONF},
    {"autogen.sh", AUTOGEN},
    {"CMakeLists.txt", CMAKE},
    {"meson.build", MESON},
    {"setup.py", PYSETUP}};

string buildtool_tool(buildtool c)
{
    switch (c)
    {
    case buildtool::AUTOCONF:
        return "../configure";
    case buildtool::AUTOGEN:
        return "../autogen.sh";
    case buildtool::CMAKE:
        return "cmake";
    case buildtool::MESON:
        return "meson";
    case buildtool::PYSETUP:
        return "setup.py";
    }

    throw std::runtime_error("unknow build tool id" + std::to_string(c));
}

extern "C" std::tuple<bool, string> pkgupd_build(
    pkgupd::recipe const &recipe,
    YAML::Node const &node,
    pkgupd::package *pkg,
    string src_dir,
    string pkg_dir)
{
    buildtool b = NONE;

    for (auto const &t : buildtool_files)
        if (std::filesystem::exists(src_dir + "/" + t.first))
        {
            b = t.second;
            break;
        }

    auto build_tool_cmd = buildtool_tool(b);

    io::info("found '", color::MAGENTA, build_tool_cmd, color::RESET, color::BOLD, "'");

    auto get_env = [&](const string &f, string fallback) -> string
    {
        auto env = recipe.environ(pkg);

        for (auto const &i : env)
        {
            size_t idx = i.find_first_of('=');
            if (i.substr(0, idx) == f)
                return i.substr(idx + 1, i.length() - (idx + 1));
        }

        return fallback;
    };

    string args;
    string prefix = get_env("PREFIX", "/usr");
    string libdir = get_env("LIBDIR", prefix + "/lib");
    string sysconfdir = get_env("SYSCONFDIR", "/etc");
    string bindir = get_env("BINDIR", prefix + "/bin");
    string localstatedir = get_env("LOCALSTATEDIR", "/var");
    string datadir = get_env("DATADIR", prefix + "/share");

    if (b >= AUTOCONF && b <= MESON)
    {

        args = rlx::io::format(
            " --prefix=", prefix,
            " --libdir=", libdir,
            " --libexecdir=", libdir,
            " --sysconfdir=", sysconfdir,
            " --bindir=", bindir,
            " --sbindir=", bindir,
            " --localstatedir=", localstatedir,
            " --datadir=", datadir);

        if (b == MESON)
            args += " .. ";
    }

    if (b == CMAKE)
    {
        args = rlx::io::format(
            " -DCMAKE_INSTALL_PREFIX=", prefix);

        args += " -S ..";
    }

    for (auto const &i : pkg->flags())
    {
        if (i.id() == "configure")
        {
            if (i.only())
                args = " " + i.value().substr(0, i.value().length() - 1) + (b == CMAKE ? " -S " : " ") + (b == MESON || b == CMAKE ? " .. " : "");
            else
                args += " " + i.value();

            break;
        }
    }

    string dir = src_dir + "/build_" + pkg->id();
    std::filesystem::create_directories(dir);

    if (rlx::utils::exec::command(rlx::io::format(
                                      build_tool_cmd,
                                      args),
                                  dir, recipe.environ(pkg)))
        return {false, "failed to configure"};

    string cmd;
    if (std::filesystem::exists(dir + "/Makefile"))
    {
        io::info("found ", color::MAGENTA, "make");
        cmd = " make ";
    }
    else if (std::filesystem::exists(dir + "/build.ninja"))
    {
        io::info("found ", color::MAGENTA, "ninja");
        cmd = " ninja ";
    }
    else
        return {false, "failed to get buildtool"};

    string cmd_args = "";

    for (auto const &i : pkg->flags())
    {
        if (i.id() == "compile")
        {
            if (i.only())
                cmd_args = " " + i.value();
            else
                cmd_args += " " + i.value();

            break;
        }
    }

    if (rlx::utils::exec::command(
            cmd + " " + cmd_args,
            dir,
            recipe.environ(pkg)))
    {
        return {false, "failed to compile"};
    }

    {
        string cmd = "DESTDIR=" + get_env("DESTDIR", pkg_dir);
        if (std::filesystem::exists(dir + "/Makefile"))
        {
            io::info("found ", color::MAGENTA, "make");
            cmd = " make install " + cmd;
        }
        else if (std::filesystem::exists(dir + "/build.ninja"))
        {
            io::info("found ", color::MAGENTA, "ninja");
            cmd += " ninja install";
        }
        else
            return {false, "failed to get installer tool"};

        string cmd_args = "";

        for (auto const &i : pkg->flags())
        {
            if (i.id() == "install")
            {
                if (i.only())
                    cmd_args = " " + i.value();
                else
                    cmd_args += " " + i.value();

                break;
            }
        }

        if (rlx::utils::exec::command(
                io::format(
                    cmd, cmd_args),
                dir,
                recipe.environ(pkg)))
        {
            return {false, "failed to install"};
        }
    }

    return {true, ""};
}