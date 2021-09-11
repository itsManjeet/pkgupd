#ifndef _INSTALLER_HH_
#define _INSTALLER_HH_

#include "Defines.hh"
#include "SystemDatabase.hh"
#include "RepositoryDatabase.hh"
#include "Downloader.hh"

namespace rlxos::libpkgupd
{
    class Installer : public Object
    {
    private:
        std::vector<std::string> packages;
        std::shared_ptr<SystemDatabase> mSystemDatabase;
        std::shared_ptr<RepositoryDatabase> mRepositoryDatabase;
        std::shared_ptr<Downloader> mDownloader;

        std::string packagesDir = DEFAULT_PKGS_DIR;

        bool isSkipTriggers = false;
        bool isForced = false;

    public:
        Installer()
        {
        }

        void SetPackagesDir(std::string const &pkgDir)
        {
            packagesDir = pkgDir;
        }

        void AddPackage(std::string const &pkg)
        {
            packages.push_back(pkg);
        }

        void SetPackages(std::vector<std::string> const &pkgs)
        {
            packages = pkgs;
        }

        void SetSkipTriggers(bool t)
        {
            isSkipTriggers = t;
        }

        void SetForced(bool t)
        {
            isForced = t;
        }

        void SetSystemDatabase(std::string const &path)
        {
            mSystemDatabase = std::make_shared<SystemDatabase>(path);
        }

        void SetSystemDatabase(std::shared_ptr<SystemDatabase> systemDatabase)
        {
            mSystemDatabase = systemDatabase;
        }

        void SetRepositoryDatabase(std::string const &path)
        {
            mRepositoryDatabase = std::make_shared<RepositoryDatabase>(path);
        }

        void SetRepositoryDatabase(std::shared_ptr<RepositoryDatabase> repositoryDatabase)
        {
            mRepositoryDatabase = repositoryDatabase;
        }

        void SetDownloader(std::shared_ptr<Downloader> downloader)
        {
            mDownloader = downloader;
        }

        bool Install(std::string const &rootDir);

        bool InstallPackages(std::vector<std::string> const &packages, std::string const &rootDir);
    };
}

#endif