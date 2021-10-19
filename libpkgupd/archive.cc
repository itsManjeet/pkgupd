#include "archive.hh"

#include <fstream>

#include "exec.hh"

namespace rlxos::libpkgupd {

archive::package::package(YAML::Node const &data, std::string const &file) {
    READ_VALUE(std::string, id);
    READ_VALUE(std::string, version);
    READ_VALUE(std::string, about);
    READ_LIST(std::string, depends);

    READ_OBJECT_LIST(pkginfo::user, users);
    READ_OBJECT_LIST(pkginfo::group, groups);

    OPTIONAL_VALUE(std::string, install_script, "");
}

}  // namespace rlxos::libpkgupd