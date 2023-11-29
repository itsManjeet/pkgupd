#ifndef LIBPKGUPD_METAINFO_HXX
#define LIBPKGUPD_METAINFO_HXX

#include <yaml-cpp/yaml.h>

#include "defines.hxx"


namespace rlxos::libpkgupd {
    /**
     * @brief PackageInfo holds all the information of rlxos packages
     * their dependencies and required user groups
     */

    struct MetaInfo {
        std::string id, version, about;
        std::string integration, cache;

        std::vector<std::string> depends, backup;

        MetaInfo() = default;

        explicit MetaInfo(const YAML::Node& node) { update_from(node); }

        [[nodiscard]] std::string str() const;

        std::string package_name() const;

    protected:
        void update_from(const YAML::Node& node);
    };
} // namespace rlxos::libpkgupd

#endif
