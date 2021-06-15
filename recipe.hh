#ifndef __RECIPE__
#define __RECIPE__

#include <yaml-cpp/yaml.h>
#include <utils/utils.hh>
#include <path/path.hh>

#include <string>
#include <vector>

#include <assert.h>

namespace pkgupd
{
    using std::string;

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

    inline string get_dir(string const &url)
    {
        string dir = rlx::path::basename(url);
        size_t idx = dir.find_first_of('.');
        if (idx == string::npos)
            return dir;

        return dir.substr(0, idx);
    }

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

        string const dir() const
        {
            if (_dir.length() == 0)
                if (_sources.size() == 0)
                    return "";
                else
                    return get_dir(_sources[0]);

            return _dir;
        }
    };

    class recipe
    {
    private:
        string _id,
            _version,
            _about,
            _prescript,
            _postscript,
            _path,
            _pack;

        YAML::Node _node;

        std::vector<string> _sources,
            _environ;

        std::vector<package> _packages;

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

            OPTVAL(pack, _node, "none");

            if (_node["sources"])
                for (auto const &i : _node["sources"])
                    _sources.push_back(i.as<string>());

            if (_node["packages"])
                for (auto const &i : _node["packages"])
                    _packages.push_back(package(i));

            if (_node["environ"])
                for (auto const &i : _node["environ"])
                    _environ.push_back(i.as<string>());
        }

        DEFINE_GET_METHOD(string, id);
        DEFINE_GET_METHOD(string, version);
        DEFINE_GET_METHOD(string, about);
        DEFINE_GET_METHOD(string, prescript);
        DEFINE_GET_METHOD(string, postscript);

        DEFINE_GET_METHOD(string, path);

        DEFINE_GET_METHOD(YAML::Node, node);

        DEFINE_GET_METHOD(std::vector<string>, sources);

        std::vector<string> const environ(package *pkg = nullptr) const
        {
            // CHECK pkg != nullptr, just in case!
            if (pkg == nullptr || (pkg && pkg->environ().size() == 0))
                return _environ;

            if (_environ.size() == 0)
                return pkg->environ();

            auto _new_env = _environ;
            _new_env.insert(
                _new_env.end(),
                pkg->environ().begin(), pkg->environ().end());

            return _new_env;
        }

        void appendenviron(std::string const &s)
        {
            _environ.push_back(s);
        }

        DEFINE_GET_METHOD(std::vector<package>, packages);

        string const pack(package *pkg) const
        {
            assert(pkg != nullptr);
            if (pkg->pack().length() == 0)
                return _pack;

            return pkg->pack();
        }

        string const dir(package *pkg) const
        {
            assert(pkg != nullptr);

            if (pkg->dir().length() != 0)
                return pkg->dir();

            if (_sources.size())
                return get_dir(_sources[0]);

            return "";
        }
    };
}

#endif