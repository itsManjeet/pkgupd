#ifndef PKGUPD_SYSTEM_DATABASE_H
#define PKGUPD_SYSTEM_DATABASE_H

#include "Configuration.h"
#include "MetaInfo.h"
#include <map>
#include <optional>
#include <utility>

struct InstalledMetaInfo : MetaInfo {
    std::string timestamp;
    bool dependency{false};

    InstalledMetaInfo() = default;

    explicit InstalledMetaInfo(const std::string& input);

    explicit InstalledMetaInfo(
            const MetaInfo& meta_info, std::string timestamp, bool dependency)
            : MetaInfo{meta_info}, timestamp(std::move(timestamp)),
              dependency(dependency) {}

    [[nodiscard]] std::string str() const override;
};

class SystemDatabase {
    std::filesystem::path data_dir;

    std::map<std::string, InstalledMetaInfo> mPackages;

public:
    SystemDatabase() = default;

    void load(std::filesystem::path p);

    [[nodiscard]] std::map<std::string, InstalledMetaInfo> const& get() const {
        return mPackages;
    }

    void get_files(const InstalledMetaInfo& installed_meta_info,
            std::vector<std::string>& files) const;

    [[nodiscard]] std::optional<InstalledMetaInfo> get(
            const std::string& id) const;

    InstalledMetaInfo add(const MetaInfo& pkginfo,
            std::vector<std::string> const& files, const std::string& root,
            bool update = false, bool is_dependency = false);

    void remove(const InstalledMetaInfo& installed_meta_info);
};

#endif
