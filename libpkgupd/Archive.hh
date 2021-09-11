#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class Archive : public Object
    {
    protected:
        std::vector<std::string> args;
        std::string pkgfile;

        std::string archiveTool = DEFAULT_ARCHIVE_TOOL;

    public:
        Archive(std::string const &pkgfile)
            : pkgfile(pkgfile)
        {
        }

        void AddArgs(std::string const &a)
        {
            args.push_back(a);
        }

        void SetArchiveTool(std::string tool)
        {
            archiveTool = tool;
        }

        std::tuple<int, std::string> ReadFile(std::string const &path) const;

        std::vector<std::string> List();

        bool checkfile(std::string const &path) const;

        bool Extract(std::string const &outdir, std::vector<std::string> excludefile = {});

        bool Compress(std::string const &srcdir, std::vector<std::string> excludefile = {});
    };
}

#endif