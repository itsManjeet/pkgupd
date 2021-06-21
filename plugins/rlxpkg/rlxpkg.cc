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

extern "C" std::tuple<bool, string> pkgupd_pack(pkgupd::recipe const &recipe, YAML::Node const &, pkgupd::package *pkg, string dir, string output)
{
    std::ofstream file(dir + "/.info");

    file << recipe.node() << std::endl;
    file << "pkgid: " << pkg->id() << std::endl;
    file.close();

    if (rlx::utils::exec::command(
            io::format("tar -caf " + output + " . "),
            dir,
            recipe.environ()))
    {
        return {false, "failed to pack"};
    }
    return {true, ""};
}

extern "C" std::tuple<bool, string, std::vector<string>>
pkgupd_unpack(pkgupd::recipe const &recipe, YAML::Node const &, pkgupd::package *pkg, string pkgfile, string root_dir)
{
    auto filelist = tar::files(pkgfile);
    io::process("extracting ", pkgfile, " into ", root_dir);
    if (utils::exec::command(
            io::format("tar -xhpf ", pkgfile, " -C ", root_dir)))
        return {false, "failed to unpack", std::vector<string>()};

    return {true, "", filelist};
}