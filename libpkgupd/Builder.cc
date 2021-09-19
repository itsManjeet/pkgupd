#include "Builder.hh"

#include "../libpkgupd/Downloader.hh"
#include "../libpkgupd/Archive.hh"
#include "../libpkgupd/Command.hh"
#include "../libpkgupd/rlxArchive.hh"
#include "../libpkgupd/Stripper.hh"

#include "Compiler.hh"

#include <iostream>

namespace rlxos::libpkgupd
{

    void Builder::SetPackages(std::vector<std::shared_ptr<RecipePackageInfo>> const &packages)
    {
        this->packages = packages;
    }
    bool Builder::Prepare(std::vector<std::string> const &sources, std::string const &srcdir)
    {
        for (auto const &i : sources)
        {
            std::string pkgfile = std::filesystem::path(i).filename().string();
            std::string url = i;

            size_t idx = i.rfind("::");
            if (idx != std::string::npos)
            {
                pkgfile = i.substr(0, idx);
                url = i.substr(idx + 2, i.length() - (idx + 2));
            }

            std::string outfile = sourcesDirectory + "/" + pkgfile;

            auto downloader = Downloader();

            if (!std::filesystem::exists(outfile))
            {
                if (!downloader.PerformCurl(url, outfile))
                {
                    error = downloader.Error();
                    return false;
                }
            }

            auto archiver = Archive(outfile);

            auto endswith = [](std::string const &fullString, std::string const &ending)
            {
                if (fullString.length() >= ending.length())
                {
                    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
                }
                else
                {
                    return false;
                }
            };

            bool extracted = false;

            for (auto const &i : {".tar", ".gz", ".tgz", ".xz", ".txz", ".bzip2", ".bz", ".bz2", ".lzma"})
            {
                if (endswith(outfile, i))
                {
                    if (Command::ExecuteScript("tar -xaf " + outfile + " -C " + srcdir, "", {}) != 0)
                    {
                        error = "failed to extract " + outfile + " with tar";
                        return false;
                    }
                    extracted = true;
                    break;
                }
            }

            if (endswith(outfile, "zip"))
            {
                if (Command::ExecuteScript("unzip " + outfile + " -d " + srcdir, "", {}) != 0)
                {
                    error = "failed to extract " + outfile + " with tar";
                    return false;
                }
                extracted = true;
            }

            if (!extracted)
            {
                std::error_code err;
                std::filesystem::copy(outfile, srcdir + "/" + std::filesystem::path(outfile).filename().string(), err);
                if (err)
                {
                    error = err.message();
                    return false;
                }
            }
        }

        return true;
    }

    bool Builder::Compile(std::string const &srcdir, std::string const &destdir, std::shared_ptr<RecipePackageInfo> package)
    {
        auto compiler = Compiler(package);
        if (!compiler.Compile(srcdir, destdir))
        {
            error = compiler.Error();
            return false;
        }

        return true;
    }

    bool Builder::Build(std::shared_ptr<RecipePackageInfo> package)
    {
        std::string packageWorkDir = workingDirectory + "/" + package->ID();
        std::string packageSrcDir = packageWorkDir + "/src";
        std::string packagePkgDir = packageWorkDir + "/pkg";

        std::string outputPackage = packagesDirectory + "/" + package->PackageFile(package->Pack());
        if (std::filesystem::exists(outputPackage) && !isForceFlagSet)
        {
            std::cout << "Found in cache, skipping" << std::endl;
            if (package->Pack() != "none")
            {
                packagesArchiveList.push_back(outputPackage);
            }
            return true;
        }

        for (auto const &dir : {packageSrcDir})
        {
            std::error_code err;
            std::filesystem::create_directories(dir, err);
            if (err)
            {
                error = err.message();
                return false;
            }
        }

        auto allSources = package->Sources();

        if (!Prepare(allSources, packageSrcDir))
        {
            return false;
        }

        // Inserting required environment variables
        package->AddEnviron("pkgupd_srcdir=" + packageSrcDir);
        package->AddEnviron("pkgupd_pkgdir=" + packagePkgDir);

        if (package->PreScript().size())
        {
            if (int status = Command::ExecuteScript(package->PreScript(), packageSrcDir + "/" + package->Dir(), package->Environ()); status != 0)
            {
                error = "prescript failed to exit code: " + std::to_string(status);
                return false;
            }
        }

        std::cout << "=> Compiling source code" << std::endl;
        if (!Compile(packageSrcDir + "/" + package->Dir(), packagePkgDir, package))
        {
            return false;
        }

        if (package->PostScript().size())
        {
            if (int status = Command::ExecuteScript(package->PostScript(), packageSrcDir + "/" + package->Dir(), package->Environ()); status != 0)
            {
                error = "postscript failed to exit code: " + std::to_string(status);
                return false;
            }
        }

        if (!package->NoStrip())
        {
            Stripper stripper;
            stripper.SetSkip(package->SkipStrip());

            std::cout << "=> Stripping " + package->ID() << std::endl;
            if (!stripper.Strip(packagePkgDir))
            {
                error = stripper.Error();
                return false;
            }
        }

        if (package->Pack() == "none")
        {
            std::cout << ":: No packaging done ::" << std::endl;
        }
        else
        {
            std::cout << "=> Packaging rlx archive" << std::endl;

            auto archive = rlxArchive(outputPackage);
            if (!archive.Pack(packagePkgDir, package))
            {
                error = archive.Error();
                return false;
            }

            std::cout << ":: Package generated at " + outputPackage << std::endl;
            packagesArchiveList.push_back(outputPackage);
        }

        return true;
    }
}