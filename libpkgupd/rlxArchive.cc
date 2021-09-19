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

        DEBUG("Input Data: " << data);

        auto sysPkgInfo = std::make_shared<rlxArchiveInfo>(YAML::Load(data), pkgfile);
        return sysPkgInfo;
    }

    bool rlxArchive::IsArchive(std::string const &pkgpath)
    {
        return (std::filesystem::exists(pkgpath));
    }

    bool rlxArchive::Extract(std::string const &outdir, std::vector<std::string> excludefile)
    {
        AddArgs("-h");
        AddArgs("-p");
        return Archive::Extract(outdir, excludefile);
    }

    bool rlxArchive::Pack(std::string const &srcdir, std::shared_ptr<PackageInfo> const &pkginfo)
    {
        for (auto const &i : std::filesystem::recursive_directory_iterator(srcdir))
        {
            if (i.is_regular_file() && i.path().filename().extension() == "la")
            {
                DEBUG("removing " + i.path().string());
                std::filesystem::remove(i);
            }
        }

        std::ofstream fileptr(srcdir + "/.info");

        fileptr << "id: " << pkginfo->ID() << "\n"
                << "version: " << pkginfo->Version() << "\n"
                << "about: " << pkginfo->About() << "\n";

        if (pkginfo->Depends(false).size())
        {
            fileptr << "depends:"
                    << "\n";
            for (auto const &i : pkginfo->Depends(false))
                fileptr << " - " << i << "\n";
        }

        fileptr.close();

        setenv("XZ_OPT","-9", 1);

        return Compress(srcdir);
    }

}