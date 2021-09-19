#ifndef _LIBPKGUPD_BUILDER_HH_
#define _LIBPKGUPD_BUILDER_HH_

#include "../libpkgupd/Defines.hh"
#include "../libpkgupd/RecipeFile.hh"

namespace rlxos::libpkgupd
{
    class Builder : public Object
    {
    private:
        std::vector<std::shared_ptr<RecipePackageInfo>> packages;
        std::vector<std::string> packagesArchiveList;

        std::string workingDirectory = "/tmp",
                    packagesDirectory = DEFAULT_PKGS_DIR,
                    sourcesDirectory = DEFAULT_SRC_DIR;

        bool isForceFlagSet = false;

        bool Prepare(std::vector<std::string> const &sources, std::string const &srcdir);

        bool Compile(std::string const &srcdir, std::string const &pkgdir, std::shared_ptr<RecipePackageInfo> package);

        bool Build(std::shared_ptr<RecipePackageInfo> package);

    public:
        Builder() {}

        void SetPackages(std::vector<std::shared_ptr<RecipePackageInfo>> const &packages);

        void SetWorkDir(std::string const &workdir)
        {
            workingDirectory = workdir;
        }

        void SetPackageDir(std::string const &pkgdir)
        {
            packagesDirectory = pkgdir;
        }

        void SetSourceDir(std::string const &srcdir)
        {
            sourcesDirectory = srcdir;
        }

        void SetForceFlag(bool value)
        {
            isForceFlagSet = value;
        }

        std::vector<std::string> const &PackagesList() const
        {
            return packagesArchiveList;
        }

        bool Build()
        {
            workingDirectory += "/" + packages[0]->ID();

            for (auto const &dir : {packagesDirectory, sourcesDirectory, workingDirectory})
            {
                if (!std::filesystem::exists(dir))
                {
                    std::error_code err;
                    std::filesystem::create_directories(dir, err);
                    if (err)
                    {
                        error = err.message();
                        return false;
                    }
                }
            }
            for (auto const &pkg : packages)
                if (!Build(pkg))
                {
                    std::filesystem::remove_all(workingDirectory);
                    return false;
                }

            return true;
        }
    };
}

#endif