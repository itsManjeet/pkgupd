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

            std::string outfile = srcdir + "/" + pkgfile;

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
            if (!archiver.Extract(srcdir))
            {
                error = archiver.Error();
                return false;
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
        if (std::filesystem::exists(outputPackage) && getenv("FORCE") == nullptr)
        {
            std::cout << "Found in cache, skipping" << std::endl;
            return true;
        }

        auto cleanup = [&]()
        {
            if (getenv("NOCLEAN") == nullptr)
                std::filesystem::remove_all(packageWorkDir);
        };

        for (auto const &dir : {packageSrcDir})
        {
            std::error_code err;
            std::filesystem::create_directories(dir, err);
            if (err)
            {
                error = err.message();
                cleanup();
                return false;
            }
        }

        auto allSources = package->Sources();

        if (!Prepare(allSources, packageSrcDir))
        {
            cleanup();
            return false;
        }

        // Inserting required environment variables
        package->AddEnviron("pkgupd_srcdir=" + packageSrcDir);
        package->AddEnviron("pkgupd_pkgdir=" + packagePkgDir);

        if (package->PreScript().size())
        {
            std::cout << "=> Executing Pre Script" << std::endl;
            auto binary = "/bin/sh";
            std::vector<std::string> args = {"-e", "-u", "-c", package->PreScript()};
            auto cmd = Command(binary, args);
            cmd.SetEnviron(package->Environ());
            cmd.SetDirectory(packageSrcDir + "/" + package->Dir());

            if (int status = cmd.Execute(); status != 0)
            {
                error = "prescript failed to exit code: " + std::to_string(status);
                return false;
            }
        }

        std::cout << "=> Compiling source code" << std::endl;
        if (!Compile(packageSrcDir + "/" + package->Dir(), packagePkgDir, package))
        {
            cleanup();
            return false;
        }

        if (package->PostScript().size())
        {
            std::cout << "=> Executing Post Script" << std::endl;
            auto binary = "/bin/sh";
            std::vector<std::string> args = {"-e", "-u", "-c", package->PostScript()};
            auto cmd = Command(binary, args);
            cmd.SetEnviron(package->Environ());
            cmd.SetDirectory(packageSrcDir + "/" + package->Dir());

            if (int status = cmd.Execute(); status != 0)
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
                cleanup();
                return false;
            }

            std::cout << ":: Package generated at " + outputPackage << std::endl;
            packagesArchiveList.push_back(outputPackage);
        }

        cleanup();
        return true;
    }
}