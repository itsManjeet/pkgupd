#include "SystemDatabase.hh"
#include <ctime>
#include <sys/stat.h>
#include <string.h>
#include <fstream>

namespace rlxos::libpkgupd
{

    SystemPackageInfo::FileInfo::FileInfo(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(std::string, path);
        READ_VALUE(unsigned int, group);
        READ_VALUE(unsigned int, user);
        READ_VALUE(unsigned int, mode);
    }

    SystemPackageInfo::SystemPackageInfo(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(std::string, id);
        READ_VALUE(std::string, version);
        READ_VALUE(std::string, about);

        READ_VALUE(std::string, installedon);

        READ_LIST(std::string, depends);
        READ_OBJECT_LIST(FileInfo, files);
    }

    std::shared_ptr<PackageInfo> SystemDatabase::operator[](std::string const &pkgid)
    {
        auto packageDataFile = std::filesystem::path(datadir) / pkgid;
        if (!std::filesystem::exists(packageDataFile))
            return nullptr;

        return std::make_shared<SystemPackageInfo>(YAML::LoadFile(packageDataFile), packageDataFile);
    }

    bool SystemDatabase::IsRegistered(std::shared_ptr<PackageInfo> pkginfo)
    {
        if ((*this)[pkginfo->ID()] == nullptr)
            return false;

        return true;
    }

    bool SystemDatabase::IsOutDated(std::shared_ptr<PackageInfo> pkginfo)
    {
        auto installedPackage = (*this)[pkginfo->ID()];
        if (installedPackage == nullptr)
            throw std::runtime_error(pkginfo->ID() + " is missing in SystemDatabase");

        return (installedPackage->Version() != pkginfo->Version());
    }

    bool SystemDatabase::Register(std::shared_ptr<PackageInfo> pkginfo, std::vector<std::string> const &files, std::string root, bool toupdate)
    {

        try
        {
            if (IsRegistered(pkginfo) && !IsOutDated(pkginfo) && !toupdate)
            {
                error = pkginfo->ID() + " " + pkginfo->Version() + " is already registered in the system";
                return false;
            }

            toupdate = IsOutDated(pkginfo);
        }
        catch (...)
        {
        }

        auto packageDataFile = std::filesystem::path(datadir) / pkginfo->ID();

        std::ofstream fileptr(packageDataFile);
        if (!fileptr.is_open())
        {
            error = "Failed to open SystemDatabase to register " + pkginfo->ID();
            return false;
        }

        fileptr << "id: " << pkginfo->ID() << std::endl
                << "version: " << pkginfo->Version() << std::endl
                << "about: " << pkginfo->About() << std::endl;

        if (pkginfo->Depends(false).size())
        {
            fileptr << "depends:" << std::endl;
            for (auto const &i : pkginfo->Depends(false))
                fileptr << " - " << i << std::endl;
        }

        std::time_t t = std::time(0);
        std::tm *now = std::localtime(&t);

        fileptr << "installedon: " << (now->tm_year + 1900) << "/" << (now->tm_mon + 1) << "/" << (now->tm_mday) << " " << (now->tm_hour) << ":" << (now->tm_min) << std::endl;

        fileptr << "files:" << std::endl;
        for (auto i : files)
        {
            if (i == "./" || i == "./.info")
                continue;

            i = i.substr(1, i.length() - 1);

            struct stat info;
            if (stat((root + i).c_str(), &info) == -1)
            {
                error = "Failed to read attributes of " + root + i + ", " + std::string(strerror(errno));
                return false;
            }

            fileptr << " - path: " << i << std::endl
                    << "   user: " << info.st_uid << std::endl
                    << "   group: " << info.st_gid << std::endl
                    << "   mode: " << info.st_mode << std::endl;
        }
        fileptr.close();

        return true;
    }
}