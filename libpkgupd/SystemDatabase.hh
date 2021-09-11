#ifndef _PKGUPD_SYSTEM_DATABASE_HH_
#define _PKGUPD_SYSTEM_DATABASE_HH_

#include "Database.hh"
#include "yaml-cpp/yaml.h"

namespace rlxos::libpkgupd
{

    class SystemPackageInfo : public PackageInfo
    {
    public:
        class FileInfo
        {
        private:
            std::string path;
            unsigned int group,
                user, mode;

        public:
            FileInfo(YAML::Node const &data, std::string const &file);

            std::string const &Path() const
            {
                return path;
            }
        };

    private:
        std::string id;
        std::string version;
        std::string about;

        std::vector<std::string> depends;
        std::vector<std::shared_ptr<FileInfo>> files;

        std::string installedon;

        std::string requiredby;

    public:
        SystemPackageInfo(YAML::Node const &data, std::string const &file);

        std::string ID() const
        {
            return id;
        }

        std::string Version() const
        {
            return version;
        }

        std::string About() const
        {
            return about;
        }

        std::vector<std::string> Depends(bool all) const
        {
            return depends;
        }

        std::vector<std::shared_ptr<FileInfo>> Files() const
        {
            return files;
        }

        std::string const &InstalledOn() const
        {
            return installedon;
        }

        std::string const &RequiredBy() const
        {
            return requiredby;
        }
    };

    class SystemDatabase : public Database
    {

    public:
        SystemDatabase()
            : Database(DEFAULT_DATA_DIR)
        {
        }

        SystemDatabase(std::string const &datadir)
            : Database(datadir)
        {
        }

        std::shared_ptr<PackageInfo> operator[](std::string const &pkgid);

        bool IsRegistered(std::shared_ptr<PackageInfo> pkginfo);

        bool IsOutDated(std::shared_ptr<PackageInfo> pkginfo);

        bool Register(std::shared_ptr<PackageInfo> pkginfo, std::vector<std::string> const &files, std::string root, bool update = false);
        
        bool UnRegister(std::shared_ptr<PackageInfo> pkginfo);
    };
}

#endif