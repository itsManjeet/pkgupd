#ifndef _PKGUPD_PKGINFO_HH_
#define _PKGUPD_PKGINFO_HH_

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <string>
#include <vector>

#include "pkginfo.hh"

namespace rlxos::libpkgupd {

class Recipe : public std::enable_shared_from_this<Recipe> {
 public:
  class Package : public PackageInformation {
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
    std::string _id, _dir;
    bool _strip;

    std::string _pack;

    std::vector<std::string> _skipstrip;

    std::vector<std::string> _runtime_depends;
    std::vector<std::string> _buildtime_depends;

    std::vector<std::string> _sources;

    std::vector<std::string> _environ;

    std::string _script;
    std::string _prescript, _postscript;

    std::vector<std::shared_ptr<flag>> _flags;

    std::shared_ptr<Recipe> _parent;

   public:
    Package(YAML::Node const &data, std::string const &file);

    METHOD(std::shared_ptr<Recipe>, parent);

    std::string id() const;

    std::string version() const;

    std::string about() const;

    PackageType type() const;

    std::vector<std::string> depends(bool all) const;

    std::vector<std::string> sources() const;

    std::vector<std::shared_ptr<PackageInformation::User>> users() const {
      return _parent->_users;
    }

    std::vector<std::shared_ptr<PackageInformation::Group>> groups() const {
      return _parent->_groups;
    }

    std::shared_ptr<flag> getflag(std::string const &i) {
      auto iter = std::find_if(
          _flags.begin(), _flags.end(),
          [&](std::shared_ptr<flag> const &f) -> bool { return f->id() == i; });

      if (iter == _flags.end()) {
        return nullptr;
      }
      return *iter;
    }

    GET_METHOD(std::string, script);

    GET_METHOD(std::string, prescript);

    GET_METHOD(std::string, postscript);

    GET_METHOD(std::vector<std::shared_ptr<flag>>, flags);

    GET_METHOD(std::string, pack);

    GET_METHOD(std::string, dir);

    GET_METHOD(std::vector<std::string>, skipstrip);

    GET_METHOD(bool, strip);

    std::vector<std::string> environ();

    void prepand_environ(std::string const &env) {
      _environ.insert(_environ.begin(), env);
    }

    void append_environ(std::string const &env) { _environ.push_back(env); }
  };

 private:
  std::string _id;
  std::string _version;
  std::string _about;

  std::vector<std::string> _runtime_depends;
  std::vector<std::string> _buildtime_depends;

  std::vector<std::string> _sources;

  std::vector<std::string> _environ;

  std::vector<std::shared_ptr<PackageInformation::User>> _users;
  std::vector<std::shared_ptr<PackageInformation::Group>> _groups;

  std::vector<std::shared_ptr<Package>> _packages;

  bool _split = false;

  YAML::Node _node;

 public:
  Recipe(YAML::Node const &node, std::string const &file);

  static std::shared_ptr<Recipe> from_filepath(std::string const &filepath) {
    auto ptr = std::make_shared<Recipe>(YAML::LoadFile(filepath), filepath);
    for (auto &pkg : ptr->_packages) pkg->parent(ptr->shared_from_this());

    return ptr;
  }

  static std::shared_ptr<Recipe> from_yaml(YAML::Node const &node) {
    auto ptr = std::make_shared<Recipe>(node, "");
    for (auto &pkg : ptr->_packages) pkg->parent(ptr->shared_from_this());

    return ptr;
  }

  std::shared_ptr<Package> operator[](std::string const &pkgid) const;

  GET_METHOD(std::vector<std::shared_ptr<Package>>, packages);
  GET_METHOD(std::string, id);
  GET_METHOD(bool, split);

  GET_METHOD(YAML::Node, node);

  friend class Recipe::Package;
};

}  // namespace rlxos::libpkgupd

#endif
