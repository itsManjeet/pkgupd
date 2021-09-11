#ifndef _LIBPKGUPD_UNSTABLE_RECIPE_HH_
#define _LIBPKGUPD_UNSTABLE_RECIPE_HH_

#include <string>
#include <vector>

#include <assert.h>
#include <pwd.h>
#include <grp.h>

#define _CHECK_VALUE(node, type, variable) \
    if (node[#variable])                   \
        _##variable = node[#variable].as<type>();

#define _THROW_ERROR(variable) \
    else throw std::runtime_error(#variable " is missing in recipe file");

#define _USE_FALLBACK(variable, fallback) \
    else _##variable = fallback;

#define GETVAL_TYPE(variable, node, type) \
    _CHECK_VALUE(node, type, variable)    \
    _THROW_ERROR(variable)

#define GETVAL(variable, node)                \
    _CHECK_VALUE(node, std::string, variable) \
    _THROW_ERROR(variable)

#define OPTVAL(variable, node, value)         \
    _CHECK_VALUE(node, std::string, variable) \
    _USE_FALLBACK(variable, value)

#define OPTVAL_TYPE(variable, node, value, type) \
    _CHECK_VALUE(node, type, variable)           \
    _USE_FALLBACK(variable, value)

#define DEFINE_GET_METHOD(type, variable) \
    type const &variable() const          \
    {                                     \
        return _##variable;               \
    }

namespace rlxos::libpkgupd::unstable
{
    using std::string;

    class user
    {
    private:
        unsigned _id;
        string _name,
            _group,
            _about,
            _shell,
            _dir;

    public:
        user(YAML::Node const &n)
        {
            GETVAL_TYPE(id, n, unsigned);
            GETVAL(name, n);
            OPTVAL(group, n, "");
            OPTVAL(about, n, "");
            OPTVAL(shell, n, "");
            OPTVAL(dir, n, "");
        }

        DEFINE_GET_METHOD(unsigned, id);
        DEFINE_GET_METHOD(string, name);
        DEFINE_GET_METHOD(string, group);
        DEFINE_GET_METHOD(string, about);
        DEFINE_GET_METHOD(string, shell);
        DEFINE_GET_METHOD(string, dir);

        std::vector<string> const command() const
        {
            return {"/bin/useradd", _name,
                    "-u", std::to_string(_id),
                    "-c", _about,
                    "-d", _dir,
                    "-s", _shell,
                    "-g", _group};
        }

        bool exists() const
        {
            return getpwnam(_name.c_str()) != nullptr;
        }
    };

    class group
    {
    private:
        unsigned _id;
        string _name;

    public:
        group(YAML::Node const &n)
        {
            GETVAL_TYPE(id, n, unsigned);
            GETVAL(name, n);
        }

        DEFINE_GET_METHOD(unsigned, id);
        DEFINE_GET_METHOD(string, name);

        std::vector<string> const command() const
        {
            return {"/bin/groupadd", _name, "-g", std::to_string(_id)};
        }

        bool exists() const
        {

            return getgrnam(_name.c_str()) != nullptr;
        }
    };

    class flag
    {
    private:
        string _id,
            _value;
        bool _only;

    public:
        flag(YAML::Node const &n)
        {
            GETVAL(id, n);
            GETVAL(value, n);
            OPTVAL_TYPE(only, n, false, bool);
        }

        DEFINE_GET_METHOD(string, id);
        DEFINE_GET_METHOD(string, value);
        DEFINE_GET_METHOD(bool, only);
    };

    class package
    {
    private:
        string _id,
            _dir,
            _plugin,
            _pack,
            _prescript,
            _script,
            _postscript;

        std::vector<string> _sources;
        std::vector<flag> _flags;
        std::vector<string> _environ;

    public:
        package(YAML::Node const &n)
        {
            GETVAL(id, n);
            OPTVAL(dir, n, "");
            OPTVAL(plugin, n, "auto");
            OPTVAL(pack, n, "");

            OPTVAL(prescript, n, "");
            OPTVAL(postscript, n, "");
            OPTVAL(script, n, "");

            if (n["sources"])
                for (auto const &i : n["sources"])
                    _sources.push_back(i.as<string>());

            if (n["flags"])
                for (auto const &i : n["flags"])
                    _flags.push_back(flag(i));

            if (n["environ"])
                for (auto const &i : n["environ"])
                    _environ.push_back(i.as<string>());
        }

        DEFINE_GET_METHOD(string, id);
        DEFINE_GET_METHOD(string, plugin);
        DEFINE_GET_METHOD(string, pack);

        DEFINE_GET_METHOD(string, script);
        DEFINE_GET_METHOD(string, prescript);
        DEFINE_GET_METHOD(string, postscript);

        DEFINE_GET_METHOD(std::vector<flag>, flags);
        DEFINE_GET_METHOD(std::vector<string>, sources);
        DEFINE_GET_METHOD(std::vector<string>, environ);

        DEFINE_GET_METHOD(std::string, dir);
    };

    class recipe
    {
    private:
        string _id,
            _version,
            _about,
            _prescript,
            _preinstall,
            _postscript,
            _path,
            _pack,
            _strip_script,
            _port,
            _postinstall;

        bool _clean = true,
             _strip = true,
             _compile = false;

        YAML::Node _node;

        std::vector<string> _sources,
            _environ,
            _runtime,
            _buildtime,
            _no_strip;

        std::vector<package> _packages;
        std::vector<user> _users;
        std::vector<group> _groups;

    public:
        recipe(string const &path)
            : _path(path)
        {
            _node = YAML::LoadFile(_path);

            GETVAL(id, _node);
            GETVAL(version, _node);
            GETVAL(about, _node);

            OPTVAL(prescript, _node, "");
            OPTVAL(postscript, _node, "");

            OPTVAL(pack, _node, "rlx");
            OPTVAL_TYPE(clean, _node, false, bool);
            OPTVAL_TYPE(strip, _node, true, bool);
            OPTVAL(preinstall, _node, "");
            OPTVAL(postinstall, _node, "");

            OPTVAL(strip_script, _node, "");

            OPTVAL(port, _node, "");

            OPTVAL_TYPE(compile, _node, false, bool);

            if (_node["sources"])
                for (auto const &i : _node["sources"])
                    _sources.push_back(i.as<string>());

            if (_node["no_strip"])
                for (auto const &i : _node["no_strip"])
                    _no_strip.push_back(i.as<string>());

            if (_node["packages"])
                for (auto const &i : _node["packages"])
                    _packages.push_back(package(i));

            if (_node["environ"])
                for (auto const &i : _node["environ"])
                    _environ.push_back(i.as<string>());

            if (_node["depends"])
            {
                if (_node["depends"] && _node["depends"]["runtime"])
                    for (auto const &i : _node["depends"]["runtime"])
                        _runtime.push_back(i.as<string>());

                if (_node["depends"] && _node["depends"]["buildtime"])
                    for (auto const &i : _node["depends"]["buildtime"])
                        _buildtime.push_back(i.as<string>());
            }

            if (_node["users"])
                for (auto const &i : _node["users"])
                    _users.push_back(user(i));

            if (_node["groups"])
                for (auto const &i : _node["groups"])
                    _groups.push_back(group(i));
        }

        DEFINE_GET_METHOD(string, id);
        DEFINE_GET_METHOD(string, version);
        DEFINE_GET_METHOD(string, about);
        DEFINE_GET_METHOD(string, prescript);
        DEFINE_GET_METHOD(string, postscript);

        DEFINE_GET_METHOD(string, path);
        DEFINE_GET_METHOD(string, port);

        DEFINE_GET_METHOD(string, preinstall);
        DEFINE_GET_METHOD(string, postinstall);

        DEFINE_GET_METHOD(std::vector<user>, users);
        DEFINE_GET_METHOD(std::vector<group>, groups);

        DEFINE_GET_METHOD(bool, clean);
        DEFINE_GET_METHOD(bool, compile);

        DEFINE_GET_METHOD(bool, strip);
        DEFINE_GET_METHOD(string, strip_script);

        DEFINE_GET_METHOD(YAML::Node, node);

        DEFINE_GET_METHOD(std::vector<string>, sources);
        DEFINE_GET_METHOD(std::vector<string>, buildtime);
        DEFINE_GET_METHOD(std::vector<string>, runtime);
        DEFINE_GET_METHOD(std::vector<string>, no_strip);

        std::vector<string> const environ(package *pkg = nullptr) const
        {
            // CHECK pkg != nullptr, just in case!
            if (pkg == nullptr || (pkg && pkg->environ().size() == 0))
                return _environ;

            if (_environ.size() == 0)
                return pkg->environ();

            auto _new_env = _environ;
            for (auto i : pkg->environ())
                _new_env.push_back(i);

            return _new_env;
        }

        void appendenviron(std::string const &s)
        {
            _environ.push_back(s);
        }

        DEFINE_GET_METHOD(std::vector<package>, packages);

        package *operator[](string subpkg)
        {
            if (subpkg.length() == 0)
                return nullptr;

            for (auto &p : _packages)
                if (p.id() == subpkg)
                    return new package(p);

            return nullptr;
        }

        string const pack(package *pkg) const
        {
            if (pkg == nullptr || (pkg && pkg->pack().length() == 0))
                return _pack;

            return pkg->pack();
        }

        string const dir(package *pkg) const
        {
            assert(pkg != nullptr);

            return pkg->dir();
        }

        friend std::ostream &operator<<(std::ostream &os, recipe const &rcp)
        {
            os << rcp.node();
            return os;
        }
    };
}

#endif