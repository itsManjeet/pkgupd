#ifndef LIBPKGUPD_METAINFO_HXX
#define LIBPKGUPD_METAINFO_HXX


#include "Configuration.h"
#include "defines.hxx"


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


    explicit MetaInfo(const std::string &input) { update_from(input); }

    [[nodiscard]] std::string name() const;

    [[nodiscard]] virtual std::string str() const;

    [[nodiscard]] std::string package_name() const;

protected:
    void update_from(const std::string &input);
};

#endif
