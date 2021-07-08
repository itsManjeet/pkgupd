#ifndef __INSTALLER__
#define __INSTALLER__

#include <rlx.hh>
#include <algo/algo.hh>
#include "../recipe.hh"
#include "../config.hh"
#include "../database/database.hh"
#include "plugin.hh"
#include <chrono>
#include <ctime>
namespace pkgupd
{
    using namespace rlx;
    using color = rlx::io::color;
    using level = rlx::io::debug_level;

    class installer : public obj
    {
    public:
        using plugin_init = plugin::installer *(*)(YAML::Node const &);

        typedef recipe (*getrecipe)(YAML::Node const &, string pkgpath);
        typedef std::tuple<bool, string> (*unpack)(YAML::Node const &, string pkgpath, string root_dir);

        typedef std::tuple<bool, string, std::vector<string>> (*unpack_func)(recipe const &, YAML::Node const &, package *, string pkgpath, string rootdir);
        typedef std::tuple<bool, string, recipe, package *> (*getrecipe_func)(string path);

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
                io::process("clearing cache");
                std::filesystem::remove_all(_dir_work);
            }
            else
                io::debug(level::warn, "NO_CLEAN environ set, skip cleaning");
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

            _dir_work = rlx::utils::sys::tempdir(_dir_work, "pkgupd");
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
                _error = _recipe.id() + "has no package with id '" + _sub_pkg + "'";
                return {};
            }

            string pkgid = _database.pkgid(_recipe.id(), pkg->id(), _recipe.version());
            string pkgfile = pkgid + "." + _recipe.pack(pkg);
            string pkgpath = _database.dir_pkgs() + "/" + pkgfile;

            io::debug(level::debug, "pkgpath: ", pkgpath);
            if (std::filesystem::exists(pkgpath))
                io::info(pkgid, " found in cache");
            else
            {
                if (!_database.get_from_server(_database.getrepo(_recipe.id()), pkgfile, pkgpath))
                {
                    _error = _database.error();
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
            string plugin = std::filesystem::path(pkgpath).extension();
            plugin = plugin.substr(1, plugin.length() - 1);
            if (plugin.length() == 0)
                throw installer::exception(io::format(pkgpath, " not a installable package"));

            return get_plugin(plugin);
        }

        plugin::installer *get_plugin(string const &plugin)
        {

            string plugin_path = utils::dlmodule::search(plugin, "/lib/pkgupd:/usr/lib/pkgupd", "PKGUPD_PLUGINS");
            if (plugin_path.length() == 0)
                throw installer::exception(io::format("failed to find plugin '" + plugin + "'"));

            plugin_init _plug_init;
            try
            {
                _plug_init = utils::dlmodule::load<plugin_init>(plugin_path, "pkgupd_init");
            }
            catch (std::runtime_error const &e)
            {
                _error = e.what();
                return nullptr;
            }

            return _plug_init(_config);
        }

        bool install(string const &pkgpath, bool skip_scripts = false, bool skip_usrgrp = false, bool skip_triggers = false)
        {
            auto plug = get_plugin_from_path(pkgpath);
            if (plug == nullptr)
                return false;

            auto [_recipe, pkg] = plug->get(pkgpath);

            if (_recipe == nullptr)
            {
                _error = plug->error();
                return false;
            }

            if (_recipe->preinstall().length() && !skip_scripts)
            {
                io::info("executing preinstallation script");
                rlx::utils::exec::command("bash -euc '" + _recipe->preinstall() + "'", "/tmp", _recipe->environ(pkg));
            }

            std::vector<string> fileslist;
            if (!plug->unpack(pkgpath, _database.dir_root(), fileslist))
            {
                _error = plug->error();
                delete plug;
                return false;
            }

            if (_recipe->postinstall().length() && !skip_scripts)
            {
                io::info("executing postinstallation script");
                rlx::utils::exec::command("bash -euc '" + _recipe->postinstall() + "'", "/tmp", _recipe->environ(pkg));
            }

            if (_recipe->groups().size() && !skip_usrgrp)
            {
                for (auto const &i : _recipe->groups())
                    if (!i.exists())
                    {
                        io::info("creating group ", color::MAGENTA, i.name());
                        if (rlx::utils::exec::command(i.command()))
                            io::warn("failed to create required group ", i.name());
                    }
            }

            if (_recipe->users().size() && !skip_usrgrp)
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

            if (!_database.exec_triggers(fileslist) && !skip_triggers)
            {
                _error = _database.error();
                return false;
            }

            return true;
        }

        bool register_pkg(recipe *_recipe, std::vector<string> fileslist, string pkgid, bool scripts_done = false, bool usrgrp_done = false, bool triggers_done = false)
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
            node["script-done"] = scripts_done;
            node["triggers-done"] = triggers_done;
            node["usrgrp-done"] = usrgrp_done;

            io::writefile(data_file, node);
            io::writefile(list_file, algo::str::join(fileslist, "\n"));

            return true;
        }
    };

}

#endif