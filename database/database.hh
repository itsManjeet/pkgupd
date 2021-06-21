#ifndef __DATABASE__
#define __DATABASE__

#include "../recipe.hh"

namespace pkgupd
{
    class database : public obj
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

            char const *what() const
            {
                return _what.c_str();
            }
        };

    private:
        YAML::Node _config;
        string _dir_data,
            _dir_root,
            _dir_recipe;

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
            _dir_recipe = get_dir("recipe", DEFAULT_DIR_RECIPE);
        }

        recipe const operator[](std::string pkgid) const
        {
            try
            {
                return recipe(_dir_recipe + "/" + pkgid);
            }
            catch (YAML::BadFile const &e)
            {
                throw database::exception("No recipe file found for " + pkgid);
            }
        }

        std::string const pkgid(recipe const &_recipe, package *pkg) const
        {
            return _recipe.id() + (pkg == nullptr ? "" : ":" + pkg->id());
        }

        bool const installed(string const &pkgid) const
        {
            return std::filesystem::exists(_dir_data + "/" + pkgid);
        }

        bool const installed(recipe const &_recipe, package *pkg) const
        {
            return installed(pkgid(_recipe, pkg));
        }

        std::vector<string> installedfiles(recipe const &_recipe, package *pkg) const
        {
            auto id = pkgid(_recipe, pkg);
            if (!installed(id))
                throw database::exception(id + " is not already installed");

            string _list_file = _dir_data + "/" + id + ".files";
            if (!std::filesystem::exists(_list_file))
                throw database::exception("no file list exist for " + id);

            return algo::str::split(io::readfile(_list_file), '\n');
        }

        recipe const installed_recipe(recipe const &_recipe, package *pkg) const
        {
            auto id = pkgid(_recipe, pkg);
            if (!installed(id))
                throw database::exception(id + " is not already installed");

            string data_file = _dir_data + "/" + id;
            if (!std::filesystem::exists(data_file))
                throw database::exception("no data exist for " + id);

            return recipe(data_file);
        }
    };
}

#endif