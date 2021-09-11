#ifndef _LIBPKGUPD_UNSTABLE_DATABASE_HH_
#define _LIBPKGUPD_UNSTABLE_DATABASE_HH_

#include "../recipe.hh"
#include "../config.hh"
#include <regex>
#include <iostream>
#include <fstream>
#include <string>

#include "../../Downloader.hh"

namespace rlxos::libpkgupd::unstable
{
    class database : public Object
    {
    public:
        class trigger
        {
        private:
            string _id;
            string _message;
            string _regex;
            bool _ondir;
            bool _args;
            YAML::Node _node;
            string _exec;

        public:
            trigger(string const &trigpath)
            {
                _node = YAML::LoadFile(trigpath);
                GETVAL(id, _node);
                GETVAL(message, _node);
                GETVAL(regex, _node);
                GETVAL(exec, _node);

                OPTVAL_TYPE(args, _node, false, bool);
                OPTVAL_TYPE(ondir, _node, true, bool);
            }

            DEFINE_GET_METHOD(string, id);
            DEFINE_GET_METHOD(string, message);
            DEFINE_GET_METHOD(string, regex);

            string const exec(string path) const
            {
                if (path[0] == '.')
                    path = path.substr(1, path.length() - 1);

                if (!std::regex_match(path, std::regex(_regex)))
                    return "";

                return _exec + (_args ? " " + (_ondir ? std::filesystem::path(path).parent_path().string() : path) : "");
            }
        };

    private:
        YAML::Node _config;
        string _dir_data,
            _dir_root,
            _dir_recipe,
            _dir_pkgs,
            _dir_src,
            _dir_work,
            _dir_triggers;

        std::vector<string> _repositories;
        std::vector<string> __depid;

        std::vector<string> _visited;
        std::vector<trigger> _triggers;

    public:
        database(YAML::Node const &c)
            : _config(c)
        {
            auto get_dir = [&](string path, string fallback) -> string
            {
                if (c["dir"] && c["dir"][path])
                    return c["dir"][path].as<string>();
                return fallback;
            };

            _dir_root = get_dir("root", "/");
            _dir_data = get_dir("data", DEFAULT_DIR_DATA);
            _dir_recipe = get_dir("recipes", DEFAULT_DIR_RECIPE);
            _dir_pkgs = get_dir("pkgs", DEFAULT_DIR_PKGS);
            _dir_work = get_dir("work", DEFAULT_DIR_WORK);
            _dir_src = get_dir("src", DEFAULT_DIR_SRC);
            _dir_triggers = get_dir("triggers", DEFAULT_DIR_TRIGGERS);

            if (c["default"] && c["default"]["repositories"])
                for (auto const &i : c["default"]["repositories"])
                    _repositories.push_back(i.as<string>());
            else
                _repositories.push_back("core");

            for (auto const &i : {_dir_root, _dir_data, _dir_recipe,
                                  _dir_pkgs, _dir_work, _dir_src,
                                  _dir_triggers})
                if (!std::filesystem::exists(i))
                    std::filesystem::create_directories(i);

            for (auto const &i : std::filesystem::directory_iterator(dir_triggers()))
            {
                if (i.path().extension() == ".yml")
                {
                    _triggers.push_back(trigger(i.path().string()));
                }
            }
        }

        DEFINE_GET_METHOD(string, dir_root);
        DEFINE_GET_METHOD(string, dir_data);
        DEFINE_GET_METHOD(string, dir_recipe);
        DEFINE_GET_METHOD(string, dir_pkgs);
        DEFINE_GET_METHOD(string, dir_work);
        DEFINE_GET_METHOD(string, dir_src);
        DEFINE_GET_METHOD(string, dir_triggers);
        DEFINE_GET_METHOD(std::vector<string>, repositories);

        string const getrepo(string const &id) const
        {
            for (auto const &i : _repositories)
            {
                string rcp_path = _dir_recipe + "/" + i + "/" + id + ".yml";
                if (std::filesystem::exists(rcp_path))
                    return i;
            }
            throw std::runtime_error("No repository found");
        }

        recipe const operator[](std::string pkgid) const
        {
            auto [rcp, p] = parse_pkgid(pkgid);
            auto rpath = _dir_recipe + "/" + getrepo(rcp) + "/" + rcp + ".yml";
            if (!std::filesystem::exists(rpath))
                throw std::runtime_error("recipe file for " + pkgid + " not exist");

            return rpath;
        }

        std::vector<string> resolve(string pkgid, bool compiletime = false)
        {

            auto [rcp, p] = parse_pkgid(pkgid);
            auto pkg = (*this)[rcp];

            if (std::find(__depid.begin(), __depid.end(), pkgid) != __depid.end() || installed(pkgid))
                return __depid;

            if (std::find(_visited.begin(), _visited.end(), pkgid) == _visited.end())
                _visited.push_back(pkgid);
            else
                return __depid;

            auto total_deps = pkg.runtime();
            if (compiletime)
            {
                for (auto const &b : pkg.buildtime())
                    total_deps.push_back(b);
            }

            for (auto const &i : total_deps)
            {
                auto dep = (*this)[i];
                if (std::find(__depid.begin(), __depid.end(), i) != __depid.end() || installed(i))
                    continue;

                auto [_rcp, _p] = parse_pkgid(i);

                resolve(i, compiletime);
                if (std::find(__depid.begin(), __depid.end(), i) == __depid.end() &&
                    std::find(__depid.begin(), __depid.end(), _rcp) == __depid.end())
                    __depid.push_back(i);
            }

            if (std::find(__depid.begin(), __depid.end(), pkgid) == __depid.end() &&
                std::find(__depid.begin(), __depid.end(), rcp) == __depid.end())
                __depid.push_back(pkgid);

            return __depid;
        }

