#ifndef _PKGUPD_DATABASE_HH_
#define _PKGUPD_DATABASE_HH_

#include <optional>

#include "defines.hh"
#include "package.hh"

namespace rlxos::libpkgupd {
class Database : public Object {
 protected:
  std::string p_DataDir;

 public:
  Database(std::string const &datadir) : p_DataDir(datadir) {
    if (!std::filesystem::exists(p_DataDir)) {
      std::error_code ec;
      DEBUG("Creating database directory " << p_DataDir);
      std::filesystem::create_directories(p_DataDir, ec);
    }
  }

  std::string const &path() const { return p_DataDir; }

  virtual std::optional<Package> operator[](std::string const &pkgid) = 0;
  virtual std::vector<Package> all() {
    throw std::runtime_error("INTERNAL ERROR: not yet implemented for " +
                             std::string(typeid(*this).name()));
  };
};
}  // namespace rlxos::libpkgupd

#endif