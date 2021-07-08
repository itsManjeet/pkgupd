#include "../../recipe.hh"
#include "../../database/database.hh"
#include "../../installer/plugin.hh"
#include <tuple>
#include <string>

#include <rlx.hh>
#include <io.hh>
#include <fstream>
#include <tar/tar.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class rlxpkg : public plugin::installer
{
public:
    rlxpkg(YAML::Node const &c)
        : plugin::installer(c)
    {
    }

    bool unpack(string pkgpath, string rootdir, std::vector<string> &fileslist)
    {
        fileslist = tar::files(pkgpath);
        io::process("extracting ", pkgpath, " into ", rootdir);
        if (utils::exec::command(
                io::format("tar --exclude='./.info' --exclude='./.data/' -xhpf \"", pkgpath, "\" -C ", rootdir)))
        {
            _error = "failed to unpack";
            return false;
        }

        return true;
    }

    bool pack(pkgupd::recipe const &recipe, string const &pkgid, string pkgdir, string out)
    {
        std::ofstream file(pkgdir + "/.info");
        file << recipe.node() << std::endl;
        file << "pkgid: " << pkgid << std::endl;

        file.close();
        if (rlx::utils::exec::command(
                io::format("tar -caf " + out + " . "),
                pkgdir))
        {
            _error = "failed to pack " + recipe.id() + ":" + pkgid;
            return false;
        }

        return true;
    }

    std::tuple<pkgupd::recipe *, pkgupd::package *> get(string pkgpath)
    {
        io::process("getting recipe file from ", pkgpath);
        auto tmpfile = utils::sys::tempfile("/tmp", "rcp");
        auto data = utils::exec::output(
            io::format("tar -xaf \"", pkgpath, "\" ./.info -O"));

        try
        {
            io::writefile(tmpfile, data);
            auto recipe = new pkgupd::recipe(tmpfile);
            auto node = YAML::Load(data);
            pkgupd::package *pkg = nullptr;

            if (node["pkgid"])
            {
                for (auto p : recipe->packages())
                    if (p.id() == node["pkgid"].as<string>())
                    {
                        pkg = new pkgupd::package(p);
                        break;
                    }
            }
            std::filesystem::remove(tmpfile);

            return {recipe, pkg};
        }
        catch (std::exception const &c)
        {
            _error = c.what();
            return {nullptr, nullptr};
        }

        return {nullptr, nullptr};
    }
};

extern "C" plugin::installer *pkgupd_init(YAML::Node const &c)
{
    return new rlxpkg(c);
}