        std::string const pkgid(string const &_recipe_id, string const &_pkg_id = "", string const &_recipe_version = "") const
        {
            if (getenv("USE_APPCTL_FORMAT") != nullptr)
            {
                string rel = getenv("APPCTL_RELEASE") == nullptr ? "1" : getenv("APPCTL_RELEASE");
                return (_pkg_id.length() == 0 ? _recipe_id : _pkg_id) + (_recipe_version.length() ? "-" + _recipe_version + "-" + rel + "-x86_64" : "");
            }

            return _recipe_id + (_recipe_version.length() ? "-" : "") + _recipe_version + (_pkg_id.length() ? ":" : "") + _pkg_id;
        }

        /** @brief Check if pkg is already installed or not
         *  @param pkgid packages id in format <recipe>:<package>
         * 
         */
        bool const installed(string const &pkgid) const
        {
            if (pkgid.find(':') == string::npos)
            {
                /* check if all the packages of recipe is installed or not */
                auto _recipe = (*this)[pkgid];
                for (auto const &i : _recipe.packages())
                    if (!installed(_recipe.id() + ":" + i.id()))
                        return false;

                return _recipe.packages().size() != 0;
            }
            else
            {
                if (std::filesystem::exists(_dir_data + "/" + pkgid))
                    return true;
            }

            return false;
        }

        bool const installed(recipe *_recipe) const
        {

            /* check if all the packages of recipe is installed or not */

            for (auto const &i : _recipe->packages())
                if (!installed(_recipe->id() + ":" + i.id()))
                    return false;

            return _recipe->packages().size() != 0;
        }

        bool const
        installed(recipe const &_recipe, package *pkg) const
        {
            return installed(pkgid(_recipe.id(), pkg == nullptr ? "" : pkg->id()));
        }

        std::tuple<string, string> parse_pkgid(string pkgid) const
        {
            size_t idx = pkgid.find_first_of(':');
            if (idx == string::npos)
                return {pkgid, ""};

            return {pkgid.substr(0, idx), pkgid.substr(idx + 1, pkgid.length() - (idx + 1))};
        }

        std::vector<string> list_out_dated() const
        {
            auto outdatedlist = std::vector<string>();
            for (auto const &i : std::filesystem::directory_iterator(_dir_data))
            {
                if (i.path().filename().extension() == ".files")
                    continue;

                auto [pkgid, subpkg] = parse_pkgid(i.path().filename().string());

                try
                {
                    auto _recipe = (*this)[pkgid];
                    auto _installed = installed_recipe(i.path().filename().string());

                    if (_recipe.version() != _installed.version())
                        outdatedlist.push_back(i.path().filename().string());
                }
                catch (std::exception const &err)
                {
                    std::cerr << err.what() << std::endl;
                    continue;
                }
            }

            return outdatedlist;
        }

        bool get_from_server(string repo, string pkgid = "", string output = "a.out")
        {
            if (pkgid.length() == 0)
                pkgid = "recipes";

            std::vector<string> mirrors;
            if (_config["mirrors"])
                for (auto const &i : _config["mirrors"])
                    mirrors.push_back(i.as<string>());
            else
                mirrors.push_back(DEFAULT_MIRROR);

            bool downloaded = false;

            Downloader mDownloader;
            for (auto const &mirror : mirrors)
                mDownloader.AddURL(mirror + "/" + repo);

            if (!mDownloader.Download(pkgid, output))
            {
                error = mDownloader.Error();
                return false;
            }

            return true;
        }

        std::vector<string> installedfiles(string pkgid) const
        {
            if (!installed(pkgid))
                throw std::runtime_error(pkgid + " is not already installed");

            string _list_file = _dir_data + "/" + pkgid + ".files";
            if (!std::filesystem::exists(_list_file))
                throw std::runtime_error("no file list exist for " + pkgid);

            std::ifstream file(_list_file);
            if (!file.good())
                throw std::runtime_error("failed to open file for read " + pkgid);

            std::vector<std::string> fileslist;
            {
                std::string line;
                while (std::getline(file, line, '\n'))
                    fileslist.push_back(line);
            }

            return fileslist;
        }

        std::vector<string> installedfiles(recipe const &_recipe, package *pkg) const
        {
            return installedfiles(pkgid(_recipe.id(), pkg == nullptr ? "" : pkg->id()));
        }

        recipe const installed_recipe(string const &id) const
        {
            if (!installed(id))
                throw std::runtime_error(id + " is not already installed");

            string data_file = _dir_data + "/" + id;
            if (!std::filesystem::exists(data_file))
                throw std::runtime_error("no data exist for " + id);

            return recipe(data_file);
        }

        std::map<string, trigger> get_triggers(std::vector<string> const &fileslist)
        {
            std::map<string, trigger> _needed;
            for (auto const &f : fileslist)
            {
                for (auto const &t : _triggers)
                {
                    auto cmd = t.exec(f);
                    if (cmd.length() != 0)
                    {
                        _needed.insert(std::make_pair(f, t));
                        break;
                    }
                }
            }

            return _needed;
        }
    };
}

#endif