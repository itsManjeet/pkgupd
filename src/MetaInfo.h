#ifndef LIBPKGUPD_METAINFO_HXX
#define LIBPKGUPD_METAINFO_HXX

#include "Configuration.h"
#include "defines.h"

/**
 * @brief PackageInfo holds all the information of rlxos packages
 * their dependencies and required user groups
 */

struct MetaInfo {
    std::string id, version, about;
    std::string integration, cache;

    std::vector<std::string> depends, backup;
    Configuration config;

    MetaInfo() = default;

    virtual ~MetaInfo() = default;

    static MetaInfo from_file(const std::string& filepath) {
        MetaInfo metaInfo;
        metaInfo.update_from_file(filepath);
        return metaInfo;
    }

    static MetaInfo from_data(
            const std::string& data, const std::string& filepath = "") {
        MetaInfo metaInfo;
        metaInfo.update_from_data(data, filepath);
        return metaInfo;
    }

    void update_from_data(const std::string& data, const std::string& filepath);

    void update_from_file(const std::string& filepath);

    [[nodiscard]] std::string name() const;

    [[nodiscard]] virtual std::string str() const;

    [[nodiscard]] std::string package_name(std::string eid = "") const;
};

#endif
