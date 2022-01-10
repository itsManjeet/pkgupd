#include "archive.hh"

#include <fstream>

#include "exec.hh"

namespace rlxos::libpkgupd {

Archive::Package::Package(YAML::Node const &data, std::string const &file) {
  READ_VALUE(std::string, id);
  READ_VALUE(std::string, version);
  READ_VALUE(std::string, about);
  READ_LIST(std::string, depends);

  READ_OBJECT_LIST(PackageInformation::User, users);
  READ_OBJECT_LIST(PackageInformation::Group, groups);

  _type = PackageType::RLX;
  if (data["type"]) {
    _type = PackageInformation::str2pkgtype(data["type"].as<std::string>());
  }

  OPTIONAL_VALUE(std::string, install_script, "");
}

}  // namespace rlxos::libpkgupd