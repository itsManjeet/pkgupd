#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include <yaml-cpp/yaml.h>

#include "defines.hh"
#include "pkginfo.hh"

namespace rlxos::libpkgupd {
/**
 * This class represent rlxos compressed Package,
 * @brief It provides various methods to handle, read, compress and extract
 * rlxos packages
 */
class Archive : public Object {
 public:
  class Package : public PackageInformation {
   private:
    std::string _id, _version, _about;
    std::string _script;
    PackageType _type;

    std::vector<std::string> _depends;

   public:
    Package(YAML::Node const &data, std::string const &file);

    std::string id() const { return _id; }
    std::string version() const { return _version; }
    std::string about() const { return _about; }
    PackageType type() const { return _type; }
    std::vector<std::string> depends(bool) const { return _depends; }
  };

 protected:
  std::string _pkgfile;
  std::shared_ptr<Archive::Package> _package;

 public:
  Archive(std::string const &packagefile) : _pkgfile{packagefile} {}

  /**
   * @brief Provides the file data of specified file in the package
   * @param filepath path to the file in Package (must be started from ./)
   * @return content of file
   */
  virtual std::tuple<int, std::string> getdata(std::string const &filepath) = 0;

  virtual std::shared_ptr<Archive::Package> info() = 0;
  /**
   * List all files in the archive
   */
  virtual std::vector<std::string> list() = 0;

  virtual bool is_exist(std::string const &path) = 0;

  virtual bool extract(std::string const &outdir) = 0;

  virtual bool compress(std::string const &srcdir,
                        std::shared_ptr<PackageInformation> const &info) = 0;
};
}  // namespace rlxos::libpkgupd

#endif