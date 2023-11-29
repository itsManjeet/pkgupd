#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include <yaml-cpp/yaml.h>

#include <utility>

#include "configuration.hxx"
#include "MetaInfo.hxx"

namespace rlxos::libpkgupd {
    struct InstalledMetaInfo : MetaInfo {
        std::string timestamp;
        bool dependency{false};

        InstalledMetaInfo() = default;

        explicit InstalledMetaInfo(const YAML::Node& node);

        explicit InstalledMetaInfo(const MetaInfo& meta_info, std::string timestamp, bool dependency)
            : MetaInfo{meta_info},
              timestamp(std::move(timestamp)),
              dependency(dependency) {
        }

        [[nodiscard]] std::string str() const;
    };

    class SystemDatabase {
        Configuration* mConfig;
        std::string data_dir;

        std::map<std::string, InstalledMetaInfo> mPackages;

    public:
        explicit SystemDatabase(Configuration* config) : mConfig{config} {
            data_dir = mConfig->get<std::string>(DIR_DATA, DEFAULT_DATA_DIR);
            init();
        }

        void init();

        [[nodiscard]] std::map<std::string, InstalledMetaInfo> const& get()
        const {
            return mPackages;
        }

        void get_files(const InstalledMetaInfo& installed_meta_info,
                       std::vector<std::string>& files) const;

        std::optional<InstalledMetaInfo> get(const std::string& id);

        InstalledMetaInfo add(const MetaInfo& pkginfo,
                              std::vector<std::string> const& files,
                              const std::string& root, bool update = false,
                              bool is_dependency = false);

        void remove(const std::string& id);
    };
} // namespace rlxos::libpkgupd

#endif
