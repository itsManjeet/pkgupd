#include "../../recipe.hh"
#include "../../database/database.hh"
#include "../../installer/plugin.hh"
#include <tuple>
#include <string>

#include <rlx.hh>
#include <io.hh>
#include <sys/exec.hh>
#include <fstream>
#include <tar/tar.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class _rlx : public plugin::installer
{
public:
    _rlx(YAML::Node const &c)
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

        string output;

        {
            if (getenv("USE_APPCTL_FORMAT") == nullptr)
            {
                auto [exitcode, _output] = rlx::sys::exec(
                    io::format("tar -xaf \"", pkgpath, "\" ./.info -O"));
                if (exitcode != 0)
                {
                    _error = "corrupt or invalid package, meta data is missing, try set USE_APPCTL_FORMAT=1 for appctl packages";
                    return {nullptr, nullptr};
                }
                else
                {
                    output = _output;
                }
            }
            else
            {
                auto [exitcode, _output] = rlx::sys::exec(
                    io::format("tar -xaf \"", pkgpath, "\" .data/info -O"));
                if (exitcode != 0)
                {
                    _error = "corrupt or invalid package, meta data is missing";
                    return {nullptr, nullptr};
                }
                else
                {
                    auto node = YAML::Load(_output);
                    output = io::format(
                        "id: ", node["name"].as<string>(), '\n',
                        "version: ", node["version"].as<string>(), '\n',
                        "about: ", node["description"].as<string>(), '\n',
                        "packages:\n  - id: ", node["name"].as<string>(), '\n',
                        "    plugin: port\npkgid: ", node["name"].as<string>());

                    io::debug(level::trace, output);
                }
            }
        }

        try
        {
            io::writefile(tmpfile, output);
            auto recipe = new pkgupd::recipe(tmpfile);
            auto node = YAML::Load(output);
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

    bool
    getfile(string pkgpath, string path, string out)
    {
        auto [status, output] = rlx::sys::exec("tar -xhaf " + pkgpath + " " + path + " -O >" + out);
        if (status != 0)
        {
            _error = status;
            return false;
        }

        return true;
    }
};

extern "C" plugin::installer *pkgupd_init(YAML::Node const &c)
{
    return new _rlx(c);
}
