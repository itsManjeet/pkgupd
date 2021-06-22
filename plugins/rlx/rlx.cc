#include "../../recipe.hh"
#include <tuple>
#include <string>

#include <io.hh>
#include <fstream>
#include <tar/tar.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

extern "C" std::tuple<bool, string> pkgupd_pack(pkgupd::recipe const &recipe, YAML::Node const &, pkgupd::package *pkg, string pkgdir, string output)
{
    if (!std::filesystem::create_directories(pkgdir + "/.data"))
        return {false, "failed to create " + pkgdir + "/.data"};

    std::ofstream file(pkgdir + "/.data/info");
    file << "id: " << pkg->id() << std::endl
         << "version: " << recipe.version() << std::endl
         << "release: " << 1 << std::endl
         << "description: " << recipe.about()
         << "depends: " << std::endl;

    file.close();

    std::ofstream rcp(pkgdir + "/.info");
    rcp << recipe.node() << std::endl
        << "pkgid: " << pkg->id() << std::endl;
    rcp.close();

    if (rlx::utils::exec::command(
            io::format("tar -caf ", output, " . "),
            pkgdir,
            recipe.environ()))
        return {false, "failed to pack"};

    return {true, ""};
}

extern "C" std::tuple<bool, string, std::vector<string>>
pkgupd_unpack(pkgupd::recipe const &recipe, YAML::Node const &, pkgupd::package *pkg, string pkgfile, string root_dir)
{
    auto filelist = tar::files(pkgfile);
    io::process("extracting ", pkgfile, " into ", root_dir);
    if (utils::exec::command(
            io::format("tar -xhpf \"", pkgfile, "\" -C ", root_dir)))
        return {false, "failed to unpack", std::vector<string>()};

    return {true, "", filelist};
}

extern "C" std::tuple<bool, string, pkgupd::recipe, pkgupd::package *>
pkgupd_getrecipe(string pkgpath)
{
    io::process("getting recipe file from ", pkgpath);
    auto tmpfile = utils::sys::tempfile("/tmp", "rcp");
    auto data = utils::exec::output(
        io::format("tar -xaf \"", pkgpath, "\" ./.info -O"));

    try
    {
        io::writefile(tmpfile, data);
        auto recipe = pkgupd::recipe(tmpfile);
        auto node = YAML::Load(data);
        pkgupd::package *pkg = nullptr;

        if (node["pkgid"])
        {
            for (auto p : recipe.packages())
                if (p.id() == node["pkgid"].as<string>())
                {
                    pkg = new pkgupd::package(p);
                    break;
                }
        }

        return {true, "", recipe, pkg};
    } catch (std::exception const& c)
    {
        io::error(c.what());
        return {false, "", pkgupd::recipe(tmpfile), nullptr};
    }
}