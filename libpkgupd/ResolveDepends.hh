#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include "Defines.hh"
#include "Database.hh"

#include <functional>

namespace rlxos::libpkgupd
{
    class ResolveDepends : public Object
    {
    private:
        std::shared_ptr<Database> database;
        std::vector<std::string> data, visited;
        std::function<bool(std::string const &pkgid)> skipper;

        bool toSkip(std::string const &pkgid);

    public:
        ResolveDepends(std::shared_ptr<Database> database)
            : database{database}
        {
        }

        void SetSkipper(std::function<bool(std::string const &)> skipper)
        {
            this->skipper = skipper;
        }

        bool Resolve(std::string const &pkgid, bool all = false);

        std::vector<std::string> const &GetData() const
        {
            return data;
        }
    };
}

#endif