#ifndef _LIBPKGUPD_RLXARCHIVE_HH_
#define _LIBPKGUPD_RLXARCHIVE_HH_

#include "Archive.hh"
#include "SystemDatabase.hh"

namespace rlxos::libpkgupd
{
    class rlxArchiveInfo : public PackageInfo
    {
    private:
        std::string id;
        std::string version;
        std::string about;

        std::vector<std::string> depends;

    public:
        rlxArchiveInfo(YAML::Node const &data, std::string const &file);

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

        std::vector<std::string> Depends(bool) const
        {
            return depends;
        }
    };

    class rlxArchive : public Archive
    {
    public:
        rlxArchive(std::string const &filepath)
            : Archive(filepath)
        {
            AddArgs("--zstd");
        }

        std::shared_ptr<rlxArchiveInfo> GetInfo();

        bool Pack(std::string const &srcdir, std::shared_ptr<PackageInfo> const &pkginfo);

        static bool IsArchive(std::string const &pkgpath);
    };
}

#endif