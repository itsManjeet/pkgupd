#ifndef LIBPKGUPD_BUILDER
#define LIBPKGUPD_BUILDER

#include <filesystem>
#include <system_error>

#include "../configuration.hxx"
#include "../defines.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"
#include "../utils/utils.hxx"

namespace rlxos::libpkgupd {
    class Recipe;

    class Builder : public Object {
        Configuration* mConfig;
        std::string mBuildDir;
        std::string mSourceDir;
        std::string mPackageDir;

        std::vector<std::string> mPackages;

    public:
        explicit Builder(Configuration* config) : mConfig{config} {
            mBuildDir = config->get<std::string>("dir.build",
                                                 "/tmp/pkgupd-" + utils::random(10));
            mSourceDir = config->get<std::string>(DIR_SRC, DEFAULT_SRC_DIR);
            mPackageDir = config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR);
        }

        ~Builder() {
            if (mConfig->get("build.clean", true)) {
                if (std::filesystem::exists(mBuildDir)) {
                    std::error_code err;
                    std::filesystem::remove_all(mBuildDir, err);
                }
            }
        }

        bool prepare(std::vector<std::string> const& sources, std::string const& dir);

        std::vector<std::string> packages() {
            std::vector<std::string> build_packages = mPackages;
            mPackages.clear();
            return build_packages;
        }

        bool pack(
            std::vector<std::pair<MetaInfo, std::string>> const
            & dirs);

        bool compile(const Recipe& recipe, const std::string& dir, const std::string& destdir);

        bool build(const Recipe& recipe, SystemDatabase* systemDatabase,
                   Repository* repository);
    };
} // namespace rlxos::libpkgupd

#endif
