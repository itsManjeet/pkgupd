#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include <grp.h>
#include <pwd.h>
#include <yaml-cpp/yaml.h>

#include <ostream>

#include "defines.hh"
#include "exec.hh"

namespace rlxos::libpkgupd {
class pkginfo {
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

        GET_METHOD(std::string, name);

        user(YAML::Node const &data, std::string const &file) {
            READ_VALUE(unsigned int, id);
            READ_VALUE(std::string, name);
            READ_VALUE(std::string, about);
            READ_VALUE(std::string, group);
            READ_VALUE(std::string, dir);
            READ_VALUE(std::string, shell);
        }

        bool exists() const {
            return getpwnam(_name.c_str()) != nullptr;
        }

        bool create() const {
            return exec().execute("useradd -c '" + _about + "' -d " + _dir + " -u " + std::to_string(_id) + " -g " + _group + " -s " + _shell + " " + _name) == 0;
        }

        void print(std::ostream &os) const {
            os << " - id: " << _id << "\n"
               << "   name: " << _name << "\n"
               << "   about: " << _about << "\n"
               << "   dir: " << _dir << "\n"
               << "   shell: " << _shell << std::endl;
        }
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

        group(YAML::Node const &data, std::string const &file) {
            READ_VALUE(unsigned int, id);
            READ_VALUE(std::string, name);
        }

        GET_METHOD(std::string, name);

        bool exists() const {
            return getgrnam(_name.c_str()) != nullptr;
        }

        bool create() const {
            return exec().execute("groupadd -g " + std::to_string(_id) + " " + _name) == 0;
        }

        void print(std::ostream &os) const {
            os << " - id: " << _id << "\n"
               << "   name: " << _name << std::endl;
        }
    };

   protected:
    std::vector<std::shared_ptr<pkginfo::user>> _users;
    std::vector<std::shared_ptr<pkginfo::group>> _groups;

   public:
    virtual std::string id() const = 0;
    virtual std::string version() const = 0;
    virtual std::string about() const = 0;

    virtual std::vector<std::shared_ptr<user>> users() const { return _users; }
    virtual std::vector<std::shared_ptr<group>> groups() const { return _groups; }

    virtual std::string packagefile(std::string ext = DEFAULT_EXTENSION) const {
        return id() + "-" + version() + "." + ext;
    }

    virtual std::vector<std::string> depends(bool) const = 0;
};
}  // namespace rlxos::libpkgupd

#endif