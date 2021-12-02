#ifndef _PKGUPD_DATABASE_HH_
#define _PKGUPD_DATABASE_HH_

#include "defines.hh"
#include "pkginfo.hh"

namespace rlxos::libpkgupd {
class db : public object {
 protected:
  std::string _data_dir;

 public:
  db(std::string const &datadir) : _data_dir{datadir} {
    if (!std::filesystem::exists(_data_dir)) {
      std::error_code ec;
      DEBUG("Creating database directory " << _data_dir);
      std::filesystem::create_directories(_data_dir, ec);
    }
  }

  GET_METHOD(std::string, data_dir);

  virtual std::shared_ptr<pkginfo> operator[](std::string const &pkgid) = 0;
  virtual std::vector<std::shared_ptr<pkginfo>> all() {
    throw std::runtime_error("INTERNAL ERROR: not yet implemented for " +
                             std::string(typeid(*this).name()));
  };
};
}  // namespace rlxos::libpkgupd

#endif