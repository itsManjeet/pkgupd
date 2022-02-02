#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include <yaml-cpp/yaml.h>

#include <optional>

#include "defines.hh"
#include "package.hh"

namespace rlxos::libpkgupd {
/**
 * This class represent rlxos compressed Package,
 * @brief It provides various methods to handle, read, compress and extract
 * rlxos packages
 */
class Packager : public Object {
 protected:
  std::string m_PackageFile;
  Package m_Package;

 public:
  Packager(std::string const &packageFile) : m_PackageFile(packageFile) {}

  /**
   * @brief Provides the file data of specified file in the package
   * @param filepath path to the file in Package (must be started from ./)
   * @return content of file
   */
  virtual std::tuple<int, std::string> get(std::string const &filepath) = 0;

  virtual std::optional<Package> info() = 0;
  /**
   * List all files in the archive
   */
  virtual std::vector<std::string> list() = 0;

  bool exists(std::string const &path) {
    auto [status, output] = get(path);
    return status == 0;
  }

  virtual bool extract(std::string const &output) = 0;

  virtual bool compress(std::string const &input, Package const &package) = 0;

  static std::shared_ptr<Packager> create(PackageType packageType,
                                          std::string const &packageFile);

  static std::shared_ptr<Packager> create(std::string const &packageFile);
};
}  // namespace rlxos::libpkgupd

#endif