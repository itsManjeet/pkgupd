#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include <memory>

#include "configuration.hxx"
#include "MetaInfo.hxx"

namespace rlxos::libpkgupd {
    class Repository {
        Configuration* mConfig;
        std::vector<std::string> repos_list;
        std::string repo_dir;
        std::map<std::string, MetaInfo> mPackages;

    public:
        explicit Repository(Configuration* config);

        void init();

        [[nodiscard]] std::map<std::string, MetaInfo> const& get() const {
            return mPackages;
        }

        [[nodiscard]] std::vector<std::string> const& repos() const { return repos_list; }

        std::optional<MetaInfo> get(const std::string& id);
    };
} // namespace rlxos::libpkgupd

#endif
