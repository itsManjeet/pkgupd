#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include <grp.h>
#include <pwd.h>
#include <yaml-cpp/yaml.h>

#include <ostream>

#include "defines.hh"
#include "exec.hh"

namespace rlxos::libpkgupd {
class PackageInformation {
 public:
  enum class PackageType : int {
    APP,
    RLX,
    PKG,
  };
  class User {
   private:
    unsigned int _id;
    std::string _name, _about, _dir, _shell, _group;

   public:
    User(unsigned int id, std::string const &name, std::string const &about,
         std::string const &dir, std::string const &shell,
         std::string const &group)
        : _id(id),
          _name(name),
          _about(about),
          _dir(dir),
          _shell(shell),
          _group(group) {}

    GET_METHOD(std::string, name);

    User(YAML::Node const &data, std::string const &file) {
      READ_VALUE(unsigned int, id);
      READ_VALUE(std::string, name);
      READ_VALUE(std::string, about);
      READ_VALUE(std::string, group);
      READ_VALUE(std::string, dir);
      READ_VALUE(std::string, shell);
    }

    bool exists() const { return getpwnam(_name.c_str()) != nullptr; }

    bool create() const {
      return Executor().execute("useradd -c '" + _about + "' -d " + _dir + " -u " +
                            std::to_string(_id) + " -g " + _group + " -s " +
                            _shell + " " + _name) == 0;
    }

    void print(std::ostream &os) const {
      os << " - id: " << _id << "\n"
         << "   name: " << _name << "\n"
         << "   about: " << _about << "\n"
         << "   group: " << _group << "\n"
         << "   dir: " << _dir << "\n"
         << "   shell: " << _shell << std::endl;
    }
  };

  class Group {
   private:
    unsigned int _id;
    std::string _name;

   public:
    Group(unsigned int id, std::string const &name) : _id(id), _name(name) {}

    Group(YAML::Node const &data, std::string const &file) {
      READ_VALUE(unsigned int, id);
      READ_VALUE(std::string, name);
    }

    GET_METHOD(std::string, name);

    bool exists() const { return getgrnam(_name.c_str()) != nullptr; }

    bool create() const {
      return Executor().execute("groupadd -g " + std::to_string(_id) + " " +
                            _name) == 0;
    }

    void print(std::ostream &os) const {
      os << " - id: " << _id << "\n"
         << "   name: " << _name << std::endl;
    }
  };

 protected:
  std::vector<std::shared_ptr<PackageInformation::User>> _users;
  std::vector<std::shared_ptr<PackageInformation::Group>> _groups;
  std::string _install_script;

 public:
  virtual std::string id() const = 0;
  virtual std::string version() const = 0;
  virtual std::string about() const = 0;
  virtual PackageType type() const = 0;

  virtual std::vector<std::shared_ptr<User>> users() const { return _users; }
  virtual std::vector<std::shared_ptr<Group>> groups() const { return _groups; }

  static PackageType str2pkgtype(std::string const &t) {
    if (t == "app") {
      return PackageType::APP;
    } else if (t == "rlx") {
      return PackageType::RLX;
    } else if (t == "pkg") {
      return PackageType::PKG;
    } else {
      throw std::runtime_error("invalid pkgtype " + t + " specified");
    }
  }

  static std::string pkgtype2str(PackageType t) {
    switch (t) {
      case PackageType::APP:
        return "app";
      case PackageType::PKG:
        return "pkg";
      case PackageType::RLX:
        return "rlx";
      default:
        throw std::runtime_error("invalid pkgtype specified");
    }
  }
  virtual std::string packagefile() const {
    std::string ext = pkgtype2str(type());
    return id() + "-" + version() + "." + ext;
  }

  virtual std::vector<std::string> depends(bool) const = 0;

  std::string const &install_script() const { return _install_script; };
};
}  // namespace rlxos::libpkgupd

#endif