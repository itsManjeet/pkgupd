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
#include <path/path.hh>
#include <sys/stat.h>
#include <regex>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class app : public plugin::installer
{
private:
    string desktop_file;
    string app_run;
    string appid;
    std::set<string> _libraries;

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
        string _app_dir = rootdir + "/apps";

        string _app_bin = _app_dir + "/" + appname;
        std::error_code ec;
        std::filesystem::create_directories(_app_dir);

        std::filesystem::copy_file(pkgpath, _app_bin, std::filesystem::copy_options::update_existing, ec);
        if (ec)
        {
            _error = ec.message();
            return false;
        }
        fileslist.push_back(_app_bin);

        auto [recipe, pkg] = get(pkgpath);
        if (recipe == nullptr || pkg == nullptr)
        {
            return false;
        }

        // install icon file
        string _icon_file = pkg->id() + ".png";
        string _app_icon = _app_dir + "/share/pixmaps/" + appname + ".png";
        std::filesystem::create_directories(_app_dir + "/share/pixmaps/");
        if (!getfile(pkgpath, _icon_file, _app_icon))
            return false;

        fileslist.push_back(_app_icon);

        // install desktop file
        string _desktop_file = pkg->id() + ".desktop";
        string _app_desktop = _app_dir + "/share/applications/" + appname + ".desktop";
        std::filesystem::create_directories(_app_dir + "/share/applications");
        if (!getfile(pkgpath, _desktop_file, _app_desktop))
            return false;

        fileslist.push_back(_app_desktop);

        // patch desktop file
        string content = io::readfile(_app_desktop);
        auto replace = [&content](string const &i, string const &v)
        {
            auto start_idx = content.find(i);
            auto end_idx = content.find_first_of("\n", start_idx);

            content = content.substr(0, start_idx) +
                      v + content.substr(end_idx, content.length() - end_idx);
        };
        replace("Exec=", "Exec=" + _app_bin);
        replace("Icon=", "Icon=" + _app_icon);

        io::writefile(_app_desktop, content);

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

    void _resolve(string const &path, string const &pkgdir)
    {
        _libraries.insert(path);
        auto [status, output] = rlx::sys::exec("LD_LIBRARY_PATH=" + pkgdir + " ldd " + path + " | awk '{print $3}'");
        if (status == 0)
        {
            std::stringstream ss(output);
            string l;
            while (getline(ss, l, '\n'))
            {
                if (l[0] == '/')
                    if (_libraries.find(l) == _libraries.end())
                        _resolve(l, pkgdir);
            }
        }
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
                _resolve(i, pkgdir);

            for (auto i : _libraries)
            {
                if (i == "/lib64/ld-linux-x86-64.so.2")
                    continue;

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
            auto [status, output] = rlx::sys::exec("ARCH=$(uname -m) appimagetool " + pkgdir + " " + out);
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

        auto _work_dir = rlx::utils::sys::tempdir("/tmp/", "pkgupd-app");
        chdir(_work_dir.c_str());

        auto [status, output] = rlx::sys::exec(pkgpath + " --appimage-extract .info");
        if (status != 0)
        {
            _error = output;
            std::filesystem::remove_all(_work_dir);
            return {nullptr, nullptr};
        }

        auto recipe = new pkgupd::recipe("squashfs-root/.info");
        auto pkg = new pkgupd::package(recipe->packages()[0]);

        std::filesystem::remove_all(_work_dir);

        return {recipe, pkg};
    }

    bool getfile(string pkgpath, string path, string out)
    {
        auto _work_dir = rlx::utils::sys::tempdir("/tmp/", "pkgupd-app");
        chdir(_work_dir.c_str());

        auto [status, output] = rlx::sys::exec(pkgpath + " --appimage-extract " + path);
        if (status != 0)
        {
            _error = output;
            std::filesystem::remove_all(_work_dir);
            return false;
        }

        std::error_code ec;
        std::filesystem::copy("squashfs-root/" + path, out, std::filesystem::copy_options::update_existing, ec);
        if (ec)
        {
            _error = ec.message();
            std::filesystem::remove_all(_work_dir);
            return false;
        }

        std::filesystem::remove_all(_work_dir);

        return true;
    }
};

extern "C" plugin::installer *pkgupd_init(YAML::Node const &c)
{
    return new app(c);
}
