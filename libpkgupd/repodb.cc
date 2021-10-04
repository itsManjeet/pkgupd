#include "repodb.hh"

#include "recipe.hh"

namespace rlxos::libpkgupd {
std::shared_ptr<pkginfo> repodb::operator[](std::string const &pkgid) {
    auto direct_path = std::filesystem::path(_data_dir) / (pkgid + ".yml");
    if (std::filesystem::exists(direct_path)) {
        std::shared_ptr<recipe> recipe_;
        try {
            recipe_ = recipe::from_filepath(direct_path);
        } catch (YAML::Exception const &ee) {
            _error = "failed to read recipe file '" + direct_path.string() + "' " + std::string(ee.what());
            return nullptr;
        }

        auto pkginfo_ = (*recipe_)[pkgid];
        if (pkginfo_ == nullptr) {
            _error = "no package with id '" + pkgid + "' found in recipe file " + direct_path.string();
            return nullptr;
        }
        return pkginfo_;
    }

    // if sub package lib32
    std::string _try_rcp_file;

    if (pkgid.rfind("lib32", 0) == 0 && pkgid.length() > 6)
        _try_rcp_file = pkgid.substr(5, pkgid.length() - 5);
    else if (pkgid.rfind("lib", 0) == 0 && pkgid.length() > 0)
        _try_rcp_file = pkgid.substr(3, pkgid.length() - 3);
    else {
        size_t rdx = pkgid.find_last_of('-');
        if (rdx == std::string::npos) {
            _error = "no package found with id '" + pkgid + "'";
            return nullptr;
        }

        _try_rcp_file = pkgid.substr(0, rdx);
    }

    _try_rcp_file = (std::filesystem::path(_data_dir) / _try_rcp_file).string() + ".yml";

    if (!std::filesystem::exists(_try_rcp_file)) {
        _error = "no package found with id '" + pkgid + "'";
        return nullptr;
    }

    std::shared_ptr<recipe> recipe_;
    try {
        recipe_ = recipe::from_filepath(_try_rcp_file);
    } catch (YAML::Exception const &ee) {
        _error = "failed to read recipe file '" + _try_rcp_file + "' " + std::string(ee.what());
        return nullptr;
    }

    auto packageInfo = (*recipe_)[pkgid];
    if (packageInfo == nullptr) {
        _error = "no package found with id '" + pkgid + "' in detected recipe file " + _try_rcp_file;
        return nullptr;
    }

    return packageInfo;
}
}  // namespace rlxos::libpkgupd