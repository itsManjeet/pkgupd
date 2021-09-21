#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include "defines.hh"
#include "sysdb.hh"
#include "repodb.hh"

#include <functional>

namespace rlxos::libpkgupd
{
    class resolver : public object
    {
    private:
        repodb &_repodb;
        sysdb &_sysdb;
        std::vector<std::string> _data, _visited;

        bool _to_skip(std::string const &pkgid);

    public:
        resolver(repodb &rp, sysdb &sd)
            : _repodb{rp},
              _sysdb{sd}
        {
        }

        bool resolve(std::string const &pkgid, bool all = false);

        GET_METHOD(std::vector<std::string>, data);
    };
}

#endif