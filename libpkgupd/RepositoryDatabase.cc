#include "RepositoryDatabase.hh"
#include "RecipeFile.hh"

namespace rlxos::libpkgupd
{
    std::shared_ptr<PackageInfo> RepositoryDatabase::operator[](std::string const &pkgid)
    {
        auto directPath = std::filesystem::path(datadir) / (pkgid + ".yml");
        if (std::filesystem::exists(directPath))
        {
            std::shared_ptr<RecipeFile> recipeFile;
            try
            {
                recipeFile = RecipeFile::FromFilePath(directPath);
            }
            catch (YAML::Exception const &ee)
            {
                error = "Failed to read recipe file '" + directPath.string() + "' " + std::string(ee.what());
                return nullptr;
            }

            auto packageInfo = (*recipeFile)[pkgid];
            if (packageInfo == nullptr)
            {
                error = "no package with id '" + pkgid + "' found in recipe file " + directPath.string();
                return nullptr;
            }
            return packageInfo;
        }

        // if sub package lib32
        std::string guessedRecipeFile;

        if (pkgid.rfind("lib32", 0) == 0 && pkgid.length() > 6)
            guessedRecipeFile = pkgid.substr(5, pkgid.length() - 5);
        else if (pkgid.rfind("lib", 0) == 0 && pkgid.length() > 0)
            guessedRecipeFile = pkgid.substr(3, pkgid.length() - 3);
        else
        {
            size_t rdx = pkgid.find_last_of('-');
            if (rdx == std::string::npos)
            {
                error = "No package found with id '" + pkgid + "'";
                return nullptr;
            }

            guessedRecipeFile = pkgid.substr(0, rdx);
        }

        guessedRecipeFile = (std::filesystem::path(datadir) / guessedRecipeFile).string() + ".yml";

        if (!std::filesystem::exists(guessedRecipeFile))
        {
            error = "No package found with id '" + pkgid + "'";
            return nullptr;
        }

        std::shared_ptr<RecipeFile> recipeFile;
        try
        {
            recipeFile = RecipeFile::FromFilePath(guessedRecipeFile);
        }
        catch (YAML::Exception const &ee)
        {
            error = "Failed to read recipe file '" + guessedRecipeFile + "' " + std::string(ee.what());
            return nullptr;
        }

        auto packageInfo = (*recipeFile)[pkgid];
        if (packageInfo == nullptr)
        {
            error = "No package found with id '" + pkgid + "' in detected recipe file " + guessedRecipeFile;
            return nullptr;
        }

        return packageInfo;
    }
}