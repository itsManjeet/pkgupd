#ifndef PKGUPD_REPOSITORY_DATABASE_H
#define PKGUPD_REPOSITORY_DATABASE_H

#include "MetaInfo.h"
#include <map>
#include <optional>

class Repository {
    std::filesystem::path path;
    std::map<std::string, MetaInfo> mPackages;

public:
    Repository() = default;

    void load(std::filesystem::path p);

    [[nodiscard]] std::map<std::string, MetaInfo> const& get() const {
        return mPackages;
    }

    std::optional<MetaInfo> get(const std::string& id) const;
};

#endif
