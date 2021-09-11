#ifndef _PKGUPD_DATABASE_HH_
#define _PKGUPD_DATABASE_HH_

#include "Defines.hh"
#include "PackageInfo.hh"

namespace rlxos::libpkgupd
{
    class Database : public Object
    {
    protected:
        std::string datadir;

    public:
        Database(std::string const &datadir)
            : datadir{datadir}
        {
        }

        virtual std::shared_ptr<PackageInfo> operator[](std::string const &pkgid) = 0;
    };
}

#endif