#ifndef _PKGUPD_PKGINFO_HH_
#define _PKGUPD_PKGINFO_HH_

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

#include "pkginfo.hh"

namespace rlxos::libpkgupd {

class recipe : public std::enable_shared_from_this<recipe> {
   public:
    class user {
       private:
        unsigned int _id;
        std::string _name, _about, _dir, _shell, _group;

       public:
        user(unsigned int id,
             std::string const &name,
             std::string const &about,
             std::string const &dir,
             std::string const &shell,
             std::string const &group)
            : _id(id),
              _name(name),
              _about(about),
              _dir(dir),
              _shell(shell),
              _group(group) {
        }

        user(YAML::Node const &data, std::string const &file);

        bool exists() const;

        bool create() const;
    };

    class group {
       private:
        unsigned int _id;
        std::string _name;

       public:
        group(unsigned int id,
              std::string const &name)
            : _id(id),
              _name(name) {}

        group(YAML::Node const &data, std::string const &file);

        bool exists() const;

        bool create() const;
    };

    class package : public pkginfo {
       public:
        class flag {
           private:
            std::string _id, _value;

            bool _force;

           public:
            flag(YAML::Node const &data, std::string const &file);

            GET_METHOD(std::string, id);
            GET_METHOD(std::string, value);
            GET_METHOD(bool, force);
        };

       private:
        std::string _id,
            _dir;
        bool _strip;

        std::string _pack;

        std::vector<std::string> _skipstrip;

        std::vector<std::string> _runtime_depends;
        std::vector<std::string> _buildtime_depends;

        std::vector<std::string> _sources;

        std::vector<std::string> _environ;

        std::string _script;
        std::string _prescript, _postscript;
        std::string _install_script;

        std::vector<std::shared_ptr<flag>> _flags;

        std::shared_ptr<recipe> _parent;

       public:
        package(YAML::Node const &data, std::string const &file);

        METHOD(std::shared_ptr<recipe>, parent);

        std::string id() const;

        std::string version() const;

        std::string about() const;

        std::vector<std::string> depends(bool all) const;

        std::vector<std::string> sources() const;

        GET_METHOD(std::string, script);
        GET_METHOD(std::string, prescript);
        GET_METHOD(std::string, postscript);

        GET_METHOD(std::string, install_script);

        GET_METHOD(std::vector<std::shared_ptr<flag>>, flags);

        GET_METHOD(std::string, pack);

        GET_METHOD(std::string, dir);

        GET_METHOD(std::vector<std::string>, skipstrip);

        GET_METHOD(bool, strip);

        std::vector<std::string> environ();

        void prepand_environ(std::string const &env) {
            _environ.insert(_environ.begin(), env);
        }

        void append_environ(std::string const &env) {
            _environ.push_back(env);
        }
    };

   private:
    std::string _id;
    std::string _version;
    std::string _about;

    std::vector<std::string> _runtime_depends;
    std::vector<std::string> _buildtime_depends;

    std::vector<std::string> _sources;

    std::vector<std::string> _environ;

    std::vector<std::shared_ptr<user>> _users;
    std::vector<std::shared_ptr<group>> _groups;

    std::vector<std::shared_ptr<package>> _packages;

    bool _split = false;

   public:
    recipe(YAML::Node const &node, std::string const &file);

    static std::shared_ptr<recipe> from_filepath(std::string const &filepath) {
        auto ptr = std::make_shared<recipe>(YAML::LoadFile(filepath), filepath);
        for (auto &pkg : ptr->_packages)
            pkg->parent(ptr->shared_from_this());

        return ptr;
    }

    static std::shared_ptr<recipe> from_yaml(YAML::Node const &node) {
        auto ptr = std::make_shared<recipe>(node, "");
        for (auto &pkg : ptr->_packages)
            pkg->parent(ptr->shared_from_this());

        return ptr;
    }

    std::shared_ptr<package> operator[](std::string const &pkgid) const;

    GET_METHOD(std::vector<std::shared_ptr<package>>, packages);
    GET_METHOD(std::string, id);
    GET_METHOD(bool, split);

    friend class recipe::package;
};

}  // namespace rlxos::libpkgupd

#endif
