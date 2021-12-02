#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include "db.hh"

namespace rlxos::libpkgupd {

class sysdb : public db {
 public:
  class package : public pkginfo {
   private:
    std::string _id;
    std::string _version;
    std::string _about;

    std::vector<std::string> _depends, _files;

    std::string _installed_on;

    std::string _required_by;

   public:
    package(YAML::Node const &data, std::string const &file);

    std::string id() const { return _id; }
    std::string version() const { return _version; }
    std::string about() const { return _about; }
    std::vector<std::string> depends(bool) const { return _depends; }

    GET_METHOD(std::vector<std::string>, files);
    GET_METHOD(std::string, installed_on);
    GET_METHOD(std::string, required_by);
  };
  sysdb(std::string const &d) : db(d) {
    DEBUG("System Database: " << _data_dir);
  }

  std::shared_ptr<pkginfo> operator[](std::string const &pkgid);

  std::vector<std::shared_ptr<pkginfo>> all();

  bool is_installed(std::shared_ptr<pkginfo> const &pkginfo);

  bool outdated(std::shared_ptr<pkginfo> const &pkginfo);

  bool add(std::shared_ptr<pkginfo> const &pkginfo,
           std::vector<std::string> const &files, std::string root,
           bool update = false);

  bool remove(std::shared_ptr<pkginfo> const &pkginfo);
};
}  // namespace rlxos::libpkgupd

#endif