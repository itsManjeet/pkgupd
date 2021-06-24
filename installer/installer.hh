#ifndef __INSTALLER__
#define __INSTALLER__

#include <rlx.hh>
#include <algo/algo.hh>
#include "../recipe.hh"
#include "../config.hh"
#include "../database/database.hh"

namespace pkgupd
{
    using namespace rlx;
    using color = rlx::io::color;
    using level = rlx::io::debug_level;

    class installer : public obj
    {
    public:
        typedef std::tuple<bool, string, std::vector<string>> (*unpack_func)(recipe const &, YAML::Node const &, package *, string pkgpath, string rootdir);
        typedef std::tuple<bool, string, recipe, package *> (*getrecipe_func)(string path);

    private:
        recipe _recipe;
        YAML::Node _config;
        database _database;

        string _dir_pkgs,
            _dir_work,
            _dir_data,
            _dir_root;

        void clean()
        {

            if (getenv("NO_CLEAN") == nullptr && std::filesystem::exists(_dir_work))
            {
                io::process("clearing cache");
                std::filesystem::remove_all(_dir_work);
            }
            else
                io::debug(level::warn, "NO_CLEAN environ set, skip cleaning");
        }

    public:
        installer(recipe const &r,
                  YAML::Node const &c)
            : _recipe(r), _config(c), _database(c)
        {
            auto get_dir = [&](string path, string fallback) -> string
            {
                if (c["dir"] && c["dir"][path])
                    return c["dir"][path].as<string>();
                return fallback;
            };

            _dir_pkgs = get_dir("pkgs", DEFAULT_DIR_PKGS);
            _dir_work = get_dir("work", DEFAULT_DIR_WORK);
            _dir_root = get_dir("root", "/");
            _dir_data = get_dir("data", DEFAULT_DIR_DATA);

            _dir_work = rlx::utils::sys::tempdir(_dir_work, "pkgupd");
        }

        ~installer()
        {
            clean();
        }

        std::tuple<bool, string> download(package *pkg)
        {
            auto pkgid = _recipe.id() + "-" + _recipe.version() + (pkg == nullptr ? "" : ":" + pkg->id());
            string pkgfile = pkgid + "." + _recipe.pack(pkg);
            string pkgpath = _dir_pkgs + "/" + pkgfile;

            io::debug(level::trace, "pkgfile ", pkgpath);
            if (std::filesystem::exists(pkgpath))
                io::debug(level::trace, pkgfile, " found in cache");
            else
            {

                if (!_database.get_from_server(pkgfile, pkgpath))
                {
                    _error = _database.error();
                    return {false, ""};
                }
            }

            return {true, pkgpath};
        }

        static std::tuple<installer, package *> frompath(std::string pkgpath, YAML::Node const &cc)
        {
            assert(std::filesystem::exists(pkgpath));
            string plugin = std::filesystem::path(pkgpath).extension();
            plugin = plugin.substr(1, plugin.length() - 1);
            if (plugin.length() == 0)
                throw installer::exception(io::format(pkgpath, " not a installable package"));

            string plugin_path = utils::dlmodule::search(plugin, "/lib/pkgupd:/usr/lib/pkgupd", "PKGUPD_PLUGINS");
            if (plugin_path.length() == 0)
                throw installer::exception(io::format("failed to find plugin '" + plugin + "' required to getrecipe " + pkgpath));

            getrecipe_func getrecipe_plugin_fn;
            try
            {
                getrecipe_plugin_fn = utils::dlmodule::load<getrecipe_func>(plugin_path, "pkgupd_getrecipe");
            }
            catch (std::runtime_error const &e)
            {
                throw installer::exception(e.what());
            }

            auto [status, mesg, rcp, pkg] = getrecipe_plugin_fn(pkgpath);
            if (!status)
                throw installer::exception(mesg);

            return {installer(rcp, cc), pkg};
        }

        bool install(std::string subpkg = "")
        {
            package *pkg;
            if (subpkg.length())
            {
                bool found = false;
                for (auto p : _recipe.packages())
                {
                    if (p.id() == subpkg)
                    {
                        found = true;
                        pkg = &p;
                        break;
                    }
                }
                if (!found)
                {
                    _error = "no package " + subpkg + " found in " + _recipe.id();
                    return false;
                }
            }

            string plugin = _recipe.pack(pkg);
            string pkgid = pkg == nullptr ? _recipe.id() : pkg->id();

            if (plugin.length() == 0 || plugin == "none")
            {
                io::info(pkgid, " not a installable package, try compile");
                return true;
            }

            auto [status, pkgpath] = download(pkg);
            if (!status)
                return false;

            string plugin_path = utils::dlmodule::search(plugin, "/lib/pkgupd:/usr/lib/pkgupd", "PKGUPD_PLUGINS");
            if (plugin_path.length() == 0)
            {
                _error = "failed to find plugin '" + plugin + "' required to unpack " + pkgid;
                return false;
            }

            unpack_func plugin_unpack_fn;
            try
            {
                plugin_unpack_fn = utils::dlmodule::load<unpack_func>(plugin_path, "pkgupd_unpack");
            }
            catch (std::runtime_error const &e)
            {
                _error = e.what();
                return false;
            }

            assert(plugin_unpack_fn != nullptr);

            auto [unpack_status, output, filelist] =
                plugin_unpack_fn(_recipe, _config, pkg, pkgpath, _dir_root);

            if (!unpack_status)
            {
                _error = output;
                return false;
            }
            if (!_database.exec_triggers(filelist))
            {
                _error = _database.error();
                return false;
            }

            if (!register_pkg(filelist, pkg))
                return false;

            return true;
        }

        bool register_pkg(std::vector<string> fileslist, package *pkg)
        {
            auto pkgid = _recipe.id() + (pkg == nullptr ? "" : ":" + pkg->id());
            string data_file = _dir_data + "/" + pkgid;
            string list_file = data_file + ".files";
            if (std::filesystem::exists(list_file))
            {
                auto old_files = rlx::algo::str::split(rlx::io::readfile(list_file), '\n');
                std::vector<string> deprecated_file;
                for (auto const &i : old_files)
                    if (!rlx::algo::contains<string>(fileslist, i))
                        deprecated_file.push_back(i);
                io::debug(level::debug, "found ", deprecated_file.size(), " old files, cleaning");

                std::reverse(deprecated_file.begin(), deprecated_file.end());

                for (auto i : deprecated_file)
                {
                    i = _dir_root + "/" + i;
                    io::debug(level::trace, "cleaning ", i);
                    if (std::filesystem::is_directory(i) && std::filesystem::is_empty(i))
                        std::filesystem::remove(i);
                    else if (!std::filesystem::is_directory(i))
                        std::filesystem::remove(i);
                    else
                        io::debug(level::trace, "skipping ", i, " not a empty dir or file");
                }
            }

            io::writefile(data_file, _recipe.node());
            io::writefile(list_file, algo::str::join(fileslist, "\n"));

            return true;
        }
    };

}

#endif