#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include "defines.hh"

namespace rlxos::libpkgupd
{
    class pkginfo
    {
    public:
        virtual std::string id() const = 0;
        virtual std::string version() const = 0;
        virtual std::string about() const = 0;

        virtual std::string packagefile(std::string ext = DEFAULT_EXTENSION) const
        {
            return id() + "-" + version() + "." + ext;
        }

        virtual std::vector<std::string> depends(bool) const = 0;
    };
}

#endif