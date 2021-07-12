#ifndef __DATABASE__
#define __DATABASE__

#include <rlx.hh>
#include <algo/algo.hh>
#include <curl/curl.hh>
#include "../recipe.hh"
#include "../config.hh"

namespace pkgupd
{
    class database : public rlx::obj
    {
    public:
        class exception : public std::exception
        {
        private:
            string _what;

        public:
            exception(string const &c)
                : _what(c)
            {
            }

            const char *what() const noexcept
            {
                return _what.c_str();
            }
        };

    private:
        YAML::Node _config;
        string _dir_data,
            _dir_root,
            _dir_recipe,
            _dir_pkgs,
            _dir_src,
            _dir_work;

        std::vector<string> _repositories;
        std::vector<string> __depid;

        std::vector<string> _visited;

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

            if (c["default"] && c["default"]["repositories"])
                for (auto const &i : c["default"]["repositories"])
                    _repositories.push_back(i.as<string>());
            else
                _repositories.push_back("core");

            for (auto const &i : {_dir_root, _dir_data, _dir_recipe, _dir_pkgs, _dir_work, _dir_src})
                if (!std::filesystem::exists(i))
                    std::filesystem::create_directories(i);
        }

        DEFINE_GET_METHOD(string, dir_root);
        DEFINE_GET_METHOD(string, dir_data);
        DEFINE_GET_METHOD(string, dir_recipe);
        DEFINE_GET_METHOD(string, dir_pkgs);
        DEFINE_GET_METHOD(string, dir_work);
        DEFINE_GET_METHOD(string, dir_src);
        DEFINE_GET_METHOD(std::vector<string>, repositories);

        string const getrepo(string const &id) const
        {
            for (auto const &i : _repositories)
            {
                io::debug(level::trace, "checking ", i, " for ", id);
                string rcp_path = _dir_recipe + "/" + i + "/" + id + ".yml";
                if (std::filesystem::exists(rcp_path))
                    return i;
            }
            throw database::exception("No recipe file found for " + id);
        }

        recipe const operator[](std::string pkgid) const
        {
            auto [rcp, p] = parse_pkgid(pkgid);
            return _dir_recipe + "/" + getrepo(rcp) + "/" + rcp + ".yml";
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
                total_deps = rlx::algo::merge(total_deps, pkg.buildtime());

            for (auto const &i : total_deps)
            {
                auto dep = (*this)[i];
                if (std::find(__depid.begin(), __depid.end(), i) != __depid.end() || installed(i))
                    continue;

                resolve(i, compiletime);
                if (std::find(__depid.begin(), __depid.end(), i) == __depid.end())
                    __depid.push_back(i);
            }

            if (std::find(__depid.begin(), __depid.end(), pkgid) == __depid.end())
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
            if (std::filesystem::exists(_dir_data + "/" + pkgid))
                return true;

            if (pkgid.find(':') == string::npos)
            {
                /* check if all the packages of recipe is installed or not */
                auto _recipe = (*this)[pkgid];
                for (auto const &i : _recipe.packages())
                    if (!installed(_recipe.id() + ":" + i.id()))
                        return false;

                return _recipe.packages().size() != 0;
            }

            // check if recipe is a meta package
            return false;
        }

        bool const installed(recipe const &_recipe, package *pkg) const
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
                catch (database::exception e)
                {
                    io::warn(e.what());
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
            for (auto const &mirror : mirrors)
            {
                io::process("getting ", pkgid, " ", mirror, " ", repo);
                string url = mirror + "/" + repo + "/" + pkgid;
                if (!rlx::curl::download(url, output))
                {
                    io::error("faild to get from ", mirror);
                }
                else
                {
                    downloaded = true;
                    break;
                }
            }

            if (!downloaded)
                _error = "failed to download " + pkgid;

            return downloaded;
        }

        std::vector<string> installedfiles(string pkgid) const
        {
            if (!installed(pkgid))
                throw database::exception(pkgid + " is not already installed");

            string _list_file = _dir_data + "/" + pkgid + ".files";
            if (!std::filesystem::exists(_list_file))
                throw database::exception("no file list exist for " + pkgid);

            return algo::str::split(io::readfile(_list_file), '\n');
        }

        std::vector<string> installedfiles(recipe const &_recipe, package *pkg) const
        {
            return installedfiles(pkgid(_recipe.id(), pkg == nullptr ? "" : pkg->id()));
        }

        recipe const installed_recipe(string const &id) const
        {
            if (!installed(id))
                throw database::exception(id + " is not already installed");

            string data_file = _dir_data + "/" + id;
            if (!std::filesystem::exists(data_file))
                throw database::exception("no data exist for " + id);

            return recipe(data_file);
        }

        bool exec_triggers(std::vector<string> const &fileslist)
        {
            return true;
        }
    };
}

#endif