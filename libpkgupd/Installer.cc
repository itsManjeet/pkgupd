#include "Installer.hh"
#include "rlxArchive.hh"
#include "Triggerer.hh"

#include <iostream>
#include <cassert>

namespace rlxos::libpkgupd
{

    bool Installer::InstallPackages(std::vector<std::string> const &packages, std::string const &rootDir)
    {
        std::vector<std::shared_ptr<PackageInfo>> packageInformationList;
        std::vector<std::vector<std::string>> allPackagesFiles;

        for (auto const &i : packages)
        {
            auto archive = rlxArchive(i);

            PROCESS("Getting information from " << std::filesystem::path(i).filename().string());

            auto pkginfo = archive.GetInfo();
            if (pkginfo == nullptr)
            {
                error = archive.Error();
                return false;
            }

            try
            {
                if (!isForced)
                {
                    if (mSystemDatabase->IsRegistered(pkginfo) && !mSystemDatabase->IsOutDated(pkginfo))
                    {
                        std::cout << ":: " + pkginfo->ID() + " Latest version is already installed ::" << std::endl;
                        continue;
                    }
                }
            }
            catch (...)
            {
            }

            PROCESS("Extracting " << pkginfo->ID() << " into " << rootDir);
            if (!archive.Extract(rootDir, {"./.info"}))
            {
                error = archive.Error();
                return false;
            }
            allPackagesFiles.push_back(archive.List());
            packageInformationList.push_back(pkginfo);
        }

        assert(allPackagesFiles.size() == packageInformationList.size());

        for (int i = 0; i < packageInformationList.size(); i++)
        {
            PROCESS("Registering " << packageInformationList[i]->ID() << " into system database");
            if (!mSystemDatabase->Register(packageInformationList[i], allPackagesFiles[i], rootDir, isForced))
            {
                error = mSystemDatabase->Error();
                return false;
            }
        }

        if (isSkipTriggers)
        {
            INFO("Skipping Triggers")
        }
        else
        {
            auto triggerer = Triggerer();
            if (!triggerer.Trigger(allPackagesFiles))
            {
                error = triggerer.Error();
                return false;
            }
        }

        return true;
    }
    bool Installer::Install(std::string const &rootdir)
    {
        std::vector<std::string> packageArchiveFiles;

        for (auto const &i : packages)
        {
            if (rlxArchive::IsArchive(i))
            {
                packageArchiveFiles.push_back(i);
                continue;
            }

            auto pkginfo = (*mRepositoryDatabase)[i];
            if (pkginfo == nullptr)
            {
                error = "no package found with name '" + i + "'";
                return false;
            }

            if (mSystemDatabase->IsRegistered(pkginfo) && !isForced)
            {
                error = pkginfo->ID() + " is already installed in the system";
                return false;
            }

            auto pkgArchiveFile = pkginfo->PackageFile();
            auto pkgArchivePath = packagesDir + "/" + pkgArchiveFile;

            if (!std::filesystem::exists(pkgArchivePath))
            {
                if (!mDownloader->Download(pkgArchiveFile, pkgArchivePath))
                {
                    error = mDownloader->Error();
                    return false;
                }
            }

            packageArchiveFiles.push_back(pkgArchivePath);
        }

        return InstallPackages(packageArchiveFiles, rootdir);
    }
}