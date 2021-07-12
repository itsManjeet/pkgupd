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

#include <sys/stat.h>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class app : public plugin::installer
{
private:
    std::vector<string> _liblist;

    string desktop_file;
    string app_run;
    string appid;

public:
    app(YAML::Node const &c)
        : plugin::installer(c)
    {
    }

    /* 
     * Unpack here no really means to unpack but install into system
     */
    bool unpack(string pkgpath, string rootdir, std::vector<string> &fileslist)
    {
        string appname = basename((char *)pkgpath.c_str());
        string new_path = rootdir + "/apps";

        std::error_code ec;

        if (!std::filesystem::exists(new_path))
        {
            std::filesystem::create_directories(new_path, ec);
            if (ec)
            {
                _error = ec.message();
                return false;
            }
        }

        ec.clear();

        std::filesystem::copy(pkgpath, new_path + "/" + appname, ec);
        if (ec)
        {
            _error = ec.message();
            return false;
        }

        fileslist.push_back(new_path + "/" + appname);

        return true;
    }

    bool patch_binaries(string const &pkgdir)
    {
        auto [status, output] = rlx::sys::exec("find " + pkgdir + "/ -type f -exec sed -i -e \"s#/usr#././#g\" {} \\;");
        if (status != 0)
        {
            _error = output;
            return false;
        }

        return true;
    }

    bool resolve_libraries(string const &binpath, string pkgdir)
    {
        auto [status, output] = rlx::sys::exec("LD_LIBRARY_PATH=" + pkgdir + " ldd " + binpath + " | awk '{print $3}'");
        if (status != 0)
        {
            _error = output;
            return false;
        }

        if (std::find(_liblist.begin(), _liblist.end(), binpath) != _liblist.end())
            return true;

        auto liblist = rlx::algo::str::split(output, '\n');
        for (auto const &i : liblist)
        {
            // skip if not a path
            if (i.length() == 0)
                continue;

            if (i[0] != '/')
                continue;

            // return error if not exist
            if (!std::filesystem::exists(i))
            {
                io::error(i, " not exist");
            }

            // check if listing its own library
            if (i.rfind(pkgdir, 0) == 0)
                continue;

            if (std::find(_liblist.begin(), _liblist.end(), i) == _liblist.end())
                _liblist.push_back(i);
        }

        return true;
    }

    bool pack(pkgupd::recipe const &recipe, string const &pkgid, string pkgdir, string out)
    {
        std::ofstream file(pkgdir + "/.info");
        file << recipe.node() << std::endl;
        file << "pkgid: " << pkgid << std::endl;
        file.close();

        if (!(recipe.node()["resolve"] && recipe.node()["resolve"].as<bool>() == false))
        {
            auto [status, output] = rlx::sys::exec("find " + pkgdir + "/ -type f ! -size 0 -exec grep -IL . \"{}\" \\;");
            if (status != 0)
            {
                _error = output;
                return false;
            }

            for (auto const &i : rlx::algo::str::split(output, '\n'))
                if (!resolve_libraries(i, pkgdir))
                    return false;

            for (auto i : _liblist)
            {
                if (std::filesystem::exists(pkgdir + "/" + i))
                    continue;

                auto ppath = rlx::path::dirname(i);
                std::filesystem::create_directories(pkgdir + "/" + ppath);
                std::filesystem::copy(i, pkgdir + "/" + i);
            }
        }

        if (!(recipe.node()["patch"] && recipe.node()["patch"].as<bool>() == false))
            if (!patch_binaries(pkgdir))
                return false;

        if (recipe.node()["desktop-file"])
            desktop_file = recipe.node()["desktop-file"].as<string>();

        if (recipe.node()["app-run"])
            app_run = recipe.node()["app-run"].as<string>();

        if (recipe.node()["app-id"])
            appid = recipe.node()["app-id"].as<string>();
        else
            appid = pkgid;

        if (desktop_file.length() == 0)
        {
            desktop_file = io::format(
                "[Desktop Entry]\n",
                "Name=", appid, "\n"
                                "Exec=",
                appid, "\n"
                       "Icon=",
                (_config["icon"] ? _config["icon"].as<string>() : appid), "\n",
                "Type=Application\n",
                "Categories=", (_config["categories"] ? _config["categories"].as<string>() : "Application;"));
        }

        io::writefile(pkgdir + "/" + appid + ".desktop", desktop_file);

        if (app_run.length() == 0)
        {
            if (!rlx::curl::download("https://github.com/AppImage/AppImageKit/releases/download/13/AppRun-x86_64", pkgdir + "/AppRun"))
            {
                _error = "failed to download AppRun";
                return false;
            }
        }
        else
        {
            io::writefile(pkgdir + "/AppRun", app_run);
        }

        if (chmod((pkgdir + "/AppRun").c_str(), 0755))
        {
            _error = "failed to set executable permission to AppRun";
            return false;
        }

        {
            auto [status, output] = rlx::sys::exec("appimagetool " + pkgdir + " " + out);
            if (status != 0)
            {
                _error = output;
                return false;
            }
        }
        return true;
    }

    std::tuple<pkgupd::recipe *, pkgupd::package *> get(string pkgpath)
    {
        string filepath = ".info";

        chdir("/tmp");

        auto [status, output] = rlx::sys::exec(pkgpath + " --appimage-extract .info");
        if (status != 0)
        {
            _error = output;
            return {nullptr, nullptr};
        }

        auto recipe = new pkgupd::recipe("squashfs-root/.info");
        auto pkg = new pkgupd::package(recipe->packages()[0]);

        std::filesystem::remove_all("squashfs-root");

        return {recipe, pkg};
    }

    bool getfile(string pkgpath, string path, string out)
    {
        chdir("/tmp");

        auto [status, output] = rlx::sys::exec(pkgpath + " --appimage-extract " + path);
        if (status != 0)
        {
            _error = output;
            return false;
        }

        std::error_code ec;
        std::filesystem::copy("squashfs-root/" + path, out, ec);
        if (!ec)
        {
            _error = ec.message();
            return false;
        }
        std::filesystem::remove_all("squashfs-root");

        return true;
    }
};

extern "C" plugin::installer *pkgupd_init(YAML::Node const &c)
{
    return new app(c);
}
