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

        std::string workingDirectory,
            packagesDirectory;

        bool isForceFlagSet = false;

        bool Prepare(std::vector<std::string> const &sources, std::string const &srcdir);

        bool Compile(std::string const &srcdir, std::string const &pkgdir, std::shared_ptr<RecipePackageInfo> package);

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

        void SetForceFlag(bool value)
        {
            isForceFlagSet = value;
        }

        bool Build(std::shared_ptr<RecipePackageInfo> package);

        std::vector<std::string> const &PackagesList() const
        {
            return packagesArchiveList;
        }

        bool Build()
        {
            for (auto const &pkg : packages)
                if (!Build(pkg))
                    return false;

            return true;
        }
    };
}

#endif