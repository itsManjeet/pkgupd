#include "ResolveDepends.hh"

namespace rlxos::libpkgupd
{
    bool ResolveDepends::toSkip(std::string const &pkgid)
    {
        if ((std::find(data.begin(), data.end(), pkgid) != data.end()))
            return true;

        if (skipper != nullptr)
        {
            if (skipper(pkgid))
                return true;
        }

        if ((std::find(visited.begin(), visited.end(), pkgid) != visited.end()))
            return true;

        visited.push_back(pkgid);
        return false;
    }
    bool ResolveDepends::Resolve(std::string const &pkgid, bool all)
    {

        if (toSkip(pkgid))
            return true;

        auto packageInfo = (*database)[pkgid];
        if (packageInfo == nullptr)
        {
            error = "Missing required dependency '" + pkgid + "'";
            return false;
        }

        for (auto const &i : packageInfo->Depends(all))
        {
            if (!Resolve(i, all))
            {
                error += "\n Trace Required by " + pkgid;
                return false;
            }
        }

        if ((std::find(data.begin(), data.end(), pkgid) == data.end()))
            data.push_back(pkgid);

        return true;
    }
}