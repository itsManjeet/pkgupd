#ifndef _LIBPKGUPD_UNSTABLE_INSTALLER_HH_
#define _LIBPKGUPD_UNSTABLE_INSTALLER_HH_

#include "../recipe.hh"
#include "../config.hh"
#include "../database/database.hh"
#include <dlfcn.h>

#include <yaml-cpp/yaml.h>
#include "plugin.hh"
#include <chrono>
#include <ctime>

namespace rlxos::libpkgupd::unstable
{

    class installer : public Object
    {
    public:
        using plugin_init = plugin::installer *(*)(YAML::Node const &);

        typedef recipe (*getrecipe)(YAML::Node const &, std::string pkgpath);
        typedef std::tuple<bool, std::string> (*unpack)(YAML::Node const &, std::string pkgpath, std::string root_dir);

        typedef std::tuple<bool, std::string, std::vector<std::string>> (*unpack_func)(recipe const &, YAML::Node const &, package *, string pkgpath, string rootdir);
        typedef std::tuple<bool, std::string, recipe, package *> (*getrecipe_func)(string path);

    private:
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
                std::cout << "clearing cache" << std::endl;
                std::filesystem::remove_all(_dir_work);
            }
            else
                std::cout << "NO_CLEAN environ set, skipped cleaning" << std::endl;
        }

    public:
        installer(YAML::Node const &c)
            : _config(c), _database(c)
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

            _dir_work = std::filesystem::path(_dir_work) / "pkgupd";
        }

        ~installer()
        {
            clean();
        }

        std::vector<string> download(recipe const &_recipe, string const &_sub_pkg)
        {
            package *pkg = nullptr;
            for (auto i : _recipe.packages())
            {
                if (i.id() == _sub_pkg)
                {
                    pkg = &i;
                    break;
                }
            }

            if (pkg == nullptr)
            {
                error = _recipe.id() + "has no package with id '" + _sub_pkg + "'";
                return {};
            }

            string pkgid = _database.pkgid(_recipe.id(), pkg->id(), _recipe.version());
            string pkgfile = pkgid + "." + _recipe.pack(pkg);
            string pkgpath = _database.dir_pkgs() + "/" + _database.getrepo(_recipe.id()) + "/" + pkgfile;

            std::error_code ec;
            std::filesystem::create_directory(std::filesystem::path(pkgpath).parent_path(), ec);
            if (ec)
            {
                error = ec.message();
                return {};
            }

            if (std::filesystem::exists(pkgpath))
                std::cout << "Found in cache" << std::endl;
            else
            {
                if (!_database.get_from_server(_database.getrepo(_recipe.id()), pkgfile, pkgpath))
                {
                    error = _database.Error();
                    return {};
                }
            }

            return {pkgpath};
        }

        std::vector<string> download(recipe const &_recipe)
        {
            std::vector<string> pkglist;
            for (auto const &i : _recipe.packages())
            {
                auto pkgpath = download(_recipe, i.id());
                if (pkgpath.size() == 0)
                    return {};

                if (!std::filesystem::exists(pkgpath[0]))
                    return {};

                pkglist.push_back(pkgpath[0]);
            }

            return pkglist;
        }

        plugin::installer *get_plugin_from_path(string const &pkgpath)
        {
            assert(std::filesystem::exists(pkgpath));
            auto idx = pkgpath.find_last_of(".");
            string plugin = pkgpath.substr(idx + 1, pkgpath.length() - (idx + 1));
            if (plugin.length() == 0)
            {
                error = (pkgpath + " not a installable package");
                return nullptr;
            }

            return get_plugin(plugin);
        }

        plugin::installer *get_plugin(string const &plugin)
        {

            std::string pathToSearch = "/lib/pkgupd:/opt/lib/pkgupd";

            auto ENV_PATH = getenv("PKGUPD_PLUGINS");
            if (ENV_PATH != nullptr)
                pathToSearch += std::string(ENV_PATH);

            std::stringstream iter(pathToSearch);
            std::string path;

            std::string pluginPath;

            while (std::getline(iter, path, ':'))
            {
                std::string pluginTempPath = path + "/lib" + plugin + ".so";
                if (std::filesystem::exists(pluginPath))
                {
                    pluginPath = pluginTempPath;
                    break;
                }
            }

            if (pluginPath.length() == 0)
            {
                error = ("failed to find plugin '" + plugin + "'");
                return nullptr;
            }

            plugin_init _plug_init;

            {
                void *handler = dlopen(pluginPath.c_str(), RTLD_LAZY);
                if (!handler)
                {
                    error = dlerror();
                    return nullptr;
                }

                _plug_init = (plugin_init)dlsym(handler, "pkgupd_init");
                if (!_plug_init)
                {
                    error = dlerror();
                    return nullptr;
                }
            }

            return _plug_init(_config);
        }

        bool install(string const &pkgpath, bool skip_triggers = false)
        {
            auto plug = get_plugin_from_path(pkgpath);
            if (plug == nullptr)
                return false;

            auto [_recipe, pkg] = plug->get(pkgpath);

            if (_recipe == nullptr)
            {
                error = plug->Error();
                return false;
            }

            std::vector<string> fileslist;
            if (!plug->unpack(pkgpath, _database.dir_root(), fileslist))
            {
                error = plug->Error();
                delete plug;
                return false;
            }

            if (skip_triggers)
            {
                std::cout << "Skipping triggers" << std::endl;
            }
            else
            {
                if (!execute_triggers(_recipe, pkg, fileslist))
                    return false;

                for (auto const &i : _database.get_triggers(fileslist))
                {
                    if (int status = WEXITSTATUS(system(i.second.exec(i.first).c_str())); status != 0)
                    {
                        std::cerr << "Error while executing trigger" << std::endl;
                    }
                }
            }

            if (!register_pkg(_recipe, fileslist, _database.pkgid(_recipe->id(), pkg->id()), !skip_triggers))
            {
                return false;
            }

            return true;
        }

        bool execute_triggers(recipe *_recipe, package *pkg, std::vector<string> const &fileslist)
        {
            if (_recipe->postinstall().length())
            {
                int status = WEXITSTATUS(system(("bash -euc '" + _recipe->postinstall() + "'", "/tmp", _recipe->environ(pkg)).c_str()));
            }

            if (_recipe->groups().size())
            {
                for (auto const &i : _recipe->groups())
                    if (!i.exists())
                    {
                        io::info("creating group ", color::MAGENTA, i.name());
                        if (rlx::utils::exec::command(i.command()))
                            io::warn("failed to create required group ", i.name());
                    }
            }

            if (_recipe->users().size())
            {
                for (auto const &i : _recipe->users())
                {
                    io::debug(level::debug, "checking required user: ", color::MAGENTA, i.name(), color::RESET);
                    if (!i.exists())
                    {
                        io::info("creating user ", color::MAGENTA, i.name());
                        if (rlx::utils::exec::command(i.command()))
                            io::warn("failed to create required user ", i.name());
                    }
                }
            }

            return true;
        }

        bool register_pkg(recipe *_recipe, std::vector<string> fileslist, string pkgid, bool triggers_done = false)
        {
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

            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char buff[100] = {0};
            std::strftime(buff, sizeof(buff), "%Y-%m-%d", std::localtime(&now));

            auto node = _recipe->node();
            node["pkgid"] = pkgid;
            node["installed-on"] = string(buff);

            if (!triggers_done)
                io::writefile(data_file + ".trigger", "");

            io::writefile(data_file, node);
            io::writefile(list_file, algo::str::join(fileslist, "\n"));

            return true;
        }
    };

}

#endif