#include "../../recipe.hh"
#include <tuple>
#include <string>

#include <io.hh>
#include <fstream>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

extern "C"
std::tuple<bool, string> pkgupd_pack(pkgupd::recipe const &recipe, YAML::Node const &, pkgupd::package *pkg, string dir, string output)
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