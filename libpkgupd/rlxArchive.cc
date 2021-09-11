#include "rlxArchive.hh"
#include <fstream>

namespace rlxos::libpkgupd
{

    rlxArchiveInfo::rlxArchiveInfo(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(std::string, id);
        READ_VALUE(std::string, version);
        READ_VALUE(std::string, about);
        READ_LIST(std::string, depends);
    }

    std::shared_ptr<rlxArchiveInfo> rlxArchive::GetInfo()
    {
        auto [status, data] = ReadFile("./.info");
        if (status != 0)
        {
            error = "Failed to read rlxArchive";
            return nullptr;
        }

        auto sysPkgInfo = std::make_shared<rlxArchiveInfo>(YAML::Load(data), pkgfile);
        return sysPkgInfo;
    }

    bool rlxArchive::IsArchive(std::string const &pkgpath)
    {
        return (std::filesystem::exists(pkgpath));
    }

    bool rlxArchive::Pack(std::string const &srcdir, std::shared_ptr<PackageInfo> const &pkginfo)
    {
        std::ofstream fileptr(srcdir + "/.info");

        fileptr << "id: " << pkginfo->ID() << std::endl
                << "version: " << pkginfo->Version() << std::endl
                << "about: " << pkginfo->About() << std::endl;

        if (pkginfo->Depends(false).size())
        {
            fileptr << "depends:" << std::endl;
            for (auto const &i : pkginfo->Depends(false))
                fileptr << " - " << i << std::endl;
        }

        fileptr.close();

        return Compress(srcdir);
    }

}