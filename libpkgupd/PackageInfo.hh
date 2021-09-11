#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class PackageInfo
    {
    public:
        virtual std::string ID() const = 0;
        virtual std::string Version() const = 0;
        virtual std::string About() const = 0;

        virtual std::string PackageFile(std::string ext = DEFAULT_EXTENSION) const
        {
            return ID() + "-" + Version() + "." + ext;
        }

        virtual std::vector<std::string> Depends(bool) const = 0;
    };
}

#endif