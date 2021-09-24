#include "recipe.hh"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <algorithm> // find_if

using std::string;

#define READ_COMMON()                                          \
    READ_LIST(string, sources);                                \
    READ_LIST(string, environ);                                \
    READ_LIST_FROM(string, runtime, depends, runtime_depends); \
    READ_LIST_FROM(string, buildtime, depends, buildtime_depends);

namespace rlxos::libpkgupd
{

    recipe::recipe(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, version);
        READ_VALUE(string, about);

        READ_COMMON();

        READ_OBJECT_LIST(user, users);
        READ_OBJECT_LIST(group, groups);

        READ_OBJECT_LIST(package, packages);
    }

    recipe::package::package(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, dir);

        READ_COMMON();

        OPTIONAL_VALUE(string, prescript, "");
        OPTIONAL_VALUE(string, postscript, "");
        OPTIONAL_VALUE(string, script, "");
        OPTIONAL_VALUE(string, preinstall, "");
        OPTIONAL_VALUE(string, postscript, "");

        OPTIONAL_VALUE(string, pack, "rlx");

        READ_LIST(string, skipstrip);
        OPTIONAL_VALUE(bool, strip, true);

        READ_OBJECT_LIST(flag, flags);
    }

    recipe::package::flag::flag(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, value);

        OPTIONAL_VALUE(bool, force, false);
        if (data["only"])
        {
            INFO("Use of 'only' in flags is deprecated, use 'force'")
            _force = data["only"].as<bool>();
        }
    }

    recipe::user::user(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(unsigned int, id);
        READ_VALUE(string, name);
        READ_VALUE(string, about);
        READ_VALUE(string, group);
        READ_VALUE(string, dir);
        READ_VALUE(string, shell);
    }

    recipe::group::group(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(unsigned int, id);
        READ_VALUE(string, name);
    }

    std::string recipe::package::id() const
    {
        if (_id == "lib" ||
            _id == "lib32")
            return _id + _parent->id();

        if (_id == "pkg")
            return _parent->_id;

        return _parent->_id + "-" + _id;
    }

    std::string recipe::package::version() const
    {
        return _parent->_version;
    }

    std::string recipe::package::about() const
    {
        return _parent->_about;
    }

    std::vector<std::string> recipe::package::depends(bool all) const
    {
        std::vector<string> depends = _runtime_depends;
        depends.insert(depends.end(), _parent->_runtime_depends.begin(), _parent->_runtime_depends.end());

        if (all)
        {
            depends.insert(depends.end(), _buildtime_depends.begin(), _buildtime_depends.end());
            depends.insert(depends.end(), _parent->_buildtime_depends.begin(), _parent->_buildtime_depends.end());
        }

        return depends;
    }

    std::vector<std::string> recipe::package::sources() const
    {
        std::vector<std::string> all_sources = _sources;
        all_sources.insert(all_sources.end(), _parent->_sources.begin(), _parent->_sources.end());
        return all_sources;
    }

    std::vector<std::string> recipe::package::environ()
    {
        std::vector<std::string> allenviron = _parent->_environ;
        allenviron.insert(allenviron.end(), _environ.begin(), _environ.end());
        return allenviron;
    }

    std::shared_ptr<recipe::package> recipe::operator[](std::string const &pkgid) const
    {
        auto pkgiter = std::find_if(
            _packages.begin(), _packages.end(),
            [&](std::shared_ptr<package> const &p)
            {
                if (pkgid == this->id() && (p->id() == "pkg"))
                    return true;

                if (pkgid == p->id())
                    return true;

                return false;
            });

        if (pkgiter == _packages.end())
            return nullptr;

        return (*pkgiter);
    }
}